#pragma once

#include "patchbay.hpp"

#include <string>

#include <cr/channel.hpp>
#include <rohrkabel/channel/channel.hpp>

namespace pw = pipewire;

namespace vencord
{
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

    struct abort
    {
    };

    struct ready
    {
        bool success{true};
    };

    using pw_recipe = pw::recipe<list_nodes, link_options, unset_target, quit, abort>;
    using cr_recipe = cr::recipe<std::vector<node>, ready, quit>;
} // namespace vencord
