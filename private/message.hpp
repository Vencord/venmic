#pragma once

#include "patchbay.hpp"

#include <string>
#include <vector>

#include <rohrkabel/channel/channel.hpp>

namespace vencord
{
    namespace pw = pipewire;

    struct list_nodes
    {
        std::vector<std::string> props;
    };

    struct unset_target
    {
    };

    struct quit
    {
    };

    struct ready
    {
        bool success{true};
    };

    using pw_recipe = pw::recipe<list_nodes, link_options, unset_target, quit>;
    using cr_recipe = cr::recipe<std::vector<node>, ready, quit>;
} // namespace vencord
