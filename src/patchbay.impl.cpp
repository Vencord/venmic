#include "patchbay.impl.hpp"

#include "logger.hpp"

#include <stdexcept>

#include <glaze/glaze.hpp>

#include <range/v3/view.hpp>
#include <range/v3/range.hpp>

#include <range/v3/action.hpp>
#include <range/v3/algorithm.hpp>

namespace vencord
{
    struct pw_metadata_name
    {
        std::string name;
    };

    patchbay::impl::~impl()
    {
        should_exit = true;
        sender->send(quit{});

        thread.join();
    }

    patchbay::impl::impl()
    {
        auto [pw_sender, pw_receiver] = pw::channel<pw_recipe>();
        auto [cr_sender, cr_receiver] = cr::channel<cr_recipe>();

        sender   = std::make_unique<pw_recipe::sender>(std::move(pw_sender));
        receiver = std::make_unique<cr_recipe::receiver>(std::move(cr_receiver));

        auto thread_start = [this](auto receiver, auto sender)
        {
            start(std::move(receiver), std::move(sender));
        };

        thread = std::jthread{thread_start, std::move(pw_receiver), cr_sender};

        if (receiver->recv_as<ready>().success)
        {
            return;
        }

        throw std::runtime_error("Failed to create patchbay instance");
    }

    void patchbay::impl::create_mic()
    {
        auto node = core->create<pw::node>({"adapter",
                                            {{"audio.channels", "2"},
                                             {"audio.position", "FL,FR"},
                                             {"node.name", "vencord-screen-share"},
                                             {"media.class", "Audio/Source/Virtual"},
                                             {"factory.name", "support.null-audio-sink"}}})
                        .get();

        while (nodes[node->id()].ports.size() < 4)
        {
            core->update();
        }

        mic = std::make_unique<pw::node>(std::move(*node));
    }

    void patchbay::impl::relink(std::uint32_t id)
    {
        created.erase(id);

        if (!mic)
        {
            return;
        }

        if (!nodes.contains(id))
        {
            return;
        }

        if (id == mic->id())
        {
            logger::get()->warn("[patchbay] (relink) prevented link to self", id, mic->id());
            return;
        }

        logger::get()->debug("[patchbay] (relink) linking {} [with mic = {}]", id, mic->id());

        auto &target = nodes[id];
        auto &source = nodes[mic->id()];

        auto is_output = [](const auto &item)
        {
            return item.direction == pw::port_direction::output;
        };
        auto is_input = [](const auto &item)
        {
            return item.direction == pw::port_direction::input;
        };

        auto target_ports = target.ports | ranges::views::filter(is_output) | ranges::to<std::vector>;
        auto source_ports = source.ports | ranges::views::filter(is_input);

        logger::get()->debug("[patchbay] (relink) {} has {} port(s)", id, target_ports.size());

        for (auto &port : target_ports)
        {
            auto matching_channel = [&](auto &item)
            {
                if (target_ports.size() == 1)
                {
                    logger::get()->debug("[patchbay] (relink) {} is mono", item.id);
                    return true;
                }

                /*
                 * Sometimes, for whatever reason, the "audio.channel" of a venmic port may be "UNK", to circumvent any
                 * issues regarding this we'll fallback to "port.id"
                 */

                if (item.props["audio.channel"] == "UNK" || port.props["audio.channel"] == "UNK")
                {
                    return item.props["port.id"] == port.props["port.id"];
                }

                return item.props["audio.channel"] == port.props["audio.channel"];
            };

            auto others = source_ports | ranges::views::filter(matching_channel) | ranges::to<std::vector>;
            logger::get()->debug("[patchbay] (relink) {} maps to {} other(s)", port.id, others.size());

            for (auto &other : others)
            {
                auto link = core->create<pw::link, pw::link_factory>({other.id, port.id}).get();

                if (!link.has_value())
                {
                    logger::get()->warn("[patchbay] (relink) failed to link: {}", link.error().message);
                    return;
                }

                logger::get()->debug("[patchbay] (relink) created {}, which maps {}->{}", link->id(), other.id,
                                     port.props["audio.channel"]);

                created.emplace(id, std::move(*link));
            }
        }
    }

