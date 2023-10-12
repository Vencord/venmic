#include <cstdio>
#include <napi.h>

#include <vencord/audio.hpp>

#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

struct audio : public Napi::ObjectWrap<audio>
{
    using Napi::ObjectWrap<audio>::ObjectWrap;

  public:
    Napi::Value list(const Napi::CallbackInfo &info) // NOLINT(*-static)
    {
        auto env = info.Env();

        auto list = vencord::audio::get().list();
        auto rtn  = Napi::Array::New(env, list.size());

        auto convert = [&](const auto &item)
        {
            auto obj = Napi::Object::New(env);

            obj.Set("name", Napi::String::New(env, item.name));
            obj.Set("speaker", Napi::Boolean::New(env, item.speaker));

            return obj;
        };

        auto add = [&](const auto &item)
        {
            auto [index, value] = item;
            rtn.Set(index, value);
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

        if (info.Length() != 1)
        {
            Napi::Error::New(env, "[venmic] expected one argument").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        if (!info[0].IsString())
        {
            Napi::Error::New(env, "[venmic] expected string").ThrowAsJavaScriptException();
            return Napi::Boolean::New(env, false);
        }

        vencord::audio::get().link(info[0].ToString());

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

        auto func = DefineClass(env, "audio",
                                {
                                    InstanceMethod<&audio::link>("link", attributes),
                                    InstanceMethod<&audio::list>("list", attributes),
                                    InstanceMethod<&audio::unlink>("unlink", attributes),
                                });

        auto *constructor = new Napi::FunctionReference{Napi::Persistent(func)};

        exports.Set("audio", func);

        env.SetInstanceData<Napi::FunctionReference>(constructor);

        return exports;
    }

    static Napi::Object CreateNewItem(const Napi::CallbackInfo &info)
    {
        auto env          = info.Env();
        auto *constructor = env.GetInstanceData<Napi::FunctionReference>();

        try
        {
            vencord::audio::get();
        }
        catch (std::exception &e)
        {
            Napi::Error::New(info.Env(), e.what()).ThrowAsJavaScriptException();
        }

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
