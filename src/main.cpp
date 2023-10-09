// TODO: Remove this file

#include "audio.hpp"
#include <iostream>

int main()
{
    auto &instance = vencord::audio::get();

    auto list = instance.list();
    instance.link("Firefox");

    std::cin.get();

    return 0;
}