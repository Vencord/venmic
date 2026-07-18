#include <tuple>
#include <ranges>
#include <optional>

#include <napi.h>
#include <vencord/patchbay.hpp>

namespace
{
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
    std::optional<vencord::node> convert(Napi::Value value)
    {
        if (!value.IsObject())
        {
            return std::nullopt;
        }

        auto object = value.As<Napi::Object>();
        auto rtn    = vencord::node{};

        for (const auto &[obj_key, obj_value] : object)
        {
            const auto key   = convert<std::string>(obj_key);
            const auto value = convert<std::string>(obj_value);

            if (!key.has_value() || !value.has_value())
            {
                return std::nullopt;
            }

            rtn.emplace(*key, *value);
        }

        return rtn;
    }

    template <typename T>
    std::optional<std::vector<T>> to_array(Napi::Value value)
    {
        if (!value.IsArray())
        {
            return std::nullopt;
        }

        auto array = value.As<Napi::Array>();
        auto rtn   = std::vector<T>{};

        rtn.reserve(array.Length());

        for (auto i = 0uz; array.Length() > i; ++i)
        {
            auto converted = convert<T>(array.Get(i));

            if (!converted.has_value())
            {
                return std::nullopt;
            }

            rtn.emplace_back(*converted);
        }

        return rtn;
    }

    struct patchbay : public Napi::ObjectWrap<patchbay>
    {
        patchbay(const Napi::CallbackInfo &info) : Napi::ObjectWrap<patchbay>::ObjectWrap(info)
        {
            try
            {
                std::ignore = vencord::patchbay::get();
            }
            catch (std::exception &e)
            {
                Napi::Error::New(info.Env(), e.what()).ThrowAsJavaScriptException();
            }
        }

      public:
        Napi::Value list(const Napi::CallbackInfo &info) // NOLINT(*-static)
        {
            const auto env = info.Env();
            auto props     = std::vector<std::string>{};

            if (info.Length() == 1 && !info[0].IsUndefined())
            {
                auto array = to_array<std::string>(info[0]);

                if (!array.has_value())
                {
                    Napi::Error::New(env, "[venmic] expected list of strings").ThrowAsJavaScriptException();
                    return {};
                }

                props = std::move(*array);
            }

            auto list = vencord::patchbay::get().list(props);
            auto rtn  = Napi::Array::New(env, list.size());

            const auto convert = [&](const auto &item)
            {
                auto rtn = Napi::Object::New(env);

                for (const auto &[key, value] : item)
                {
                    rtn.Set(key, Napi::String::New(env, value));
                }

                return rtn;
            };

            const auto add = [&](const auto &item)
            {
                rtn.Set(std::get<0>(item), std::get<1>(item));
            };

            std::ranges::for_each(list                                 //
                                      | std::views::transform(convert) //
                                      | std::views::enumerate,
                                  add);

            return rtn;
        }

        Napi::Value link(const Napi::CallbackInfo &info) // NOLINT(*-static)
        {
            const auto env = info.Env();

            if (info.Length() != 1 || !info[0].IsObject())
            {
                Napi::Error::New(env, "[venmic] expected link object").ThrowAsJavaScriptException();
                return Napi::Boolean::New(env, false);
            }

            const auto data = info[0].ToObject();

            if (!data.Has("include") && !data.Has("exclude"))
            {
                Napi::Error::New(env, "[venmic] expected at least one of keys 'include' or 'exclude'")
                    .ThrowAsJavaScriptException();

                return Napi::Boolean::New(env, false);
            }

            const auto include               = to_array<vencord::node>(data.Get("include"));
            const auto exclude               = to_array<vencord::node>(data.Get("exclude"));
            const auto ignore_devices        = convert<bool>(data.Get("ignore_devices"));
            const auto only_speakers         = convert<bool>(data.Get("only_speakers"));
            const auto only_default_speakers = convert<bool>(data.Get("only_default_speakers"));
            const auto workaround            = to_array<vencord::node>(data.Get("workaround"));
            const auto legacy_workaround     = convert<bool>(data.Get("legacy_workaround"));

            if (!include.has_value() && !exclude.has_value())
            {
                Napi::Error::New(env, "[venmic] expected either 'include' or 'exclude' or both to be present and to be "
                                      "arrays of key-value pairs")
                    .ThrowAsJavaScriptException();

                return Napi::Boolean::New(env, false);
            }

            vencord::patchbay::get().link({
                .include               = include.value_or(std::vector<vencord::node>{}),
                .exclude               = exclude.value_or(std::vector<vencord::node>{}),
                .ignore_devices        = ignore_devices.value_or(true),
                .only_speakers         = only_speakers.value_or(true),
                .only_default_speakers = only_default_speakers.value_or(true),
                .legacy_workaround     = legacy_workaround.value_or(false),
                .workaround            = workaround.value_or(std::vector<vencord::node>{}),
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

            const auto func = DefineClass(env, "PatchBay",
                                          {
                                              InstanceMethod<&patchbay::link>("link", attributes),
                                              InstanceMethod<&patchbay::list>("list", attributes),
                                              InstanceMethod<&patchbay::unlink>("unlink", attributes),
                                              StaticMethod<&patchbay::has_pipewire>("hasPipeWire", attributes),
                                          });

            auto *const constructor = new Napi::FunctionReference{Napi::Persistent(func)};

            exports.Set("PatchBay", func);
            env.SetInstanceData<Napi::FunctionReference>(constructor);

            return exports;
        }

        static Napi::Object CreateNewItem(const Napi::CallbackInfo &info)
        {
            const auto env          = info.Env();
            auto *const constructor = env.GetInstanceData<Napi::FunctionReference>();

            return constructor->New({});
        }
    };

    Napi::Object init(Napi::Env env, Napi::Object exports)
    {
        patchbay::Init(env, exports);
        return exports;
    }
} // namespace

// NOLINTNEXTLINE
NODE_API_MODULE(venmic, init);
