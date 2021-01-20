/*
 * Copyright (c) 2021, Brendan Walker <brendan@millerwalker.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <napi.h>

#include "HSLClient_CAPI.h"
#include "ClientConstants.h"

Napi::Value Initialize(const Napi::CallbackInfo &info)
{
	Napi::Env env = info.Env();

	HSLResult result = HSL_Initialize(HSLLogSeverityLevel::HSLLogSeverityLevel_error);

	return Napi::Boolean::New(env, result == HSLResult_Success);
}

Napi::Value Update(const Napi::CallbackInfo& info)
{
	Napi::Env env = info.Env();

	HSLResult result = HSL_Update();

	return Napi::Boolean::New(env, result == HSLResult_Success);
}

Napi::Value Shutdown(const Napi::CallbackInfo& info)
{
	Napi::Env env = info.Env();

	HSLResult result = HSL_Shutdown();

	return Napi::Boolean::New(env, result == HSLResult_Success);
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	exports.Set("initialize", Napi::Function::New(env, Initialize));
	exports.Set("update", Napi::Function::New(env, Update));
	exports.Set("shutdown", Napi::Function::New(env, Shutdown));
	return exports;
}

NODE_API_MODULE(heartsensorlibrary, Init)