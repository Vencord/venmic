#include "patchbay.impl.hpp"

#include "logger.hpp"

#include <stdexcept>

#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

#include <glaze/glaze.hpp>
#include <rohrkabel/device/device.hpp>

namespace vencord
{
    struct pw_metadata_name
    {
        std::string name;
    };

    patchbay::impl::~impl()
    {
        cleanup(true);

        should_exit = true;
        sender->send(quit{});
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

        auto response = receiver->recv_timeout_as<ready>(std::chrono::seconds(1));

        if (!response.has_value())
        {
            sender->send(abort{});
            response = receiver->recv_as<ready>();
        }

        if (response->success)
        {
            logger::get()->trace("[patchbay] (init) pw_receiver is ready");
            return;
        }

        throw std::runtime_error("Failed to create patchbay instance");
    }

    void patchbay::impl::create_mic()
    {
        logger::get()->trace("[patchbay] (create_mic) creating virt-mic");

        auto node = core->create<pw::node>(pw::null_sink_factory{
                                               .name      = "vencord-screen-share",
                                               .positions = {"FL", "FR"},
                                           })
                        .get();

        while (nodes[node->id()].ports.size() < 4)
        {
            core->update();
        }

        virt_mic = std::make_unique<pw::node>(std::move(*node));

        logger::get()->info("[patchbay] (create_mic) created: {}", virt_mic->id());
    }

    void patchbay::impl::cleanup(bool mic)
    {
        created.clear();

        if (metadata && lettuce_target)
        {
            metadata->clear_property(lettuce_target.value(), "target.node");
            metadata->clear_property(lettuce_target.value(), "target.object");
        }

        if (!mic)
        {
            return;
        }

        virt_mic.reset();
    }

    void patchbay::impl::reload()
    {
        cleanup(false);

        for (const auto &[id, info] : nodes)
        {
            on_node(id);
        }

        for (const auto &[id, info] : links)
        {
            on_link(id);
        }

        logger::get()->debug("[patchbay] (reload) finished");
    }

    void patchbay::impl::link(std::uint32_t id)
    {
        if (!virt_mic)
        {
            return;
        }

        const auto mic_id = virt_mic->id();

        if (id == mic_id)
        {
            logger::get()->warn("[patchbay] (link) prevented link to self");
            return;
        }

        if (!nodes.contains(id))
        {
            logger::get()->warn("[patchbay] (link) called with bad node: {}", id);
            return;
        }

        auto &target = nodes[id];
        auto &props  = target.info.props;

        if (options.ignore_devices && !props["device.id"].empty())
        {
            logger::get()->warn("[patchbay] (link) prevented link to device: {}", id);
            return;
        }

        if (options.ignore_input_media && props["media.class"].find("Input") != std::string::npos)
        {
            logger::get()->warn("[patchbay] (link) prevented link to node with input class: {} (\"{}\")", id,
                                props["media.class"]);
            return;
        }

        logger::get()->debug("[patchbay] (link) linking {}", id);

        auto mapping = map_ports(target);

        for (auto [it, end] = created.equal_range(id); it != end; ++it)
        {
            auto equal = [info = it->second.info()](const auto &map)
            {
                return map.first.id == info.input.port && map.second.id == info.output.port;
            };

            std::erase_if(mapping, equal);
        }

        for (auto [mic_port, target_port] : mapping)
        {
            auto link = core->create<pw::link>(pw::link_factory{
                                                   mic_port.id,
                                                   target_port.id,
                                               })
                            .get();

            if (!link.has_value())
            {
                logger::get()->warn("[patchbay] (link) failed to link {} (mic) -> {} (node): {}", mic_port.id,
                                    target_port.id, link.error().message);

                return;
            }

            logger::get()->debug("[patchbay] (link) created {}: {} (mic) -> {} (node) (channel: {})", link->id(),
                                 mic_port.id, target_port.id, target_port.props["audio.channel"]);

            created.emplace(id, std::move(*link));
        }

        logger::get()->debug("[patchbay] (link) linked all ports of {}", id);
    }

