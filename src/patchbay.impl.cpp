#include "patchbay.impl.hpp"
#include "logger.hpp"

#include <string_view>

#include <rohrkabel/device/device.hpp>
#include <rohrkabel/registry/events.hpp>
#include <rohrkabel/registry/registry.hpp>

#include <glaze/glaze.hpp>

namespace vencord
{
    using enum logger::level;
    using namespace std::chrono_literals;

    patchbay::impl::impl()
    {
        auto [pw_sender, pw_receiver] = pw::channel<pw_recipe>();
        auto [cr_sender, cr_receiver] = cr::channel<cr_recipe>();

        sender   = std::make_unique<pw_recipe::sender>(std::move(pw_sender));
        receiver = std::make_unique<cr_recipe::receiver>(std::move(cr_receiver));
        worker   = std::jthread{std::bind_front(&impl::start, this), std::move(pw_receiver), std::move(cr_sender)};

        if (!receiver->recv_as<ready>()->success)
        {
            throw std::runtime_error{"failed to create patchbay instance"};
        }

        logger::get()(debug, "[patchbay] (init) pw_receiver is ready");
    }

    patchbay::impl::~impl()
    {
        sender->send(quit{});
        receiver->try_recv_as<quit>(500ms);
    }

    void patchbay::impl::cleanup(clean kind)
    {
        options.reset();
        virt_links.clear();

        if (kind != clean::with_mic)
        {
            return;
        }

        virt_mic.reset();
    }

    coco::task<void> patchbay::impl::create_mic()
    {
        auto node = co_await core->create(pw::null_factory{
            .type      = pw::null_factory::kind::sink,
            .name      = "vencord-screen-share",
            .positions = {"FL", "FR"},
        });

        if (!node.has_value())
        {
            co_return logger::get()(error, "[patchbay] (create_mic) failed to create null-sink: {}", node.error().message);
        }

        const auto info = node->info();

        while (ports_of(info).size() < 4)
        {
            co_await core->sync();
        }

        virt_mic.emplace(std::move(*node));

        logger::get()("[patchbay] (create_mic) created null-sink: {}", virt_mic->id());
    }

    static bool matches(const std::vector<node> &targets, pw::spa::dict props) // NOLINT(*-anonymous-namespace)
    {
        const auto props_match = [&](const auto &prop)
        {
            return props[prop.first] == prop.second;
        };

        const auto has_target = [&](const auto &target)
        {
            return std::ranges::all_of(target, props_match);
        };

        return std::ranges::any_of(targets, has_target);
    }

    coco::task<void> patchbay::impl::redirect(std::optional<pw::node_info> info)
    {
        if (!options.has_value())
        {
            co_return;
        }

        if (!meta.has_value())
        {
            logger::get()(debug, "[patchbay] (redirect) metadata not available");
            co_return;
        }

        if (!virt_mic.has_value())
        {
            logger::get()(debug, "[patchbay] (redirect) virt-mic not available");
            co_return;
        }

        const auto pred = [this](const auto &item)
        {
            return matches(options->workaround, item.props);
        };

        if (!info.has_value())
        {
            const auto node = std::ranges::find_if(nodes, pred, [](auto &&item) { return item.second; });
            info            = (node == nodes.end() ? std::nullopt : std::optional<pw::node_info>{node->second});
        }
        else if (!pred(*info))
        {
            logger::get()(debug, "[patchbay] (redirect) {} did not match workaround criteria", info->id);
            co_return;
        }

        if (!info.has_value())
        {
            logger::get()(debug, "[patchbay] (redirect) could not find node matching redirect criteria");
            co_return;
        }

        const auto make = []<typename T>(auto id, T &&fn)
        {
            return std::shared_ptr<std::uint32_t>{new std::uint32_t{id}, [fn = std::forward<T>(fn)](auto *item)
                                                  {
                                                      fn(*item);
                                                      delete item;
                                                  }};
        };

        if (options->legacy_workaround)
        {
            logger::get()(debug, "[patchbay] (redirect) using legacy workaround for {}", info->id);
            workaround_target = make(info->id, [this](auto id) { virt_links.erase(id); });
            co_return link(virt_mic->info(), *info);
        }

        const auto serial  = virt_mic->info().props.at("object.serial");
        const auto cleanup = [this](auto id)
        {
            if (!meta.has_value())
            {
                return;
            }

            meta->value.clear_property(id, "node.target");
            meta->value.clear_property(id, "target.object");
        };

        workaround_target.reset();
        {
            meta->value.set_property(info->id, "node.target", "Spa:Id", std::format("{}", virt_mic->id()));
            meta->value.set_property(info->id, "target.object", "Spa:Id", serial);
        }
        workaround_target = make(info->id, cleanup);

        logger::get()(debug, "[patchbay] (redirect) redirected {}", info->id);
    }

