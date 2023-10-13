#pragma once
#include <string>
#include <glaze/glaze.hpp>

namespace vencord
{
    struct pw_metadata_name
    {
        std::string name;
    };
} // namespace vencord

template <>
struct glz::meta<vencord::pw_metadata_name>
{
    using T                     = vencord::pw_metadata_name;
    static constexpr auto value = object("name", &T::name);
};
