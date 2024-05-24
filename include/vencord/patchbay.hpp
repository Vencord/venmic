#pragma once

#include <map>
#include <vector>
#include <memory>
#include <string>

namespace vencord
{
    struct prop
    {
        std::string key;
        std::string value;
    };

    struct link_options
    {
        std::vector<prop> include;
        std::vector<prop> exclude;

      public:
        bool ignore_devices{true};     // Only link against non-device nodes
        bool ignore_input_media{true}; // Ignore Nodes that have "Input" in their media class

      public:
        bool only_default_speakers{true};

      public:
        std::vector<prop> workaround;
    };

    using node = std::map<std::string, std::string>;

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