    bool patchbay::impl::should_link(const pw::node_info &node)
    {
        if (!options.has_value())
        {
            return false;
        }

        logger::get()(debug, "[patchbay] (should_link) checking {}", node.id);

        auto props = node.props;

        if (props["node.description"].starts_with("venmic-loopback"))
        {
            logger::get()(debug, "[patchbay] (should_link) └ is virt-mic loopback", node.id);
            return false;
        }

        if (!options->include.empty() && !matches(options->include, node.props))
        {
            logger::get()(debug, "[patchbay] (should_link) └ did not match include criteria", node.id);
            return false;
        }

        if (matches(options->exclude, node.props))
        {
            logger::get()(debug, "[patchbay] (should_link) └ matched exclude criteria", node.id);
            return false;
        }

        auto ports = ports_of(node);

        if (ports.empty())
        {
            logger::get()(debug, "[patchbay] (should_link) └ has no ports", node.id);
            return false;
        }

        if (virt_mic.has_value() && virt_mic->id() == node.id)
        {
            logger::get()(debug, "[patchbay] (should_link) └ is the virt-mic", node.id);
            return false;
        }

        if (options->ignore_devices && !props["device.id"].empty())
        {
            logger::get()(debug, "[patchbay] (should_link) └ is a device", node.id);
            return false;
        }

        const auto outputs = [](const auto &item)
        {
            return item.second.input.node;
        };

        const auto exists = [this](const auto &id)
        {
            return nodes.contains(id);
        };

        const auto info = [this](const auto &id)
        {
            return nodes[id];
        };

        const auto links   = links_of(node);
        const auto targets = links                            //
                             | std::views::transform(outputs) //
                             | std::views::filter(exists)     //
                             | std::views::transform(info)    //
                             | std::ranges::to<std::vector>();

        const auto is_device = [](const auto &item)
        {
            return item.props.contains("device.id");
        };

        if (options->only_speakers && !std::ranges::any_of(targets, is_device))
        {
            logger::get()(debug, "[patchbay] (should_link) └ does not link to speakers", node.id);
            return false;
        }

        const auto speakers_known = default_speaker.has_value() && default_speaker->id.has_value();

        if (options->only_default_speakers && !speakers_known)
        {
            logger::get()(debug, "[patchbay] (should_link) └ default speakers are unknown", node.id);
            return false;
        }

        const auto extract_id = [](auto &&item)
        {
            return item.id;
        };

        if (options->only_default_speakers && !std::ranges::contains(targets, default_speaker->id, extract_id))
        {
            logger::get()(debug, "[patchbay] (should_link) └ does not link to default speakers", node.id);
            return false;
        }

        logger::get()(debug, "[patchbay] (should_link) └ accepted!");

        return true;
    }