    template <>
    void patchbay::impl::add_global<pw::node>(pw::node &node)
    {
        auto id    = node.id();
        auto props = node.info().props;

        nodes[id].info = node.info();

        logger::get()->trace("[patchbay] (add_global) new node: {} (name: \"{}\", app: \"{}\")", node.id(),
                             props["node.name"], props["application.name"]);

        on_node(id);
    }

    template <>
    void patchbay::impl::add_global<pw::link>(pw::link &link)
    {
        auto id   = link.id();
        auto info = link.info();

        links[id] = info;

        logger::get()->trace(
            "[patchbay] (add_global) new link: {} (input-node: {}, output-node: {}, input-port: {}, output-port: {})",
            link.id(), info.input.node, info.output.node, info.input.port, info.output.port);

        on_link(id);
    }

    template <>
    void patchbay::impl::add_global<pw::port>(pw::port &port)
    {
        auto props = port.info().props;

        if (!props.contains("node.id"))
        {
            logger::get()->warn("[patchbay] (add_global) {} has no parent", port.id());
            return;
        }

        auto parent = std::stoull(props["node.id"]);
        nodes[parent].ports.emplace_back(port.info());

        logger::get()->trace("[patchbay] (add_global) new port: {} with parent {}", port.id(), parent);

        /*
        ? Yes. This belongs here, as the node is created **before** the ports.
        */

        on_node(parent);
    }

    template <>
    void patchbay::impl::add_global<pw::metadata>(pw::metadata &data)
    {
        auto props = data.properties();

        if (!props.contains("default.audio.sink"))
        {
            return;
        }

        auto parsed = glz::read_json<pw_metadata_name>(props["default.audio.sink"].value);

        if (!parsed.has_value())
        {
            logger::get()->warn("[patchbay] (add_global) failed to parse speaker");
            return;
        }

        logger::get()->debug("[patchbay] (add_global) speakers name: \"{}\"", parsed->name);

        speaker.emplace(parsed->name);

        for (const auto &[id, info] : nodes)
        {
            on_node(id);
        }
    }

    template <typename T>
    void patchbay::impl::bind(const pw::global &global)
    {
        auto bound = registry->bind<T>(global.id).get();

        if (!bound.has_value())
        {
            logger::get()->warn("[patchbay] (bind) failed to bind {} (\"{}\"): {}", global.id, global.type,
                                bound.error().message);
            return;
        }

        add_global(bound.value());
    }

    template <>
    void patchbay::impl::add_global<const pw::global>(const pw::global &global)
    {
        if (global.type == pw::node::type)
        {
            bind<pw::node>(global);
        }
        else if (global.type == pw::metadata::type)
        {
            bind<pw::metadata>(global);
        }
        else if (global.type == pw::port::type)
        {
            bind<pw::port>(global);
        }
        else if (global.type == pw::link::type)
        {
            bind<pw::link>(global);
        }
    }

    void patchbay::impl::rem_global(std::uint32_t id)
    {
        nodes.erase(id);
        created.erase(id);
    }

    void patchbay::impl::on_link(std::uint32_t id)
    {
        if (!include.empty() || !speaker)
        {
            return;
        }

        auto &info = links[id];

        if (info.input.node != speaker->id)
        {
            logger::get()->trace("[patchbay] (on_link) {} is not connected to speaker but with {}", id,
                                 info.input.node);
            return;
        }

        // "Output" = the node that is emitting sound
        auto &output = nodes[info.output.node];

        auto match = [&](const auto &prop)
        {
            return output.info.props[prop.key] == prop.value;
        };

        if (ranges::any_of(exclude, match))
        {
            return;
        }

        core->update();

        relink(info.output.node);
    }

