#include <format>
#include <iostream>

#include <httplib.h>
#include <glaze/glaze.hpp>

#include <vencord/logger.hpp>
#include <vencord/patchbay.hpp>

template <>
struct glz::meta<vencord::target_mode>
{
    using enum vencord::target_mode;
    static constexpr auto value = enumerate("include", include, "exclude", exclude);
};

template <>
struct glz::meta<vencord::prop>
{
    using T                     = vencord::prop;
    static constexpr auto value = object("key", &T::key, "value", &T::value);
};

struct targets
{
    std::vector<vencord::prop> include;
    std::vector<vencord::prop> exclude;
};

template <>
struct glz::meta<targets>
{
    using T                     = targets;
    static constexpr auto value = object("include", &T::include, "exclude", &T::exclude);
};

int main(int argc, char **args)
{
    using vencord::logger;
    using vencord::patchbay;

    auto port = 7591;

    if (argc > 1)
    {
        try
        {
            port = std::stoi(args[1]);
        }
        catch (...)
        {
            logger::get()->error("Bad arguments, usage: {} [port]", args[0]);
            return 1;
        }
    }

    logger::get()->warn("DISCLAIMER: This program is not intended for standalone usage. You need a modified discord "
                        "client that makes use of this!");

    logger::get()->info("Running on port: {}", port);

    httplib::Server server;

    server.Post("/list",
                [](const auto &req, auto &response)
                {
                    auto props = glz::read_json<std::vector<std::string>>(req.body);
                    auto data  = glz::write_json(patchbay::get().list(props.value_or(std::vector<std::string>{})));

                    response.set_content(data, "application/json");
                });

    server.Post("/link",
                [](const auto &req, auto &response)
                {
                    targets parsed;

                    auto error = glz::read<glz::opts{.error_on_missing_keys = true}>(parsed, req.body);

                    if (error)
                    {
                        response.status = 418;
                        return;
                    }

                    patchbay::get().link(parsed.include, parsed.exclude);

                    response.status = 200;
                });

    server.Get("/unlink",
               [](const auto &, auto &response)
               {
                   patchbay::get().unlink();
                   response.status = 200;
               });

    server.listen("0.0.0.0", port);

    return 0;
}
