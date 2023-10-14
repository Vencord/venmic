#include <cstdio>
#include <napi.h>

#include <vencord/patchbay.hpp>

#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

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

        auto list = vencord::patchbay::get().list();
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

        if (info.Length() != 3 || !info[0].IsString() || !info[1].IsString() || !info[2].IsString())
        {
            Napi::Error::New(env, "[venmic] expected three string arguments").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        auto key   = static_cast<std::string>(info[0].ToString());
        auto value = static_cast<std::string>(info[1].ToString());
        auto mode  = static_cast<std::string>(info[2].ToString());

        if (mode != "include" && mode != "exclude")
        {
            Napi::Error::New(env, "[venmic] expected mode to be either exclude or include")
                .ThrowAsJavaScriptException();

            return Napi::Boolean::New(env, false);
        }

        vencord::patchbay::get().link({
            key,
            value,
            mode == "include" ? vencord::target_mode::include : vencord::target_mode::exclude,
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
