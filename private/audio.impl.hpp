#pragma once
#include "audio.hpp"

#include <map>

#include <thread>
#include <atomic>

#include <rohrkabel/node/node.hpp>
#include <rohrkabel/port/port.hpp>
#include <rohrkabel/link/link.hpp>

#include <rohrkabel/channel/channel.hpp>
#include <rohrkabel/registry/registry.hpp>

namespace pw = pipewire;

namespace vencord
{
    struct list_nodes
    {
    };

    struct set_target
    {
        std::string id;
    };

    struct unset_target
    {
    };

    struct ready
    {
    };

    struct quit
    {
    };

    using pw_recipe = pw::recipe<list_nodes, set_target, unset_target, quit>;
    using cr_recipe = cr::recipe<std::vector<node>, ready>;

    struct node_with_ports
    {
        pw::node_info info;
        std::vector<pw::port_info> ports;
    };

    class audio::impl
    {
        std::thread thread;

      public:
        std::unique_ptr<pw_recipe::sender> sender;
        std::unique_ptr<cr_recipe::receiver> receiver;

      private:
        std::unique_ptr<pw::node> mic;
        std::optional<std::pair<std::string, std::uint32_t>> target;

      private:
        std::vector<pw::link> links;
        std::map<std::uint32_t, node_with_ports> nodes;

      private:
        std::shared_ptr<pw::core> core;
        std::shared_ptr<pw::registry> registry;

      private:
        std::atomic_bool should_exit{false};

      public:
        ~impl();

      public:
        impl();

      private:
        void relink();
        void create_mic();

      private:
        void global_removed(std::uint32_t);
        void global_added(const pw::global &);

      private:
        template <typename T>
        void receive(cr_recipe::sender, T);

      private:
        void start(pw_recipe::receiver, cr_recipe::sender);
    };
} // namespace vencord
