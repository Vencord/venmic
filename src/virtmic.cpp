/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2023  Malte Jürgens and discord-screenaudio contributors
 * Copyright (c) 2023  Vendicated and Vencord contributors
 */

#include <virtmic.hpp>

#include <thread>
#include <rohrkabel/loop/main.hpp>
#include <rohrkabel/registry/registry.hpp>

std::atomic_bool virtMicRunning = false;
std::thread virtMicThread;

void startVirtMic(std::string target)
{
    stopVirtMic();

    virtMicRunning = true;
    virtMicThread = std::thread(start, target);

    printf("vencord-virtmic: Started virtmic %s\n", target.c_str());
}

void stopVirtMic()
{
    if (virtMicRunning)
    {
        printf("vencord-virtmic: Stopping virtmic\n");
        virtMicRunning = false;
        virtMicThread.join();
    }
}

bool shouldIgnore(std::string name)
{
    return name == "Chromium input";
}

const std::string nullstr = "";
const std::string &getTarget(const pipewire::spa::dict &props)
{
    if (props.count("media.class") &&
        props.at("media.class") == "Stream/Output/Audio")
    {
        if (props.count("application.name") && props.at("application.name") != "")
            return props.at("application.name");
        else if (props.count("application.process.binary") &&
                 props.at("application.process.binary") != "")
            return props.at("application.process.binary");
        else
            return props.at("node.name");
    }
    else
        return nullstr;
}

std::vector<std::string> getTargets()
{
    auto main_loop = pipewire::main_loop();
    auto context = pipewire::context(main_loop);
    auto core = pipewire::core(context);
    auto reg = pipewire::registry(core);

    std::vector<std::string> targets;

    auto reg_listener = reg.listen<pipewire::registry_listener>();
    reg_listener.on<pipewire::registry_event::global>(
        [&](const pipewire::global &global)
        {
            if (global.type == pipewire::node::type)
            {
                auto node = reg.bind<pipewire::node>(global.id);
                auto info = node.info();
                auto name = getTarget(info.props);
                if (name != "" && !shouldIgnore(name) &&
                    std::find(targets.begin(), targets.end(), name) == targets.end())
                {
                    targets.push_back(name);
                }
            }
        });
    core.update();

    return targets;
};

void start(std::string target)
{
    std::map<std::uint32_t, pipewire::port> ports;
    std::unique_ptr<pipewire::port> virt_fl, virt_fr;

    std::map<std::uint32_t, pipewire::node_info> nodes;
    std::map<std::uint32_t, pipewire::link_factory> links;

    auto main_loop = pipewire::main_loop();
    auto context = pipewire::context(main_loop);
    auto core = pipewire::core(context);
    auto reg = pipewire::registry(core);

    auto link = [&](const std::string &target, pipewire::core &core)
    {
        for (const auto &[port_id, port] : ports)
        {
            if (!virt_fl || !virt_fr)
                continue;

            if (links.count(port_id))
                continue;

            if (port.info().direction == pipewire::port_direction::input)
                continue;

            if (!port.info().props.count("node.id"))
                continue;

            auto parent_id = std::stoul(port.info().props["node.id"]);

            if (!nodes.count(parent_id))
                continue;

            auto &parent = nodes.at(parent_id);
            std::string name = getTarget(parent.props);
            if (name == target ||
                (target == "[All Desktop Audio]" &&
                 !shouldIgnore(name)))
            {
                auto fl = port.info().props["audio.channel"] == "FL";
                links.emplace(
                    port_id,
                    core.create<pipewire::link_factory>(
                        {fl ? virt_fl->info().id : virt_fr->info().id, port_id}));
            }
        }
    };

    auto virtual_mic = core.create("adapter",
                                   {{"node.name", "vencord-virtmic"},
                                    {"media.class", "Audio/Source/Virtual"},
                                    {"factory.name", "support.null-audio-sink"},
                                    {"audio.channels", "2"},
                                    {"audio.position", "FL,FR"}},
                                   pipewire::node::type, pipewire::node::version,
                                   pipewire::update_strategy::none);

    if (target == "[None]")
    {
        while (virtMicRunning)
        {
            main_loop.run();
        }
        return;
    }

    auto reg_events = reg.listen<pipewire::registry_listener>();
    reg_events.on<pipewire::registry_event::global>(
        [&](const pipewire::global &global)
        {
            if (global.type == pipewire::node::type)
            {
                auto node = reg.bind<pipewire::node>(global.id);
                auto info = node.info();
                std::string name = getTarget(info.props);
                if (name == nullstr)
                    return;

                printf("vencord-virtmic: Added %s", name.c_str());
                if (!nodes.count(global.id))
                {
                    nodes.emplace(global.id, node.info());
                    link(target, core);
                }
            }
            if (global.type == pipewire::port::type)
            {
                auto port = reg.bind<pipewire::port>(global.id);
                auto info = port.info();

                if (info.props.count("node.id"))
                {
                    auto node_id = std::stoul(info.props["node.id"]);

                    if (node_id == virtual_mic.id() &&
                        info.direction == pipewire::port_direction::input)
                    {
                        if (info.props["audio.channel"] == "FL")
                        {
                            virt_fl = std::make_unique<pipewire::port>(std::move(port));
                        }
                        else
                        {
                            virt_fr = std::make_unique<pipewire::port>(std::move(port));
                        }
                    }
                    else
                    {
                        ports.emplace(global.id, std::move(port));
                    }

                    link(target, core);
                }
            }
        });

    reg_events.on<pipewire::registry_event::global_removed>(
        [&](const std::uint32_t id)
        {
            if (nodes.count(id))
            {
                auto info = nodes.at(id);
                std::string name = getTarget(info.props);
                if (name == nullstr)
                    return;

                printf("vencord-virtmic: Removed %s", name.c_str());
                nodes.erase(id);
            }
            if (ports.count(id))
            {
                ports.erase(id);
            }
            if (links.count(id))
            {
                links.erase(id);
            }
        });

    while (virtMicRunning)
    {
        main_loop.run();
    }
};