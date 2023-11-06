#pragma once
#include <map>
#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace vencord
{
    enum class target_mode
    {
        include,
        exclude,
    };

    struct prop
    {
        std::string key;
        std::string value;
    };

    struct target
    {
        target_mode mode;
        std::vector<prop> props;
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
        void link(target targets);

      public:
        void unlink();

      public:
        [[nodiscard]] std::vector<node> list(std::vector<std::string> props);

      public:
        [[nodiscard]] static patchbay &get();
        [[nodiscard]] static bool has_pipewire();
    };
} // namespace vencord