    void patchbay::impl::on_node(std::uint32_t id)
    {
        auto &[info, ports] = nodes[id];

        if (speaker && info.props["node.name"] == speaker->name)
        {
            logger::get()->debug("[patchbay] (on_node) speakers are {}", id);
            speaker->id = id;
        }

        if (include.empty())
        {
            return;
        }

        auto match = [&](const auto &prop)
        {
            return info.props[prop.key] == prop.value;
        };

        if (ranges::any_of(exclude, match))
        {
            logger::get()->debug("[patchbay] (on_node) {} is excluded", id);
            return;
        }

        if (!ranges::any_of(include, match))
        {
            logger::get()->debug("[patchbay] (on_node) {} is not included", id);
            return;
        }

        if (ports.empty())
        {
            logger::get()->debug("[patchbay] (on_node) {} has no ports", id);
            return;
        }

        core->update();

        relink(id);
    }

    template <>
    void patchbay::impl::receive(cr_recipe::sender &sender, list_nodes &req)
    {
        static const std::vector<std::string> required{"application.name", "node.name"};
        const auto &props = req.props.empty() ? required : req.props;

        auto desireable = [&](auto &item)
        {
            return ranges::all_of(props, [&](const auto &key) { return !item.second.info.props[key].empty(); });
        };
        auto can_output = [](const auto &item)
        {
            return item.second.info.output.max > 0;
        };
        auto to_node = [&](auto &item)
        {
            return node{item.second.info.props};
        };

        core->update();

        auto filtered = nodes                               //
                        | ranges::views::filter(desireable) //
                        | ranges::views::filter(can_output);

        // Some nodes update their props (metadata) over time, and there is no pipewire event to catch this
        // (unless we have them bound constantly). Thus we re-bind them.

        for (auto &[id, node] : filtered)
        {
            auto updated    = registry->bind<pw::node>(id).get();
            node.info.props = updated->info().props;
        }

        auto rtn = filtered                            //
                   | ranges::views::transform(to_node) //
                   | ranges::to<std::vector>;

        sender.send(rtn);
    }

    template <>
    void patchbay::impl::receive([[maybe_unused]] cr_recipe::sender &, set_target &req)
    {
        if (!mic)
        {
            create_mic();
        }

        created.clear();

        include = std::move(req.include);
        exclude = std::move(req.exclude);

        for (const auto &[id, info] : nodes)
        {
            on_node(id);
        }

        for (const auto &[id, info] : links)
        {
            on_link(id);
        }
    }

    template <>
    void patchbay::impl::receive([[maybe_unused]] cr_recipe::sender &, [[maybe_unused]] unset_target &)
    {
        include.clear();
        exclude.clear();

        created.clear();

        mic.reset();
    }

    template <>
    void patchbay::impl::receive([[maybe_unused]] cr_recipe::sender &, [[maybe_unused]] quit &)
    {
        core->context()->loop()->quit();
    }

    void patchbay::impl::start(pw_recipe::receiver receiver, cr_recipe::sender sender)
    {
        auto loop    = pw::main_loop::create();
        auto context = pw::context::create(loop);

        core     = context ? context->core() : nullptr;
        registry = core ? core->registry() : nullptr;

        if (!core || !registry)
        {
            logger::get()->error("could not create core or registry");
            sender.send(ready{false});
            return;
        }

        receiver.attach(loop, [this, &sender]<typename T>(T message) { receive(sender, message); });

        auto listener = registry->listen();

        listener.on<pw::registry_event::global_removed>([this](std::uint32_t id) { rem_global(id); });
        listener.on<pw::registry_event::global>([this](const auto &global) { add_global(global); });

        sender.send(ready{});

        while (!should_exit)
        {
            loop->run();
        }
    }
} // namespace vencord