    void patchbay::impl::link(const pw::node_info &from, const pw::node_info &to)
    {
        if (virt_links.contains(from.id))
        {
            virt_links.erase(from.id);
        }

        const auto capture  = std::format("venmic-loopback-capture-{}-{}", from.id, to.id);
        const auto playback = std::format("venmic-loopback-playback-{}-{}", from.id, to.id);

        auto loopback = context->load(pw::loopback_options{
            .capture =
                {
                    .name        = capture,
                    .description = capture,
                    .node        = from.id,
                },
            .playback =
                {
                    .name        = playback,
                    .description = playback,
                    .node        = to.id,
                },
        });

        if (!loopback.has_value())
        {
            return logger::get()(warn, "[patchbay] (link) failed to create loopback ({} -> {}): {}", from.id, to.id,
                                 loopback.error().message());
        }

        virt_links.emplace(from.id, std::move(*loopback));

        logger::get()(info, "[patchbay] (link) created loopback {} -> {}", from.id, to.id);
    }

    std::map<std::uint32_t, pw::port_info> patchbay::impl::ports_of(const pw::node_info &info)
    {
        const auto node   = std::format("{}", info.id);
        const auto filter = [node](const auto &item)
        {
            auto props = item.second.props;
            return props["node.id"] == node;
        };

        return ports                        //
               | std::views::filter(filter) //
               | std::ranges::to<std::map>();
    }

    std::map<std::uint32_t, pw::link_info> patchbay::impl::links_of(const pw::node_info &info)
    {
        const auto filter = [&info](const auto &item)
        {
            return item.second.input.node == info.id || item.second.output.node == info.id;
        };

        return links                        //
               | std::views::filter(filter) //
               | std::ranges::to<std::map>();
    }

    template <>
    coco::stray patchbay::impl::handle(pw::node node)
    {
        const auto id = node.id();
        auto info     = node.info();
        auto props    = info.props;

        logger::get()(debug, "[patchbay] (handle) new node: {}", id);
        logger::get()(debug, "[patchbay] (handle) └ node.name: {}", props["node.name"]);
        logger::get()(debug, "[patchbay] (handle) └ application.name: {}", props["application.name"]);
        logger::get()(debug, "[patchbay] (handle) └ application.process.binary: {}", props["application.process.binary"]);

        if (default_speaker.has_value() && default_speaker->name == props["node.name"])
        {
            default_speaker->id = id;
            logger::get()("[patchbay] (handle) found node for default speaker: {}", id);
        }

        if (virt_mic.has_value() && should_link(info))
        {
            link(info, virt_mic->info());
        }

        co_await redirect(info);

        nodes[id] = std::move(info);
    }

    template <>
    coco::stray patchbay::impl::handle(pw::port port)
    {
        const auto id = port.id();
        auto info     = port.info();
        auto props    = info.props;

        auto raw_parent = props["node.id"];
        auto parent     = std::uint32_t{};

        logger::get()(trace, "[patchbay] (handle) new port: {}", id);
        logger::get()(trace, "[patchbay] (handle) └ parent: {}", raw_parent);

        if (std::from_chars(raw_parent.data(), raw_parent.data() + raw_parent.size(), parent).ec != std::errc{})
        {
            co_return logger::get()(trace, "[patchbay] (handle) could not parse parent of {} (\"{}\")", id, raw_parent);
        }

        ports[id]       = std::move(info);
        const auto node = nodes.find(parent);

        if (node == nodes.end())
        {
            co_return;
        }

        if (virt_mic.has_value() && should_link(node->second))
        {
            link(node->second, virt_mic->info());
        }

        co_await redirect(node->second);
    }

