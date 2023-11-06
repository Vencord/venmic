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

        if (!data.Has("props") || !data.Has("mode"))
        {
            Napi::Error::New(env, "[venmic] expected keys 'props' and 'mode'").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        auto mode  = convert<std::string>(data.Get("mode"));
        auto props = to_array<vencord::prop>(data.Get("props"));

        if (!mode || !props)
        {
            Napi::Error::New(env, "[venmic] expected 'mode' to be string and 'props' to be array of key-value pairs")
                .ThrowAsJavaScriptException();

            return Napi::Boolean::New(env, false);
        }

        if (mode.value() != "include" && mode.value() != "exclude")
        {
            Napi::Error::New(env, "[venmic] expected mode to be either exclude or include")
                .ThrowAsJavaScriptException();

            return Napi::Boolean::New(env, false);
        }

        vencord::patchbay::get().link({
            .mode  = mode.value() == "include" ? vencord::target_mode::include : vencord::target_mode::exclude,
            .props = props.value(),
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