    port_map patchbay::impl::map_ports(node_with_ports &target)
    {
        port_map rtn;

        auto is_output = [](const auto &item)
        {
            return item.direction == pw::port_direction::output;
        };
        auto is_input = [](const auto &item)
        {
            return item.direction == pw::port_direction::input;
        };

        auto &mic       = nodes[virt_mic->id()];
        auto mic_inputs = mic.ports | ranges::views::filter(is_input);

        const auto id       = target.info.id;
        auto target_outputs = target.ports | ranges::views::filter(is_output) | ranges::to<std::vector>;

        if (target_outputs.empty())
        {
            logger::get()->warn("[patchbay] (map_ports) {} has no ports", id);
            return rtn;
        }

        const auto is_mono = target_outputs.size() == 1;

        if (is_mono)
        {
            logger::get()->debug("[patchbay] (map_ports) {} is mono", id);
        }

        for (auto &port : target_outputs)
        {
            auto matching_channel = [is_mono, &port](auto &item)
            {
                if (is_mono)
                {
                    return true;
                }

                if (item.props["audio.channel"] == "UNK" || port.props["audio.channel"] == "UNK")
                {
                    return item.props["port.id"] == port.props["port.id"];
                }

                return item.props["audio.channel"] == port.props["audio.channel"];
            };

            auto mapping =
                mic_inputs                                                                                          //
                | ranges::views::filter(matching_channel)                                                           //
                | ranges::views::transform([port](const auto &mic_port) { return std::make_pair(mic_port, port); }) //
                | ranges::to<std::vector>;

            rtn.insert(rtn.end(), mapping.begin(), mapping.end());

            logger::get()->debug("[patchbay] (map_ports) {} maps to {} mic port(s)", port.id, mapping.size());
        }

        return rtn;
    }

    void patchbay::impl::meta_update(std::string_view key, pw::metadata_property prop)
    {
        logger::get()->debug(R"([patchbay] (meta_update) metadata property changed: "{}" (value: "{}"))", key,
                             prop.value);

        if (key != "default.audio.sink")
        {
            return;
        }

        auto parsed = glz::read_json<pw_metadata_name>(prop.value);

        if (!parsed.has_value())
        {
            logger::get()->warn("[patchbay] (meta_update) failed to parse speaker");
            return;
        }

        speaker.emplace(parsed->name);
        logger::get()->info(R"([patchbay] (meta_update) speaker name: "{}")", speaker->name);

        reload();
    }

    void patchbay::impl::on_link(std::uint32_t id)
    {
        if (!options.include.empty() || (options.only_default_speakers && !speaker))
        {
            return;
        }

        auto &info = links[id];

        const auto output_id = info.output.node;
        const auto input_id  = info.input.node;

        if (options.only_default_speakers && input_id != speaker->id)
        {
            logger::get()->trace("[patchbay] (on_link) {} is not connected to speaker but with {}", id, input_id);
            return;
        }

        auto &output = nodes[output_id]; // The node emitting sound
        auto &input  = nodes[input_id];  // The node receiving sound

        if (!options.only_default_speakers && input.info.props["device.id"].empty())
        {
            logger::get()->trace("[patchbay] (on_link) {} is not playing to a device: {}", id, input_id);
            return;
        }

        auto match = [&output](const auto &prop)
        {
            return output.info.props[prop.key] == prop.value;
        };

        if (ranges::any_of(options.exclude, match))
        {
            return;
        }

        core->update();

        link(output_id);
    }

