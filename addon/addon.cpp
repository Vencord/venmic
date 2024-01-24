#include <cstdio>
#include <optional>

#include <vencord/patchbay.hpp>

#include <napi.h>
#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

template <typename T>
std::optional<T> convert(Napi::Value) = delete;

template <>
std::optional<std::string> convert(Napi::Value value)
{
    if (!value.IsString())
    {
        return std::nullopt;
    }

    return value.ToString();
}

template <>
std::optional<bool> convert(Napi::Value value)
{
    if (!value.IsBoolean())
    {
        return std::nullopt;
    }

    return value.ToBoolean();
}

template <>
std::optional<vencord::prop> convert(Napi::Value value)
{
    if (!value.IsObject())
    {
        return std::nullopt;
    }

    auto object = value.ToObject();

    if (!object.Has("key") || !object.Has("value"))
    {
        return std::nullopt;
    }

    return vencord::prop{.key = object.Get("key").ToString(), .value = object.Get("value").ToString()};
}

template <typename T>
std::optional<std::vector<T>> to_array(Napi::Value value)
{
    if (!value.IsArray())
    {
        return std::nullopt;
    }

    auto array = value.As<Napi::Array>();

    std::vector<T> rtn;
    rtn.reserve(array.Length());

    for (auto i = 0u; array.Length() > i; i++)
    {
        auto converted = convert<T>(array.Get(i));

        if (!converted)
        {
            return std::nullopt;
        }

        rtn.emplace_back(converted.value());
    }

    return rtn;
}

struct patchbay : public Napi::ObjectWrap<patchbay>
{
    patchbay(const Napi::CallbackInfo &info) : Napi::ObjectWrap<patchbay>::ObjectWrap(info)
    {
        try
        {
            static_cast<void>(vencord::patchbay::get());
        }
        catch (std::exception &e)
        {
            Napi::Error::New(info.Env(), e.what()).ThrowAsJavaScriptException();
        }
    }

  public:
    Napi::Value list(const Napi::CallbackInfo &info) // NOLINT(*-static)
    {
        auto env = info.Env();

        std::vector<std::string> props{};

        if (info.Length() == 1)
        {
            auto array = to_array<std::string>(info[0]);

            if (!array)
            {
                Napi::Error::New(env, "[venmic] expected list of strings").ThrowAsJavaScriptException();
                return {};
            }

            props = std::move(array.value());
        }

        auto list = vencord::patchbay::get().list(props);
        auto rtn  = Napi::Array::New(env, list.size());

        auto convert = [&](const auto &item)
        {
            auto rtn = Napi::Object::New(env);

            for (const auto &[key, value] : item)
            {
                rtn.Set(key, Napi::String::New(env, value));
            }

            return rtn;
        };
        auto add = [&](const auto &item)
        {
            rtn.Set(item.first, item.second);
        };

        ranges::for_each(list                                    //
                             | ranges::views::transform(convert) //
                             | ranges::views::enumerate,
                         add);

        return rtn;
    }

    Napi::Value link(const Napi::CallbackInfo &info) // NOLINT(*-static)
    {
        auto env = info.Env();

        if (info.Length() != 1 || !info[0].IsObject())
        {
            Napi::Error::New(env, "[venmic] expected link object").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        auto data = info[0].ToObject();

        if (!data.Has("include") && !data.Has("exclude"))
        {
            Napi::Error::New(env, "[venmic] expected at least one of keys 'include' or 'exclude'")
                .ThrowAsJavaScriptException();

            return Napi::Boolean::New(env, false);
        }

        auto include        = to_array<vencord::prop>(data.Get("include"));
        auto exclude        = to_array<vencord::prop>(data.Get("exclude"));
        auto ignore_devices = convert<bool>(data.Get("ignore_devices"));
        auto workaround     = convert<vencord::prop>(data.Get("workaround"));

        if (!include && !exclude)
        {
            Napi::Error::New(env, "[venmic] expected either 'include' or 'exclude' or both to be present and to be "
                                  "arrays of key-value pairs")
                .ThrowAsJavaScriptException();

            return Napi::Boolean::New(env, false);
        }

        vencord::patchbay::get().link({
            .include        = include.value_or(std::vector<vencord::prop>{}),
            .exclude        = exclude.value_or(std::vector<vencord::prop>{}),
            .ignore_devices = ignore_devices.value_or(true),
            .workaround     = workaround,
        });

        return Napi::Boolean::New(env, true);
    }

    Napi::Value unlink([[maybe_unused]] const Napi::CallbackInfo &) // NOLINT(*-static)
    {
        vencord::patchbay::get().unlink();
        return {};
    }

    static Napi::Value has_pipewire(const Napi::CallbackInfo &info)
    {
        return Napi::Boolean::New(info.Env(), vencord::patchbay::has_pipewire());
    }

  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        static constexpr auto attributes = static_cast<napi_property_attributes>(napi_writable | napi_configurable);

        auto func = DefineClass(env, "PatchBay",
                                {
                                    InstanceMethod<&patchbay::link>("link", attributes),
                                    InstanceMethod<&patchbay::list>("list", attributes),
                                    InstanceMethod<&patchbay::unlink>("unlink", attributes),
                                    StaticMethod<&patchbay::has_pipewire>("hasPipeWire", attributes),
                                });

        auto *constructor = new Napi::FunctionReference{Napi::Persistent(func)};

        exports.Set("PatchBay", func);

        env.SetInstanceData<Napi::FunctionReference>(constructor);

        return exports;
    }

    static Napi::Object CreateNewItem(const Napi::CallbackInfo &info)
    {
        auto env          = info.Env();
        auto *constructor = env.GetInstanceData<Napi::FunctionReference>();

        return constructor->New({});
    }
};

Napi::Object init(Napi::Env env, Napi::Object exports)
{
    patchbay::Init(env, exports);
    return exports;
}

// NOLINTNEXTLINE
NODE_API_MODULE(venmic, init);
