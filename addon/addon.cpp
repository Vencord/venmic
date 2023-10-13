#include <cstdio>
#include <napi.h>

#include <vencord/audio.hpp>

#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

struct audio : public Napi::ObjectWrap<audio>
{
    audio(const Napi::CallbackInfo &info) : Napi::ObjectWrap<audio>::ObjectWrap(info)
    {
        try
        {
            vencord::audio::get();
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

        auto list = vencord::audio::get().list();
        auto rtn  = Napi::Array::New(env, list.size());

        auto convert = [&](const auto &item)
        {
            return Napi::String::New(env, item);
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

        if (info.Length() != 2 || !info[0].IsString() || !info[1].IsString())
        {
            Napi::Error::New(env, "[venmic] expected two string arguments").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        auto target = static_cast<std::string>(info[0].ToString());
        auto mode   = static_cast<std::string>(info[1].ToString());

        if (mode != "include" && mode != "exclude")
        {
            Napi::Error::New(env, "[venmic] expected mode to be either exclude or include")
                .ThrowAsJavaScriptException();

            return Napi::Boolean::New(env, false);
        }

        vencord::audio::get().link({
            target,
            mode == "include" ? vencord::target_mode::include : vencord::target_mode::exclude,
        });

        return Napi::Boolean::New(env, true);
    }

    Napi::Value unlink([[maybe_unused]] const Napi::CallbackInfo &) // NOLINT(*-static)
    {
        vencord::audio::get().unlink();
        return {};
    }

  public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports)
    {
        static constexpr auto attributes = static_cast<napi_property_attributes>(napi_writable | napi_configurable);

        auto func = DefineClass(env, "Audio",
                                {
                                    InstanceMethod<&audio::link>("link", attributes),
                                    InstanceMethod<&audio::list>("list", attributes),
                                    InstanceMethod<&audio::unlink>("unlink", attributes),
                                });

        auto *constructor = new Napi::FunctionReference{Napi::Persistent(func)};

        exports.Set("Audio", func);

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
    audio::Init(env, exports);
    return exports;
}

// NOLINTNEXTLINE
NODE_API_MODULE(venmic, init);
