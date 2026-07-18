#pragma once

#include <memory>
#include <string>

#include <map>
#include <vector>

namespace vencord
{
    using node = std::map<std::string, std::string>;

    struct link_options
    {
        std::vector<node> include;
        std::vector<node> exclude;

      public:
        bool ignore_devices{true};        // Only link against non-device nodes
                                          //
      public:                             //
        bool only_speakers{true};         // Ignore nodes that don't play to speakers
        bool only_default_speakers{true}; // Ignore nodes that don't play to the default speaker
                                          //
      public:                             //
        bool legacy_workaround{false};    // Use non metadata workaround (for use with old pipewire versions)
        std::vector<node> workaround;     // Nodes given here will automatically be linked to the venmic node
    };

    struct patchbay
    {
        class impl;

      private:
        std::unique_ptr<impl> m_impl;

      private:
        patchbay();

      public:
        ~patchbay();

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
