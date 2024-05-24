#pragma once

#include "patchbay.hpp"
#include "message.hpp"

#include <map>
#include <thread>
#include <atomic>
#include <string_view>

#include <rohrkabel/global.hpp>

#include <rohrkabel/node/node.hpp>
#include <rohrkabel/port/port.hpp>
#include <rohrkabel/link/link.hpp>

#include <rohrkabel/metadata/events.hpp>
#include <rohrkabel/metadata/metadata.hpp>

#include <rohrkabel/registry/events.hpp>
#include <rohrkabel/registry/registry.hpp>

namespace vencord
{
    using port_map = std::vector<std::pair<pw::port_info, pw::port_info>>;

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
        std::stop_source update_source;
        std::unique_ptr<pw_recipe::sender> sender;
        std::unique_ptr<cr_recipe::receiver> receiver;

      private:
        link_options options;

      private:
        /**
         * ╔══════════════════╗
         * ║ Metadata related ║
         * ╚══════════════════╝
         *
         * 1. The *default* metadata is bound to @see metadata
         * 2. A listener is installed to check for default speaker updates, @see meta_listener
         * 3. Speaker info is saved to @see speaker, the name is parsed from the metadata, the id is set in @see on_node
         * 4. The @see lettuce_target is used by the workaround, a redirect is installed in the metadata (see venmic#15)
         */

        std::unique_ptr<pw::metadata> metadata;
        std::unique_ptr<pw::metadata_listener> meta_listener;

        std::optional<vencord::speaker> speaker;
        std::optional<std::uint32_t> lettuce_target;

      private:
        /**
         * ╔══════════════════╗
         * ║ Virt mic related ║
         * ╚══════════════════╝
         *
         * 1. The @see virt_mic is created by @see create_mic
         * 2. All created links, @see relink, are bound in @see created
         *                                                 └┬─────────┘
         *                                            key: related node
         *                                          value: bound link
         */

        std::unique_ptr<pw::node> virt_mic;
        [[vc::check_erase]] std::multimap<std::uint32_t, pw::link> created;

      private:
        /**
         * ╔═══════════════╗
         * ║ Logic related ║
         * ╚═══════════════╝
         *
         * 1. All links we encounter are saved in @see links
         * 2. All nodes we encounter are saved in @see nodes
         */

        [[vc::check_erase]] std::map<std::uint32_t, pw::link_info> links;
        [[vc::check_erase]] std::map<std::uint32_t, node_with_ports> nodes;

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
        void create_mic();
        void cleanup(bool);

      private:
        void reload();

      private:
        void link(std::uint32_t);
        port_map map_ports(const node_with_ports &);

      private:
        void meta_update(std::string_view, pw::metadata_property);

      private:
        void on_link(std::uint32_t);
        void on_node(std::uint32_t);

      private:
        template <typename T>
        void bind(const pw::global &);

      public:
        template <typename T>
        void handle(T &, const pw::global &);

      private:
        void del_global(std::uint32_t);
        void add_global(const pw::global &);

      private:
        template <typename T>
        void receive(cr_recipe::sender &, T &);

      private:
        void start(pw_recipe::receiver, cr_recipe::sender);
    };
} // namespace vencord