    void patchbay::impl::on_node(std::uint32_t id)
    {
        auto &[info, ports] = nodes[id];

        if (speaker && info.props["node.name"] == speaker->name)
        {
            logger::get()->debug("[patchbay] (on_node) speakers are {}", id);
            speaker->id = id;

            return;
        }

        if (options.include.empty())
        {
            return;
        }

        if (ports.empty())
        {
            logger::get()->debug("[patchbay] (on_node) {} has no ports", id);
            return;
        }

        auto match = [&info](const auto &prop)
        {
            return info.props[prop.key] == prop.value;
        };

        if (ranges::any_of(options.exclude, match))
        {
            logger::get()->debug("[patchbay] (on_node) {} is excluded", id);
            return;
        }

        if (!ranges::any_of(options.include, match))
        {
            logger::get()->debug("[patchbay] (on_node) {} is not included", id);
            return;
        }

        created.erase(id);
        core->update();

        link(id);
    }

    template <>
    void patchbay::impl::handle<pw::node>(pw::node &node, pw::global &global)
    {
        auto id    = global.id;
        auto props = node.info().props;

        nodes[id].info = node.info();

        logger::get()->trace(R"([patchbay] (handle) new node: {} (name: "{}", app: "{}"))", id, props["node.name"],
                             props["application.name"]);

        if (!virt_mic)
        {
            return;
        }

        if (!metadata || options.workaround.empty())
        {
            return on_node(id);
        }

        logger::get()->trace("[patchbay] (handle) workaround is active ({})", glz::write_json(options.workaround));

        auto match = [&props](const auto &prop)
        {
            return props[prop.key] == prop.value;
        };

        if (!ranges::all_of(options.workaround, match))
        {
            return on_node(id);
        }

        const auto serial = virt_mic->info().props["object.serial"];
        logger::get()->debug("[patchbay] (handle) applying workaround to {} (serial = {})", id, serial);

        // https://github.com/Vencord/venmic/issues/13#issuecomment-1884975782

        metadata->set_property(id, "target.object", "Spa:Id", serial);
        metadata->set_property(id, "target.node", "Spa:Id", std::to_string(virt_mic->id()));

        lettuce_target.emplace(id);
        options.workaround.clear();

        core->update();
    }

    template <>
    void patchbay::impl::handle<pw::link>(pw::link &link, pw::global &global)
    {
        auto id   = global.id;
        auto info = link.info();

        links[id] = info;

        logger::get()->trace(
            "[patchbay] (handle) new link: {} (input-node: {}, output-node: {}, input-port: {}, output-port: {})", id,
            info.input.node, info.output.node, info.input.port, info.output.port);

        on_link(id);
    }

    template <>
    void patchbay::impl::handle<pw::port>(pw::port &port, pw::global &global)
    {
        auto info  = port.info();
        auto props = info.props;

        if (!props.contains("node.id"))
        {
            logger::get()->warn("[patchbay] (handle) {} has no parent", global.id);
            return;
        }

        auto parent = std::stoull(props["node.id"]);
        nodes[parent].ports.emplace_back(std::move(info));

        logger::get()->trace("[patchbay] (handle) new port: {} with parent {}", global.id, parent);

        // Check the parent again
        on_node(parent);
    }

    template <>
    void patchbay::impl::handle<pw::metadata>(pw::metadata &data, pw::global &global)
    {
        auto props      = data.properties();
        const auto name = global.props["metadata.name"];

        logger::get()->trace(R"([patchbay] (handle) new metadata: {} (name: "{}"))", global.id, name);

        if (name != "default")
        {
            return;
        }

        metadata      = std::make_unique<pw::metadata>(std::move(data));
        meta_listener = std::make_unique<pw::metadata_listener>(metadata->listen());

        logger::get()->info("[patchbay] (handle) found default metadata: {}", global.id);

        meta_listener->on<pw::metadata_event::property>(
            [this](const char *key, auto property)
            {
                if (key)
                {
                    meta_update(key, std::move(property));
                }

                return 0;
            });

        meta_update("default.audio.sink", props["default.audio.sink"]);
    }

