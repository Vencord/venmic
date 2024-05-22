#include <httplib.h>

#include <exception>
#include <glaze/glaze.hpp>

#include <vencord/logger.hpp>
#include <vencord/patchbay.hpp>

template <>
struct glz::meta<vencord::prop>
{
    using T                     = vencord::prop;
    static constexpr auto value = object("key", &T::key, "value", &T::value);
};

template <>
struct glz::meta<vencord::link_options>
{
    using T                     = vencord::link_options;
    static constexpr auto value = object("exclude", &T::exclude,                             //
                                         "include", &T::include,                             //
                                         "ignore_devices", &T::ignore_devices,               //
                                         "only_default_speakers", &T::only_default_speakers, //
                                         "workaround", &T::workaround);
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

    server.set_exception_handler(
        [&](const auto &, auto &, auto exception)
        {
            try
            {
                std::rethrow_exception(exception);
            }
            catch (const std::exception &ex)
            {
                logger::get()->error("Encountered error: {}", ex.what());
            }
            catch (...)
            {
                logger::get()->error("Encountered error: <Unknown>");
            }

            server.stop();
        });

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
                    vencord::link_options parsed;

                    const auto error = glz::read_json(parsed, req.body);

                    if (error)
                    {
                        response.status = 418;
                        return;
                    }

                    patchbay::get().link(std::move(parsed));

                    response.status = 200;
                });

    server.Get("/has-pipewire-pulse",
               [](const auto &, auto &response)
               {
                   auto data = glz::write_json(patchbay::has_pipewire());

                   response.set_content(data, "application/json");
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
