#pragma once
#include <memory>
#include <vector>
#include <cstdint>

namespace vencord
{
    struct node
    {
        std::string name;
        std::uint32_t id;
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
        std::vector<node> list();

      public:
        void link(std::string name);

      public:
        static audio &get();
    };
} // namespace vencord