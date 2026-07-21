#pragma once

#include "patchbay.hpp"
#include "message.hpp"

#include <thread>
#include <optional>
#include <unordered_map>

#include <coco/stray/stray.hpp>

#include <rohrkabel/global.hpp>
#include <rohrkabel/core/core.hpp>
#include <rohrkabel/registry/registry.hpp>

#include <rohrkabel/link/link.hpp>
#include <rohrkabel/port/port.hpp>
#include <rohrkabel/node/node.hpp>

#include <rohrkabel/metadata/events.hpp>
#include <rohrkabel/metadata/metadata.hpp>

namespace vencord
{
    struct speaker
    {
        std::string name;
        std::optional<std::uint32_t> id;
    };

    struct metadata
    {
        pw::metadata value;
        pw::metadata_listener listener;
    };

    enum class clean : std::uint8_t
    {
        without_mic = 0,
        with_mic    = 1,
    };

    struct patchbay::impl
    {
        std::unique_ptr<pw_recipe::sender> sender;
        std::unique_ptr<cr_recipe::receiver> receiver;

      private:
        std::shared_ptr<pw::main_loop> loop;
        std::shared_ptr<pw::context> context;
        std::shared_ptr<pw::core> core;
        std::optional<pw::registry> registry;

      private:
        std::optional<metadata> meta;
        std::optional<speaker> default_speaker;

      private:
        std::optional<link_options> options;
        std::shared_ptr<std::uint32_t> workaround_target;

      private:
        std::optional<pw::node> virt_mic;
        std::unordered_map<std::uint32_t, pw::impl::module> virt_links;

      private:
        std::unordered_map<std::uint32_t, pw::node_info> nodes;
        std::unordered_map<std::uint32_t, pw::port_info> ports;
        std::unordered_map<std::uint32_t, pw::link_info> links;

      private:
        std::jthread worker;

      public:
        impl();

      public:
        ~impl();

      private:
        void cleanup(clean);

      private:
        coco::task<void> create_mic();
        coco::task<void> redirect(std::optional<pw::node_info> = {});

      private:
        bool should_link(const pw::node_info &);
        void link(const pw::node_info &, const pw::node_info &);

      private:
        std::map<std::uint32_t, pw::port_info> ports_of(const pw::node_info &);
        std::map<std::uint32_t, pw::link_info> links_of(const pw::node_info &);

      private:
        template <typename T>
        coco::stray handle(T);

      private:
        void add_global(pw::global);
        void del_global(std::uint32_t);

      private:
        template <typename T>
        coco::stray receive(cr_recipe::sender, T);

      private:
        void start(pw_recipe::receiver, cr_recipe::sender);
    };
} // namespace vencord
