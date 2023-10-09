#include <cstdio>
#include <napi.h>

#include <vencord/audio.hpp>

#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

Napi::Array venmic_list(const Napi::CallbackInfo &info)
{
    auto env = info.Env();

    auto list = vencord::audio::get().list();
    auto rtn  = Napi::Array::New(env, list.size());

    auto convert = [&](const auto &item)
    {
        auto obj = Napi::Object::New(env);

        obj.Set("name", Napi::String::New(env, item.name));
        obj.Set("id", Napi::Number::New(env, item.id));

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

Napi::Boolean venmic_link(const Napi::CallbackInfo &info)
{
    auto env = info.Env();

    if (info.Length() != 1)
    {
        Napi::Error::New(env, "[venmic] 'link' requires one argument");
        return Napi::Boolean::New(env, false);
    }

    if (!info[0].IsString())
    {
        Napi::Error::New(env, "[venmic] expected string");
        return Napi::Boolean::New(env, false);
    }

    vencord::audio::get().link(info[0].ToString());

    return Napi::Boolean::New(env, true);
}

Napi::Value venmic_unlink(const Napi::CallbackInfo &info)
{
    auto env = info.Env();

    vencord::audio::get().unlink();

    return {};
}

Napi::Object init(Napi::Env env, Napi::Object exports)
{
    exports.Set("list", Napi::Function::New(env, venmic_list));
    exports.Set("link", Napi::Function::New(env, venmic_link));
    exports.Set("unlink", Napi::Function::New(env, venmic_unlink));

    return exports;
}

// NOLINTNEXTLINE
NODE_API_MODULE(venmic, init);