    template <typename T>
    void patchbay::impl::bind(pw::global &global)
    {
        auto bound = registry->bind<T>(global.id).get();

        if (!bound.has_value())
        {
            logger::get()->warn("[patchbay] (bind) failed to bind {} (\"{}\"): {}", global.id, global.type,
                                bound.error().message);
            return;
        }

        handle(bound.value(), global);
    }

    void patchbay::impl::add_global(pw::global &global)
    {
        logger::get()->trace(R"([patchbay] (add_global) new global: {} (type: "{}"))", global.id, global.type);

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

    void patchbay::impl::del_global(std::uint32_t id)
    {
        nodes.erase(id);
        links.erase(id);
        created.erase(id);
    }

    template <>
    void patchbay::impl::receive(cr_recipe::sender &sender, list_nodes &req)
    {
        static const std::vector<std::string> required{"application.name", "node.name"};
        const auto &props = req.props.empty() ? required : req.props;

        auto desireable = [&props](auto &item)
        {
            return ranges::all_of(props, [&](const auto &key) { return !item.second.info.props[key].empty(); });
        };
        auto can_output = [](const auto &item)
        {
            return item.second.info.output.max > 0;
        };
        auto to_node = [](auto &item)
        {
            return node{item.second.info.props};
        };

        logger::get()->trace("[patchbay] (receive): listing nodes ({{{}}})", fmt::join(req.props, ","));

        core->update();

        auto filtered = nodes                               //
                        | ranges::views::filter(desireable) //
                        | ranges::views::filter(can_output) //
                        | ranges::to<std::vector>;

        logger::get()->trace("[patchbay] (receive): found {} nodes", filtered.size());

        /*
         * Some nodes update their props (metadata) over time, and to avoid binding the node constantly,
         * we simply rebind it to fetch the updates only when needed.
         */

        for (auto &[id, node] : filtered)
        {
            logger::get()->trace("[patchbay] (receive): rebinding {}", id);
            auto updated = registry->bind<pw::node>(id).get();

            if (!updated.has_value())
            {
                const auto error = updated.error();
                logger::get()->warn(R"([patchbay] (receive) failed to rebind {}: "{}")", id, error.message);

                continue;
            }

            node.info.props = updated->info().props;
        }

        auto rtn = filtered                            //
                   | ranges::views::transform(to_node) //
                   | ranges::to<std::vector>;

        sender.send(rtn);
    }

    template <>
    void patchbay::impl::receive([[maybe_unused]] cr_recipe::sender &, link_options &req)
    {
        if (!virt_mic)
        {
            create_mic();
        }

        options = std::move(req);

        reload();
    }

    template <>
    void patchbay::impl::receive([[maybe_unused]] cr_recipe::sender &, [[maybe_unused]] unset_target &)
    {
        cleanup(true);
    }

    template <>
    void patchbay::impl::receive([[maybe_unused]] cr_recipe::sender &, [[maybe_unused]] quit &)
    {
        core->context()->loop()->quit();
    }

    template <>
    void patchbay::impl::receive([[maybe_unused]] cr_recipe::sender &, [[maybe_unused]] abort &)
    {
        should_exit = true;

        update_source.request_stop();
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

        receiver.attach(loop,
                        [this, &sender]<typename T>(T message)
                        {
                            logger::get()->trace("[patchbay] (main_loop) received message: {}", glz::type_name<T>);
                            receive(sender, message);
                        });

        auto listener = registry->listen();

        listener.on<pw::registry_event::global_removed>([this](std::uint32_t id) { del_global(id); });
        listener.on<pw::registry_event::global>([this](auto global) { add_global(global); });

        auto future   = core->update();
        update_source = future.stop_source();

        auto success = future.get();
        sender.send(ready{success.value_or(false)});

        while (!should_exit)
        {
            loop->run();
        }

        logger::get()->trace("[patchbay] (main_loop) finished");
    }
} // namespace vencord
