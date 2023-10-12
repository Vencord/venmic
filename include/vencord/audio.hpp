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

    class audio
    {
        class impl;

      private:
        std::unique_ptr<impl> m_impl;

      public:
        ~audio();

      private:
        audio();

      public:
        std::set<std::string> list();

      public:
        void link(target target);

      public:
        void unlink();

      public:
        static audio &get();
    };
} // namespace vencord
