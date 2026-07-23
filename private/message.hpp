#pragma once

#include "patchbay.hpp"

#include <string>
#include <vector>

#include <rohrkabel/channel/channel.hpp>

namespace vencord
{
    namespace pw = pipewire;

    struct list
    {
        std::vector<std::string> props;
    };

    struct unmute
    {
    };

    struct unlink
    {
    };

    struct quit
    {
    };

    struct ready
    {
        bool success{true};
    };

    using pw_recipe = pw::recipe<list, link_options, unlink, unmute, quit>;
    using cr_recipe = cr::recipe<std::vector<node>, ready, quit>;
} // namespace vencord
