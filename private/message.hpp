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

    struct target;

    using pw_recipe = pw::recipe<list_nodes, target, unset_target, quit>;
    using cr_recipe = cr::recipe<std::set<node>, ready>;
} // namespace vencord
