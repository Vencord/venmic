#include "audio.impl.hpp"
#include "meta.hpp"

#include <stdexcept>

#include <range/v3/view.hpp>
#include <range/v3/range.hpp>

#include <range/v3/action.hpp>
#include <range/v3/algorithm.hpp>

namespace vencord
{
    audio::impl::~impl()
    {
        should_exit = true;
        sender->send(quit{});
    }

    audio::impl::impl()
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

        throw std::runtime_error("Failed to create audio instance");
    }

    void audio::impl::create_mic()
    {
        auto node = core->create<pw::node>({"adapter",
                                            {{"audio.channels", "2"},
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

    void audio::impl::relink(std::uint32_t id)
    {
        created.erase(id);

        if (!nodes.contains(id))
        {
            return;
        }

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

        for (auto &port : target_ports)
        {
            auto matching_channel = [&](auto &item)
            {
                if (target_ports.size() == 1)
                {
                    return true;
                }

                return item.props["audio.channel"] == port.props["audio.channel"];
            };

            auto others = source_ports | ranges::views::filter(matching_channel);

            for (auto &other : others)
            {
                auto link = core->create<pw::link>(pw::link_factory{
                                                       other.id,
                                                       port.id,
                                                   })
                                .get();

                if (!link.has_value())
                {
                    // NOLINTNEXTLINE
                    fprintf(stderr, "[venmic] Failed to create link: %s\n", link.error().message);

                    return;
                }

                created.emplace(id, std::move(*link));
            }
        }
    }

    void audio::impl::global_removed(std::uint32_t id)
    {
        nodes.erase(id);
        created.erase(id);
    }

    void audio::impl::global_added(const pw::global &global)
    {
        if (global.type == pw::node::type)
        {
            auto node = registry->bind<pw::node>(global.id).get();

            if (!node.has_value())
            {
                return;
            }

            nodes[global.id].info = node->info();

            if (!speaker)
            {
                return;
            }

            if (node->info().props["node.name"] != speaker->name)
            {
                return;
            }

            speaker->id = global.id;

            return;
        }

        if (global.type == pw::metadata::type)
        {
            auto metadata = registry->bind<pw::metadata>(global.id).get();

            if (!metadata.has_value())
            {
                return;
            }

            auto props = metadata->properties();

            if (!props.contains("default.audio.sink"))
            {
                return;
            }

            auto parsed = glz::read_json<pw_metadata_name>(props["default.audio.sink"].value);

            if (!parsed.has_value())
            {
                return;
            }

            speaker = {parsed->name};

            auto node =
                ranges::find_if(nodes, [&](auto &item) { return item.second.info.props["node.name"] == parsed->name; });

            if (node == nodes.end())
            {
                return;
            }

            speaker->id = node->first;

            return;
        }

        if (global.type == pw::port::type)
        {
            auto port = registry->bind<pw::port>(global.id).get();

            if (!port.has_value())
            {
                return;
            }

            auto props = port->info().props;

            if (!props.contains("node.id"))
            {
                return;
            }

            auto parent = std::stoull(props["node.id"]);
            nodes[parent].ports.emplace_back(port->info());

            //? Yes this belongs here, as the node is created **before** the ports.
            on_node(parent);
        }

        if (global.type == pw::link::type)
        {
            auto link = registry->bind<pw::link>(global.id).get();

            if (!link.has_value())
            {
                return;
            }

            links[global.id] = link->info();

            on_link(global.id);
        }
    }

    void audio::impl::on_link(std::uint32_t id)
    {
        if (!target || !speaker)
        {
            return;
        }

        if (target->mode != target_mode::exclude)
        {
            return;
        }

        auto &info = links[id];

        if (info.input.node != speaker->id)
        {
            return;
        }

        auto &output = nodes[info.output.node]; // "Output" = the node that is emitting sound

        if (output.info.props["node.name"] == target->name)
        {
            return;
        }

        core->update();

        relink(info.output.node);
    }

    void audio::impl::on_node(std::uint32_t parent)
    {
        if (!target)
        {
            return;
        }

        if (target->mode != target_mode::include)
        {
            return;
        }

        if (nodes[parent].info.props["node.name"] != target->name)
        {
            return;
        }

        if (nodes[parent].ports.empty())
        {
            return;
        }

        core->update();

        relink(parent);
    }

    template <>
    void audio::impl::receive(cr_recipe::sender sender, [[maybe_unused]] list_nodes)
    {
        auto has_name = [&](auto &item)
        {
            return item.second.info.props.contains("application.name") && item.second.info.props.contains("node.name");
        };
        auto can_output = [](const auto &item)
        {
            return item.second.info.output.max > 0;
        };
        auto get_name = [](auto &item)
        {
            return item.second.info.props["node.name"];
        };

        auto rtn = nodes                                //
                   | ranges::views::filter(has_name)    //
                   | ranges::views::filter(can_output)  //
                   | ranges::views::transform(get_name) //
                   | ranges::to<std::set>;

        sender.send(rtn);
    }

    template <>
    // NOLINTNEXTLINE(*-value-param)
    void audio::impl::receive([[maybe_unused]] cr_recipe::sender, vencord::target req)
    {
        created.clear();
        target.emplace(std::move(req));

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
    // NOLINTNEXTLINE(*-value-param)
    void audio::impl::receive([[maybe_unused]] cr_recipe::sender, [[maybe_unused]] unset_target)
    {
        target.reset();
        created.clear();
    }

    template <>
    // NOLINTNEXTLINE(*-value-param)
    void audio::impl::receive([[maybe_unused]] cr_recipe::sender, [[maybe_unused]] quit)
    {
        core->context()->loop()->quit();
    }

    void audio::impl::start(pw_recipe::receiver receiver, cr_recipe::sender sender)
    {
        auto loop    = pw::main_loop::create();
        auto context = pw::context::create(loop);

        core     = context ? context->core() : nullptr;
        registry = core ? core->registry() : nullptr;

        if (!core || !registry)
        {
            sender.send(ready{false});
            return;
        }

        receiver.attach(loop, [this, sender]<typename T>(T &&message) { receive(sender, std::forward<T>(message)); });

        auto listener = registry->listen();
        listener.on<pw::registry_event::global_removed>([this](std::uint32_t id) { global_removed(id); });
        listener.on<pw::registry_event::global>([this](const auto &global) { global_added(global); });

        create_mic();

        sender.send(ready{});

        while (!should_exit)
        {
            loop->run();
        }
    }
} // namespace vencord
