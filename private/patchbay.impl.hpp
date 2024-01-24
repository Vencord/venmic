#pragma once

#include "patchbay.hpp"
#include "message.hpp"

#include <map>
#include <thread>
#include <atomic>

#include <rohrkabel/node/node.hpp>
#include <rohrkabel/port/port.hpp>
#include <rohrkabel/link/link.hpp>

#include <rohrkabel/metadata/metadata.hpp>
#include <rohrkabel/registry/registry.hpp>

namespace vencord
{
    struct node_with_ports
    {
        pw::node_info info;
        std::vector<pw::port_info> ports;
    };

    struct speaker
    {
        std::string name;
        std::uint32_t id;
    };

    class patchbay::impl
    {
        std::jthread thread;

      public:
        std::unique_ptr<pw_recipe::sender> sender;
        std::unique_ptr<cr_recipe::receiver> receiver;

      private:
        link_options options;

      private:
        std::unique_ptr<pw::node> virt_mic;
        std::optional<vencord::speaker> speaker;
        std::multimap<std::uint32_t, pw::link> created;

      private:
        std::map<std::uint32_t, pw::link_info> links;
        std::map<std::uint32_t, node_with_ports> nodes;

      private:
        std::shared_ptr<pw::core> core;
        std::shared_ptr<pw::registry> registry;

      private:
        std::unique_ptr<pw::metadata> metadata;
        std::optional<std::uint32_t> lettuce_target; // https://github.com/Vencord/venmic/issues/15

      private:
        std::atomic_bool should_exit{false};

      public:
        ~impl();

      public:
        impl();

      private:
        void create_mic();
        void cleanup(bool);
        void relink(std::uint32_t);

      private:
        template <typename T>
        void bind(const pw::global &);

        template <typename T>
        void add_global(T &);

      private:
        void rem_global(std::uint32_t);

      private:
        void on_link(std::uint32_t);
        void on_node(std::uint32_t);

      private:
        template <typename T>
        void receive(cr_recipe::sender &, T &);

      private:
        void start(pw_recipe::receiver, cr_recipe::sender);
    };
} // namespace vencord
