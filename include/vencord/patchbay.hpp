#pragma once
#include <set>
#include <map>
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

    struct target
    {
        std::string key;
        std::string value;

      public:
        target_mode mode;
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
        std::set<node> list();

      public:
        void link(target target);

      public:
        void unlink();

      public:
        static patchbay &get();
    };
} // namespace vencord