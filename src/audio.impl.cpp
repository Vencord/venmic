#include "audio.impl.hpp"

#include <range/v3/view.hpp>
#include <range/v3/range.hpp>
#include <range/v3/algorithm/find_if.hpp>

namespace vencord
{
    audio::impl::~impl()
    {
        should_exit = true;
        sender->send(quit{});

        thread.join();
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

        thread = std::thread{thread_start, std::move(pw_receiver), std::move(cr_sender)};
        receiver->recv_as<ready>();
    }

    void audio::impl::create_mic()
    {
        auto node = core->create<pw::node>({"adapter",
                                            {{"audio.channels", "2"},
                                             {"node.name", "Vesktop Screen-Share"},
                                             {"media.class", "Audio/Source/Virtual"},
                                             {"factory.name", "support.null-audio-sink"}}})
                        .get();

        while (nodes[node->id()].ports.size() < 4)
        {
            core->update();
        }

        mic = std::make_unique<pw::node>(std::move(*node));
    }

    void audio::impl::relink()
    {
        links.clear();

        if (!target)
        {
            return;
        }

        auto id = target->second;

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

                links.emplace_back(std::move(*link));
            }
        }
    }

    void audio::impl::global_removed(std::uint32_t id)
    {
        nodes.erase(id);
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

            if (!target)
            {
                return;
            }

            if (node->info().props["node.name"] != target->first)
            {
                return;
            }

            target->second = global.id;

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
        }

        if (!target)
        {
            return;
        }

        if (global.type != pw::port::type)
        {
            return;
        }

        auto &node = nodes[target->second];

        if (node.ports.empty())
        {
            return;
        }

        core->update();

        relink();
    }

    template <>
    void audio::impl::receive(cr_recipe::sender sender, [[maybe_unused]] list_nodes)
    {
        auto has_name = [](const auto &item)
        {
            return item.second.info.props.contains("node.name");
        };
        auto can_output = [](const auto &item)
        {
            return item.second.info.output.max > 0;
        };
        auto to_node = [](const auto &item)
        {
            return node{item.second.info.props.at("node.name"), item.first};
        };

        sender.send(nodes                               //
                    | ranges::views::filter(has_name)   //
                    | ranges::views::filter(can_output) //
                    | ranges::views::transform(to_node) //
                    | ranges::to<std::vector>           //
        );
    }

    template <>
    // NOLINTNEXTLINE(*-value-param)
    void audio::impl::receive([[maybe_unused]] cr_recipe::sender, set_target message)
    {
        auto name = message.id;
        auto node = ranges::find_if(nodes, [&](auto &item) { return item.second.info.props["node.name"] == name; });

        if (node == nodes.end())
        {
            return;
        }

        target.emplace(name, node->first);
        relink();
    }

    template <>
    // NOLINTNEXTLINE(*-value-param)
    void audio::impl::receive([[maybe_unused]] cr_recipe::sender, [[maybe_unused]] unset_target)
    {
        target.reset();
        links.clear();
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

        core     = context->core();
        registry = core->registry();

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