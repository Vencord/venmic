#pragma once
#include <set>
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
        std::string name;
        target_mode mode;
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
        std::set<std::string> list();

      public:
        void link(target target);

      public:
        void unlink();

      public:
        static patchbay &get();
    };
} // namespace vencord