    template <>
    coco::stray patchbay::impl::handle(pw::link link)
    {
        const auto id = link.id();
        auto info     = link.info();
        auto props    = info.props;

        logger::get()(trace, "[patchbay] (handle) new link: {}", id);
        logger::get()(trace, "[patchbay] (handle) └ from: {} (port: {})", info.output.node, info.output.port);
        logger::get()(trace, "[patchbay] (handle) └ to: {} (port: {})", info.input.node, info.input.port);

        const auto from = info.output.node;
        const auto to   = info.input.node;

        links[id] = std::move(info);

        if (!virt_mic.has_value())
        {
            co_return;
        }

        if (virt_links.contains(from) || virt_links.contains(to))
        {
            co_return;
        }

        if (const auto it = nodes.find(from); it != nodes.end() && should_link(it->second))
        {
            this->link(it->second, virt_mic->info());
        }

        if (const auto it = nodes.find(to); it != nodes.end() && should_link(it->second))
        {
            this->link(it->second, virt_mic->info());
        }

        logger::get()(debug, "[patchbay] (handle) refreshed nodes ({} -> {}) attached to {}", from, to, id);
    }

    struct pw_metadata_name // NOLINT(*-internal-linkage)
    {
        std::string name;
    };

    template <>
    coco::stray patchbay::impl::handle(pw::metadata metadata)
    {
        auto info  = metadata.properties();
        auto props = metadata.props();

        const auto id   = metadata.id();
        const auto name = props["metadata.name"];

        logger::get()(debug, "[patchbay] (handle) new metadata: {}", id);
        logger::get()(debug, "[patchbay] (handle) └ name: {}", name);

        if (name != "default")
        {
            co_return;
        }

        auto *const raw = metadata.get();
        meta.emplace(std::move(metadata), raw);

        const auto update = [this](const char *raw, pw::metadata_property prop)
        {
            if (!raw)
            {
                return 0;
            }

            const auto key = std::string_view{raw};

            if (key != "default.audio.sink")
            {
                return 0;
            }

            const auto parsed = glz::read_json<pw_metadata_name>(prop.value);

            if (!parsed.has_value())
            {
                logger::get()(warn, "[patchbay] (meta) failed to parse speaker");
                return 0;
            }

            const auto node = std::ranges::find_if(nodes,
                                                   [&](const auto &info)
                                                   {
                                                       auto props = info.second.props;
                                                       return props["node.name"] == parsed->name;
                                                   });

            default_speaker = speaker{
                .name = parsed->name,
            };

            if (node != nodes.end())
            {
                default_speaker->id = node->first;
            }

            logger::get()("[patchbay] (meta) found default speaker: {}", parsed->name);
            logger::get()("[patchbay] (meta) └ node: {}", node == nodes.end() ? "<pending>" : std::to_string(node->first));

            return 0;
        };

        meta->listener.on<pw::metadata_event::property>(update);
        update("default.audio.sink", info["default.audio.sink"]);

        co_return co_await redirect();
    }

    void patchbay::impl::add_global(pw::global global)
    {
        const auto forward = []<typename T>(auto self, auto global, std::type_identity<T>) -> coco::stray
        {
            auto bound = co_await self->registry->template bind<T>(global.id);

            if (bound.has_value())
            {
                self->handle(std::move(*bound));
                co_return;
            }

            logger::get()(warn, "[patchbay] (add_global) failed to bind {}: {}", global.id, bound.error().message);
        };

        logger::get()(trace, "[patchbay] (add_global) new global {}: {}", global.id, global.type);

        if (global.type == pw::node::type)
        {
            forward(this, std::move(global), std::type_identity<pw::node>{});
        }
        else if (global.type == pw::port::type)
        {
            forward(this, std::move(global), std::type_identity<pw::port>{});
        }
        else if (global.type == pw::link::type)
        {
            forward(this, std::move(global), std::type_identity<pw::link>{});
        }
        else if (global.type == pw::metadata::type)
        {
            forward(this, std::move(global), std::type_identity<pw::metadata>{});
        }
    }

    void patchbay::impl::del_global(std::uint32_t id)
    {
        virt_links.erase(id);

        nodes.erase(id);
        ports.erase(id);
        links.erase(id);

        logger::get()(trace, "[patchbay] (del_global) removed global {}", id);
    }

