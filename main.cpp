/*
 * vencord-virtmic
 * Copyright (c) 2023 Vendicated
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <iostream>
#include <optional>
#include <unordered_set>
#include <rohrkabel/main_loop.hpp>
#include <rohrkabel/registry/registry.hpp>

void listTargets();
void start(std::string target);
const std::string &getTarget(const pipewire::spa::dict &props);

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::cout << "Usage: " << argv[0] << " <target | --list-targets>\n";
        return 1;
    }

    if (argv[1] == std::string("--list-targets"))
    {
        listTargets();
        return 0;
    }
    else
    {
        start(argv[1]);
    }

    return 0;
}

/*========================================================================*/
/* discord-screenaudio                                                    */
/* Copyright (c) 2023  Malte Jürgens and discord-screenaudio contributors */
/* SPDX-License-Identifier: GPL-3.0-or-later                              */
/*========================================================================*/

void listTargets()
{
    auto main_loop = pipewire::main_loop();
    auto context = pipewire::context(main_loop);
    auto core = pipewire::core(context);
    auto reg = pipewire::registry(core);

    std::unordered_set<std::string> seenTargets;

    auto reg_listener = reg.listen<pipewire::registry_listener>();
    reg_listener.on<pipewire::registry_event::global>(
        [&](const pipewire::global &global)
        {
            if (global.type == pipewire::node::type)
            {
                auto node = reg.bind<pipewire::node>(global.id);
                node.wait();
                auto info = node.get().map(
                    [&](auto node)
                    {
                        auto name = getTarget(node.info().props);
                        if (name != "" && name != "Chromium input" && seenTargets.count(name) == 0)
                        {
                            seenTargets.insert(name);
                            std::cout << name << '\n';
                        }
                    });
            }
        });

    core.update();
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

/*========================================================================*/
/* rohrkabel                                                              */
/* Copyright (c) 2022  Soundux                                            */
/* SPDX-License-Identifier: MIT                                           */
/*========================================================================*/

std::map<std::uint32_t, pipewire::port> ports;
std::optional<pipewire::port> virt_fl, virt_fr;

std::map<std::uint32_t, pipewire::node> nodes;
std::map<std::uint32_t, pipewire::link> links;

void link(const std::string &target, pipewire::core &core)
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

        if (parent.info().props["node.name"].find(target) != std::string::npos)
        {
            std::cout << "Link   : " << target << ":" << port_id << " -> ";

            if (port.info().props["audio.channel"] == "FL")
            {
                links.emplace(port_id, *core.create_simple<pipewire::link>(virt_fl->info().id, port_id).get());
                std::cout << virt_fl->info().id << std::endl;
            }
            else
            {
                links.emplace(port_id, *core.create_simple<pipewire::link>(virt_fr->info().id, port_id).get());
                std::cout << virt_fr->info().id << std::endl;
            }
        }
    }
}

//? Due to pipewire listing some ports before adding the node
//? we need to call link everytime a node or port is added to catch
//? un-linked ports which we will then link.

void start(std::string target)
{
    auto main_loop = pipewire::main_loop();
    auto context = pipewire::context(main_loop);
    auto core = pipewire::core(context);
    auto reg = pipewire::registry(core);

    auto virtual_mic = core.create("adapter",
                                   {
                                       {"node.name", "vencord-virtmic"},            //
                                       {"media.class", "Audio/Source/Virtual"},     //
                                       {"factory.name", "support.null-audio-sink"}, //
                                       {"audio.channels", "2"},                     //
                                       {"audio.position", "FL,FR"}                  //
                                   },
                                   pipewire::node::type, pipewire::node::version, pipewire::update_strategy::none)
                           .share();

    auto reg_events = reg.listen<pipewire::registry_listener>();
    reg_events.on<pipewire::registry_event::global>([&](const pipewire::global &global)
                                                    {
        if (global.type == pipewire::node::type)
        {
            auto node = reg.bind<pipewire::node>(global.id).get();
            std::cout << "Added  : " << node->info().props["node.name"] << std::endl;

            if (!nodes.count(global.id))
            {
                nodes.emplace(global.id, std::move(*node));
                link(target, core);
            }
        }
        if (global.type == pipewire::port::type)
        {
            auto port = reg.bind<pipewire::port>(global.id).get();
            auto info = port->info();

            if (info.props.count("node.id"))
            {
                auto node_id = std::stoul(info.props["node.id"]);

                if (node_id == virtual_mic.get()->id() && info.direction == pipewire::port_direction::input)
                {
                    if (info.props["audio.channel"] == "FL")
                    {
                        virt_fl.emplace(std::move(*port));
                    }
                    else
                    {
                        virt_fr.emplace(std::move(*port));
                    }
                }
                else
                {
                    ports.emplace(global.id, std::move(*port));
                }

                link(target, core);
            }
        } });

    reg_events.on<pipewire::registry_event::global_removed>([&](const std::uint32_t id)
                                                            {
        if (nodes.count(id))
        {
            auto info = nodes.at(id).info();
            std::cout << "Removed: " << info.props["node.name"] << std::endl;
            nodes.erase(id);
        }
        if (ports.count(id))
        {
            ports.erase(id);
        }
        if (links.count(id))
        {
            links.erase(id);
        } });

    while (true)
    {
        main_loop.run();
    }
}