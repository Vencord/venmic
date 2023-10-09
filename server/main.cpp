#include <format>
#include <iostream>

#include <httplib.h>
#include <glaze/glaze.hpp>
#include <vencord/audio.hpp>

template <>
struct glz::meta<vencord::node>
{
    using T                     = vencord::node;
    static constexpr auto value = object("name", &T::name, "id", &T::id);
};

struct link_body
{
    std::string target;
};

template <>
struct glz::meta<link_body>
{
    using T                     = link_body;
    static constexpr auto value = object("target", &T::target);
};

int main(int argc, char **args)
{
    using vencord::audio;

    auto port = 3030;

    if (argc > 1)
    {
        try
        {
            port = std::stoi(args[1]);
        }
        catch (...)
        {
            std::cerr << std::format("Bad arguments, usage: {} [port]", args[0]) << std::endl;
            return 1;
        }
    }

    httplib::Server server;

    server.Get("/list",
               [](const auto &, auto &response)
               {
                   auto data = glz::write_json(audio::get().list());
                   response.set_content(data, "application/json");
               });

    server.Post("/link",
                [](const auto &req, auto &response)
                {
                    link_body parsed;
                    auto error = glz::read<glz::opts{.error_on_missing_keys = true}>(parsed, req.body);

                    if (error)
                    {
                        response.status = 418;
                        return;
                    }

                    audio::get().link(parsed.target);

                    response.status = 200;
                });

    server.Get("/unlink",
               [](const auto &, auto &response)
               {
                   audio::get().unlink();
                   response.status = 200;
               });

    server.listen("0.0.0.0", port);

    return 0;
}