    template <>
    coco::stray patchbay::impl::receive(cr_recipe::sender, link_options opts)
    {
        if (!virt_mic.has_value())
        {
            co_await create_mic();
        }

        options.emplace(std::move(opts));
        cleanup(clean::without_mic);

        co_await virt_mic->sync();

        const auto targets = nodes                                                           //
                             | std::views::values                                            //
                             | std::views::filter(std::bind_front(&impl::should_link, this)) //
                             | std::ranges::to<std::vector>();

        for (const auto &node : targets)
        {
            link(node, virt_mic->info());
        }

        co_await redirect();
    }

    template <>
    coco::stray patchbay::impl::receive(cr_recipe::sender, unset_target)
    {
        co_return cleanup(clean::with_mic);
    }

    template <>
    coco::stray patchbay::impl::receive(cr_recipe::sender, quit)
    {
        default_speaker.reset();
        cleanup(clean::with_mic);

        co_return loop->quit();
    }

    template <>
    coco::stray patchbay::impl::receive(cr_recipe::sender sender, list_nodes req)
    {
        using clock = std::chrono::system_clock;

        const auto desireable = [&req](const auto &item)
        {
            auto other = item.second.props;
            return std::ranges::all_of(req.props, [&](const auto &key) { return !other[key].empty(); });
        };

        const auto can_output = [](const auto &item)
        {
            return item.second.output.max > 0;
        };

        logger::get()(debug, "[patchbay] (receive) listing nodes ({})", req.props);

        for (const auto start = clock::now(); clock::now() - start < 500ms && nodes.empty();)
        {
            logger::get()(debug, "[patchbay] (receive) no nodes available, syncing...");
            co_await core->sync();
        }

        const auto filtered = nodes                                    //
                              | std::ranges::views::filter(desireable) //
                              | std::ranges::views::filter(can_output) //
                              | std::ranges::to<std::vector>();

        logger::get()(debug, "[patchbay] (receive) found {} nodes", filtered.size());

        auto rtn = std::vector<node>{};

        for (const auto &[id, info] : filtered)
        {
            const auto bound = co_await registry->bind<pw::node>(id);

            if (!bound.has_value())
            {
                logger::get()(warn, "[patchbay] (receive) failed to refresh {}: {}", id, bound.error().message);
                continue;
            }

            rtn.emplace_back(bound->info().props);
        }

        sender.send(rtn);
    }

    void patchbay::impl::start(pw_recipe::receiver receiver, cr_recipe::sender sender)
    {
        const auto bail = [&sender]
        {
            sender.send(ready{false});
        };

        if (auto result = pw::main_loop::create())
        {
            loop = std::move(*result);
        }
        else
        {
            logger::get()(error, "could not create main_loop: {}", result.error().message());
            return bail();
        }

        if (auto result = pw::context::create(loop))
        {
            context = std::move(*result);
        }
        else
        {
            logger::get()(error, "could not create context: {}", result.error().message());
            return bail();
        }

        if (auto result = pw::core::create(context))
        {
            core = std::move(*result);
        }
        else
        {
            logger::get()(error, "could not create core: {}", result.error().message());
            return bail();
        }

        if (auto result = pw::registry::create(core))
        {
            registry = std::move(*result);
        }
        else
        {
            logger::get()(error, "could not create registry: {}", result.error().message());
            return bail();
        }

        const auto callback = [this, &sender]<typename T>(T message)
        {
            logger::get()(trace, "[patchbay] received message {}", glz::type_name<T>);
            logger::get()(trace, "[patchbay] └ with content: {}", glz::write_json(message).value_or(""));
            receive(sender, std::move(message));
        };
        receiver.attach(loop, callback);

        auto listener = registry->listen();

        listener.on<pw::registry_event::global>(std::bind_front(&impl::add_global, this));
        listener.on<pw::registry_event::global_removed>(std::bind_front(&impl::del_global, this));

        sender.send(ready{true});
        loop->run();

        sender.send(quit{});
    }
} // namespace vencord
