#pragma once

#include <map>
#include <vector>
#include <memory>
#include <string>

namespace vencord
{
    using node = std::map<std::string, std::string>;

    struct link_options
    {
        std::vector<node> include;
        std::vector<node> exclude;

      public:
        bool ignore_devices{true};        // Only link against non-device nodes
        bool only_default_speakers{true}; // Ignore nodes that don't play to the default speaker

      public:
        std::vector<node> workaround;
    };

    class patchbay
    {
        class impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        ~patchbay();

      private:
        patchbay();

      public:
        void link(link_options options);

      public:
        void unlink();

      public:
        [[nodiscard]] std::vector<node> list(std::vector<std::string> props);

      public:
        [[nodiscard]] static patchbay &get();
        [[nodiscard]] static bool has_pipewire();
    };
} // namespace vencord
