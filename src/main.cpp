/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (c) 2023  Vendicated and Vencord contributors
 */

#include <node.h>
#include <virtmic.hpp>

namespace virtmic
{

    using v8::FunctionCallbackInfo;
    using v8::Isolate;
    using v8::Local;
    using v8::Object;
    using v8::String;
    using v8::Value;

    void ThrowTypeError(Isolate *isolate, const char *msg)
    {
        isolate->ThrowException(v8::Exception::TypeError(
            String::NewFromUtf8(isolate, msg).ToLocalChecked()));
    }

    void Start(const FunctionCallbackInfo<Value> &args)
    {
        auto isolate = args.GetIsolate();

        if (args.Length() != 1)
            return ThrowTypeError(isolate, "Wrong number of arguments");

        if (!args[0]->IsString())
            return ThrowTypeError(isolate, "Expected source to be a string");

        auto s = args[0].As<String>();
        auto target = std::string(*v8::String::Utf8Value(isolate, s));

        start(target);
    }

    void GetTargets(const FunctionCallbackInfo<Value> &args)
    {
        auto isolate = args.GetIsolate();
        auto targets = getTargets();

        auto arr = v8::Array::New(isolate, targets.size());
        for (unsigned int i = 0; i < targets.size(); i++)
        {
            arr->Set(isolate->GetCurrentContext(), i, v8::String::NewFromUtf8(isolate, targets[i].c_str()).ToLocalChecked());
        }

        args.GetReturnValue().Set(arr);
    }

    void Initialize(Local<Object> exports)
    {
        NODE_SET_METHOD(exports, "getTargets", GetTargets);
        NODE_SET_METHOD(exports, "start", Start);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)

}