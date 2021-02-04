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

#define REQ_ARGS(N)                                                     \
  if (info.Length() < (N)) {                                            \
    Napi::Error::New(info.Env(),                                        \
      "Expected " #N " arguments")                                      \
      .ThrowAsJavaScriptException();                                    \
    return info.Env().Null();                                           \
  }

#define REQ_INT_ARG(I, VAR)                                             \
  int VAR;                                                              \
  if (info.Length() <= (I) || !info[I].IsNumber()) {                    \
    Napi::TypeError::New(info.Env(),                                    \
      "Argument " #I " must be an integer")                             \
      .ThrowAsJavaScriptException();                                    \
    return info.Env().Null();                                           \
  }                                                                     \
  VAR = info[I].ToNumber();

#define REQ_BOOL_ARG(I, VAR)                                            \
  bool VAR;                                                             \
  if (info.Length() <= (I) || !info[I].IsBoolean()) {                   \
    Napi::TypeError::New(info.Env(),                                    \
      "Argument " #I " must be an boolean")                             \
      .ThrowAsJavaScriptException();                                    \
    return info.Env().Null();                                           \
  }                                                                     \
  VAR = info[I].ToBoolean();

Napi::Value Update(const Napi::CallbackInfo& info)
{
	return Napi::Boolean::New(info.Env(), HSL_Update());
}

Napi::Value UpdateNoPollEvents(const Napi::CallbackInfo& info)
{
	return Napi::Boolean::New(info.Env(), HSL_UpdateNoPollEvents());
}

Napi::Value HasSensorListChanged(const Napi::CallbackInfo& info)
{
	return Napi::Boolean::New(info.Env(), HSL_HasSensorListChanged());
}

Napi::Value GetVersionString(const Napi::CallbackInfo& info)
{
	Napi::Env env = info.Env();

	char version_string[64];
	HSL_GetVersionString(version_string, sizeof(version_string));

	return Napi::String::From(info.Env(), version_string);
}

class BufferIterator : public Napi::ObjectWrap<BufferIterator>
{
public:
	BufferIterator(const Napi::CallbackInfo& info)
		: Napi::ObjectWrap<BufferIterator>(info)
	{
		if (info.Length() >= 1 && info[0].IsExternal())
		{
			Napi::External<HSLBufferIterator> inBufferIter = info[0].As<Napi::External<HSLBufferIterator>>();

			m_iterator = *inBufferIter.Data();
		}
		else
		{
			Napi::TypeError::New(info.Env(), "Argument 0 invalid").ThrowAsJavaScriptException();
		}
	}

	// Create a new item using the constructor stored during Init.
	static Napi::Value CreateNewIterator(const Napi::CallbackInfo& info, HSLBufferIterator &iter)
	{
		return constructor.New({Napi::External<HSLBufferIterator>::New(info.Env(), &iter)});
	}

	Napi::Value IsValid(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		return Napi::Boolean::New(env, HSL_IsBufferIteratorValid(&m_iterator));
	}

	Napi::Value Next(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		return Napi::Boolean::New(env, HSL_BufferIteratorNext(&m_iterator));
	}

	Napi::Value GetDataType(const Napi::CallbackInfo& info) // HSLSensorBufferType
	{
		Napi::Env env = info.Env();
		return Napi::Number::New(env, m_iterator.bufferType);
	}

	Napi::Value GetHRData(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		HSLHeartRateFrame *frame= HSL_BufferIteratorGetHRData(&m_iterator);
		if (frame == nullptr)
		{
			Napi::TypeError::New(env, "BufferIterator data is not valid HRData").ThrowAsJavaScriptException();                                    
			return env.Null();
		}

		auto RRIntervals= Napi::Uint16Array::New(env, frame->RRIntervalCount, napi_uint16_array);
		for (int i = 0; i < frame->RRIntervalCount; ++i)
		{
			RRIntervals.Set(i, frame->RRIntervals[i]);
		}

		Napi::Object obj = Napi::Object::New(env);
		obj.Set("contactStatus", (int)frame->contactStatus);
		obj.Set("beatsPerMinute", frame->beatsPerMinute);
		obj.Set("energyExpended", frame->energyExpended);
		obj.Set("RRIntervals", RRIntervals);
		obj.Set("timeInSeconds", frame->timeInSeconds);

		return obj;
	}

	Napi::Value GetECGData(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		HSLHeartECGFrame *frame= HSL_BufferIteratorGetECGData(&m_iterator);
		if (frame == nullptr)
		{
			Napi::TypeError::New(info.Env(), "BufferIterator data is not valid ECGData").ThrowAsJavaScriptException();
			return env.Null();
		}

		auto ecgValues = Napi::Uint32Array::New(env, frame->ecgValueCount, napi_uint32_array);
		for (int i = 0; i < frame->ecgValueCount; ++i)
		{
			ecgValues.Set(i, frame->ecgValues[i]);
		}

		Napi::Object obj = Napi::Object::New(env);
		obj.Set("ecgValues", ecgValues);
		obj.Set("timeInSeconds", frame->timeInSeconds);

		return obj;
	}

	Napi::Value GetPPGData(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		HSLHeartPPGFrame *frame= HSL_BufferIteratorGetPPGData(&m_iterator);
		if (frame == nullptr)
		{
			Napi::TypeError::New(info.Env(), "BufferIterator data is not valid PPGData").ThrowAsJavaScriptException();
			return env.Null();
		}

		auto ppgSamples = Napi::Array::New(env, frame->ppgSampleCount);
		for (int i = 0; i < frame->ppgSampleCount; ++i)
		{
			const HSLHeartPPGSample &ppgSample= frame->ppgSamples[i];

			Napi::Object obj = Napi::Object::New(env);
			obj.Set("ambient", ppgSample.ambient);
			obj.Set("ppgValue0", ppgSample.ppgValue0);
			obj.Set("ppgValue1", ppgSample.ppgValue1);
			obj.Set("ppgValue2", ppgSample.ppgValue2);

			ppgSamples.Set(i, obj);
		}

		Napi::Object obj = Napi::Object::New(env);
		obj.Set("ppgSamples", ppgSamples);
		obj.Set("timeInSeconds", frame->timeInSeconds);

		return obj;
	}

	Napi::Value GetPPIData(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		HSLHeartPPIFrame *frame= HSL_BufferIteratorGetPPIData(&m_iterator);
		if (frame == nullptr)
		{
			Napi::TypeError::New(info.Env(), "BufferIterator data is not valid PPIData").ThrowAsJavaScriptException();
			return info.Env().Null();
		}

		auto ppiSamples = Napi::Array::New(env, frame->ppiSampleCount);
		for (int i = 0; i < frame->ppiSampleCount; ++i)
		{
			const HSLHeartPPISample& ppiSample = frame->ppiSamples[i];

			Napi::Object obj = Napi::Object::New(env);
			obj.Set("beatsPerMinute", ppiSample.beatsPerMinute);
			obj.Set("pulseDuration", ppiSample.pulseDuration);
			obj.Set("pulseDurationErrorEst", ppiSample.pulseDurationErrorEst);
			obj.Set("blockerBit", ppiSample.blockerBit != 0);
			obj.Set("skinContactBit", ppiSample.skinContactBit != 0);
			obj.Set("supportsSkinContactBit", ppiSample.supportsSkinContactBit != 0);

			ppiSamples.Set(i, obj);
		}

		Napi::Object obj = Napi::Object::New(env);
		obj.Set("ppiSamples", ppiSamples);
		obj.Set("timeInSeconds", frame->timeInSeconds);

		return obj;
	}

	Napi::Value GetAccData(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		HSLAccelerometerFrame *frame= HSL_BufferIteratorGetAccData(&m_iterator);
		if (frame == nullptr)
		{
			Napi::TypeError::New(env, "BufferIterator data is not valid AccData").ThrowAsJavaScriptException();
			return env.Null();
		}

		auto accSamples = Napi::Array::New(env, frame->accSampleCount);
		for (int i = 0; i < frame->accSampleCount; ++i)
		{
			const HSLVector3f& accSample = frame->accSamples[i];

			Napi::Object obj = Napi::Object::New(env);
			obj.Set("x", accSample.x);
			obj.Set("y", accSample.y);
			obj.Set("z", accSample.z);

			accSamples.Set(i, obj);
		}

		Napi::Object obj = Napi::Object::New(env);
		obj.Set("accSamples", accSamples);
		obj.Set("timeInSeconds", frame->timeInSeconds);

		return obj;
	}

	Napi::Value GetHrvData(const Napi::CallbackInfo& info)
	{
		Napi::Env env = info.Env();
		HSLHeartVariabilityFrame *frame= HSL_BufferIteratorGetHRVData(&m_iterator);
		if (frame == nullptr)
		{
			Napi::TypeError::New(env, "BufferIterator data is not valid HrvData").ThrowAsJavaScriptException();
			return env.Null();
		}

		Napi::Object obj = Napi::Object::New(env);
		obj.Set("hrvValue", frame->hrvValue);
		obj.Set("timeInSeconds", frame->timeInSeconds);

		return obj;
	}

	static void Init(Napi::Env env, Napi::Object exports)
	{
		Napi::HandleScope scope(env);

		Napi::Function ctor = DefineClass(env, "BufferIterator", {
			InstanceMethod("isValid", &BufferIterator::IsValid),
			InstanceMethod("next", &BufferIterator::Next),
			InstanceMethod("getDataType", &BufferIterator::GetDataType),
			InstanceMethod("getHRData", &BufferIterator::GetHRData),
			InstanceMethod("getECGData", &BufferIterator::GetECGData),
			InstanceMethod("getPPGData", &BufferIterator::GetPPGData),
			InstanceMethod("getPPIData", &BufferIterator::GetPPIData),
			InstanceMethod("getAccData", &BufferIterator::GetAccData),
			InstanceMethod("getHrvData", &BufferIterator::GetHrvData),
			// HSLContactSensorStatus
			StaticValue("BufferType_HRData", Napi::Number::New(env, HSLBufferType_HRData)),
			StaticValue("BufferType_ECGData", Napi::Number::New(env, HSLBufferType_ECGData)),
			StaticValue("BufferType_PPGData", Napi::Number::New(env, HSLBufferType_PPGData)),
			StaticValue("BufferType_PPIData", Napi::Number::New(env, HSLBufferType_PPIData)),
			StaticValue("BufferType_AccData", Napi::Number::New(env, HSLBufferType_AccData)),
			StaticValue("BufferType_HRVData", Napi::Number::New(env, HSLBufferType_HRVData)),
		});

		constructor = Napi::Persistent(ctor);
		constructor.SuppressDestruct();
		exports.Set("BufferIterator", ctor);
	}

private:
	static Napi::FunctionReference constructor;

	HSLBufferIterator m_iterator;
};
Napi::FunctionReference BufferIterator::constructor;

class Sensor : public Napi::ObjectWrap<Sensor>
{
public:
	Sensor(const Napi::CallbackInfo& info)
		: Napi::ObjectWrap<Sensor>(info)
	{
		if (info.Length() >= 1 && info[0].IsExternal())
		{
			m_sensor = info[0].As<Napi::External<HSLSensor>>().Data();
		}
		else
		{
			Napi::TypeError::New(info.Env(), "Argument 0 invalid").ThrowAsJavaScriptException();
		}
	}

	// Create a new item using the constructor stored during Init.
	static Napi::Value CreateNewSensor(const Napi::CallbackInfo& info, HSLSensorID sensor_id)
	{
		// Retrieve the instance data we stored during `Init()`. We only stored the
		// constructor there, so we retrieve it here to create a new instance of the
		// JS class the constructor represents.
		Napi::Env env = info.Env();

		// Fetch the sensor pointer by sensor id
		HSLSensor* sensor = HSL_GetSensor(sensor_id);

		return 
			(sensor != nullptr)
			? constructor.New({Napi::External<HSLSensor>::New(env, sensor)})
			: env.Null();
	}

	Napi::Value GetSensorID(const Napi::CallbackInfo& info)
	{
		return Napi::Number::New(info.Env(), GetSensor()->sensorID);
	}

	Napi::Value GetHeartRateBPM(const Napi::CallbackInfo& info)
	{
		int BPM= GetSensor()->beatsPerMinute;

		return Napi::Number::New(info.Env(), BPM);
	}

	Napi::Value GetDeviceBodyLocation(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.bodyLocation);
	}

	Napi::Value HasCapability(const Napi::CallbackInfo& info)
	{
		REQ_ARGS(1);
		REQ_INT_ARG(0, data_stream_type);
		
		bool bHasCapability = false;

		if (data_stream_type >= 0 && data_stream_type < HSLStreamFlags_COUNT)
		{
			bHasCapability = HSL_BITMASK_GET_FLAG(GetSensor()->deviceInformation.capabilities, data_stream_type);
		}

		return Napi::Boolean::New(info.Env(), bHasCapability);
	}

	Napi::Value GetDeviceFriendlyName(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.deviceFriendlyName);
	}

	Napi::Value GetDevicePath(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.devicePath);
	}

	Napi::Value GetFirmwareRevisionString(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.firmwareRevisionString);
	}

	Napi::Value GetHardwareRevisionString(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.hardwareRevisionString);
	}

	Napi::Value GetManufacturerNameString(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.manufacturerNameString);
	}

	Napi::Value GetModelNumberString(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.modelNumberString);
	}

	Napi::Value GetSerialNumberString(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.serialNumberString);
	}

	Napi::Value GetSoftwareRevisionString(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.softwareRevisionString);
	}

	Napi::Value GetSystemIdString(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), GetSensor()->deviceInformation.systemID);
	}

	Napi::Value GetHeartRateBuffer(const Napi::CallbackInfo& info)
	{
		HSLSensorID sensor_id= GetSensor()->sensorID;
		HSLBufferIterator iter = HSL_GetHeartRateBuffer(sensor_id);

		return BufferIterator::CreateNewIterator(info, iter);
	}

	Napi::Value GetHeartECGBuffer(const Napi::CallbackInfo& info)
	{
		HSLSensorID sensor_id= GetSensor()->sensorID;
		HSLBufferIterator iter = HSL_GetHeartECGBuffer(sensor_id);

		return BufferIterator::CreateNewIterator(info, iter);
	}

	Napi::Value GetHeartPPGBuffer(const Napi::CallbackInfo& info)
	{
		HSLSensorID sensor_id= GetSensor()->sensorID;
		HSLBufferIterator iter = HSL_GetHeartPPGBuffer(sensor_id);

		return BufferIterator::CreateNewIterator(info, iter);
	}

	Napi::Value GetHeartPPIBuffer(const Napi::CallbackInfo& info)
	{
		HSLSensorID sensor_id= GetSensor()->sensorID;
		HSLBufferIterator iter = HSL_GetHeartPPIBuffer(sensor_id);

		return BufferIterator::CreateNewIterator(info, iter);
	}

	Napi::Value GetHeartAccBuffer(const Napi::CallbackInfo& info)
	{
		HSLSensorID sensor_id= GetSensor()->sensorID;
		HSLBufferIterator iter = HSL_GetHeartAccBuffer(sensor_id);

		return BufferIterator::CreateNewIterator(info, iter);
	}

	Napi::Value GetHeartHrvBuffer(const Napi::CallbackInfo& info)
	{
		REQ_ARGS(1);
		REQ_INT_ARG(0, filter_type);

		HSLSensorID sensor_id = GetSensor()->sensorID;
		HSLBufferIterator iter = HSL_GetHeartHrvBuffer(sensor_id, (HSLHeartRateVariabityFilterType)filter_type);

		return BufferIterator::CreateNewIterator(info, iter);
	}

	Napi::Value SetDataStreamActive(const Napi::CallbackInfo& info)
	{
		REQ_ARGS(2);
		REQ_INT_ARG(0, data_stream_type);
		REQ_BOOL_ARG(1, want_active);

		bool bSuccess = false;
		HSLSensor* sensor = GetSensor();

		if (data_stream_type >= 0 && data_stream_type < HSLStreamFlags_COUNT)
		{
			bool isActive = HSL_BITMASK_GET_FLAG(sensor->activeDataStreams, data_stream_type);

			if (isActive && !want_active)
			{
				t_hsl_stream_bitmask new_bitmask = HSL_BITMASK_CLEAR_FLAG(sensor->activeDataStreams, data_stream_type);

				HSL_SetActiveSensorDataStreams(sensor->sensorID, new_bitmask);
			}
			else if (!isActive && want_active)
			{
				t_hsl_stream_bitmask new_bitmask = HSL_BITMASK_SET_FLAG(sensor->activeDataStreams, data_stream_type);

				HSL_SetActiveSensorDataStreams(sensor->sensorID, new_bitmask);
			}

			bSuccess = true;
		}

		return Napi::Boolean::New(info.Env(), bSuccess);
	}

	Napi::Value SetFilterStreamActive(const Napi::CallbackInfo& info)
	{
		REQ_ARGS(2);
		REQ_INT_ARG(0, filter_stream_type);
		REQ_BOOL_ARG(1, want_active);

		bool bSuccess = false;
		HSLSensor* sensor = GetSensor();

		if (filter_stream_type >= 0 && filter_stream_type < HRVFilter_COUNT)
		{
			bool isActive = HSL_BITMASK_GET_FLAG(sensor->activeFilterStreams, filter_stream_type);

			if (isActive && !want_active)
			{
				t_hrv_filter_bitmask new_bitmask = HSL_BITMASK_CLEAR_FLAG(sensor->activeFilterStreams, filter_stream_type);

				HSL_SetActiveSensorFilterStreams(sensor->sensorID, new_bitmask);
			}
			else if (!isActive && want_active)
			{
				t_hrv_filter_bitmask new_bitmask = HSL_BITMASK_SET_FLAG(sensor->activeFilterStreams, filter_stream_type);

				HSL_SetActiveSensorFilterStreams(sensor->sensorID, new_bitmask);
			}

			bSuccess = true;
		}

		return Napi::Boolean::New(info.Env(), bSuccess);
	}

	Napi::Value StopAllStreams(const Napi::CallbackInfo& info)
	{
		HSLSensorID sensor_id = GetSensor()->sensorID;

		return Napi::Boolean::New(info.Env(), HSL_StopAllSensorStreams(sensor_id));
	}

	static void Init(Napi::Env env, Napi::Object exports)
	{
		Napi::HandleScope scope(env);

		Napi::Function ctor = DefineClass(env, "Sensor", {			
			InstanceMethod("getSensorID", &Sensor::GetSensorID),
			InstanceMethod("getDeviceBodyLocation", &Sensor::GetDeviceBodyLocation),
			InstanceMethod("hasCapability", &Sensor::HasCapability),
			InstanceMethod("getDeviceFriendlyName", &Sensor::GetDeviceFriendlyName),
			InstanceMethod("getDevicePath", &Sensor::GetDevicePath),
			InstanceMethod("getFirmwareRevisionString", &Sensor::GetFirmwareRevisionString),
			InstanceMethod("getHardwareRevisionString", &Sensor::GetHardwareRevisionString),
			InstanceMethod("getManufacturerNameString", &Sensor::GetManufacturerNameString),
			InstanceMethod("getSerialNumberString", &Sensor::GetSerialNumberString),
			InstanceMethod("getSoftwareRevisionString", &Sensor::GetSoftwareRevisionString),
			InstanceMethod("getSystemIdString", &Sensor::GetSystemIdString),
			InstanceMethod("getHeartRateBPM", &Sensor::GetHeartRateBPM),
			InstanceMethod("getHeartRateBuffer", &Sensor::GetHeartRateBuffer),
			InstanceMethod("getHeartECGBuffer", &Sensor::GetHeartECGBuffer),
			InstanceMethod("getHeartPPGBuffer", &Sensor::GetHeartPPGBuffer),
			InstanceMethod("getHeartPPIBuffer", &Sensor::GetHeartPPIBuffer),
			InstanceMethod("getHeartAccBuffer", &Sensor::GetHeartAccBuffer),
			InstanceMethod("getHeartHrvBuffer", &Sensor::GetHeartHrvBuffer),
			InstanceMethod("setDataStreamActive", &Sensor::SetDataStreamActive),
			InstanceMethod("setFilterStreamActive", &Sensor::SetFilterStreamActive),
			InstanceMethod("stopAllStreams", &Sensor::StopAllStreams),
			// HSLContactSensorStatus
			StaticValue("ContactStatus_Invalid", Napi::Number::New(env, HSLContactStatus_Invalid)),
			StaticValue("ContactStatus_NoContact", Napi::Number::New(env, HSLContactStatus_NoContact)),
			StaticValue("ContactStatus_Contact", Napi::Number::New(env, HSLContactStatus_Contact)),
			// HSLSensorDataStreamFlags
			StaticValue("StreamFlags_HRData", Napi::Number::New(env, HSLStreamFlags_HRData)),
			StaticValue("StreamFlags_ECGData", Napi::Number::New(env, HSLStreamFlags_ECGData)),
			StaticValue("StreamFlags_PPGData", Napi::Number::New(env, HSLStreamFlags_PPGData)),
			StaticValue("StreamFlags_PPIData", Napi::Number::New(env, HSLStreamFlags_PPIData)),
			StaticValue("StreamFlags_AccData", Napi::Number::New(env, HSLStreamFlags_AccData)),
			// HSLHeartRateVariabityFilterType
			StaticValue("HRVFilter_SDNN", Napi::Number::New(env, HRVFilter_SDNN)),
			StaticValue("HRVFilter_RMSSD", Napi::Number::New(env, HRVFilter_RMSSD)),
			StaticValue("HRVFilter_SDSD", Napi::Number::New(env, HRVFilter_SDSD)),
			StaticValue("HRVFilter_NN50", Napi::Number::New(env, HRVFilter_NN50)),
			StaticValue("HRVFilter_pNN50", Napi::Number::New(env, HRVFilter_pNN50)),
			StaticValue("HRVFilter_NN20", Napi::Number::New(env, HRVFilter_NN20)),
			StaticValue("HRVFilter_pNN20", Napi::Number::New(env, HRVFilter_pNN20)),
		});

		constructor = Napi::Persistent(ctor);
		constructor.SuppressDestruct();
		exports.Set("Sensor", ctor);
	}

private:
	static Napi::FunctionReference constructor;

	HSLSensor* GetSensor() const
	{
		return m_sensor;
	}

	HSLSensor* m_sensor;
};
Napi::FunctionReference Sensor::constructor;

class SensorList : public Napi::ObjectWrap<SensorList>
{
public:
	SensorList(const Napi::CallbackInfo& info)
		: Napi::ObjectWrap<SensorList>(info)
	{
		HSL_GetSensorList(&m_sensorList);
	}

	// Create a new item using the constructor stored during Init.
	static Napi::Object CreateNewSensorList(const Napi::CallbackInfo& info)
	{
		return constructor.New({});
	}

	Napi::Value GetHostSerial(const Napi::CallbackInfo& info)
	{
		return Napi::String::New(info.Env(), m_sensorList.hostSerial);
	}

	Napi::Value GetSensorCount(const Napi::CallbackInfo& info)
	{
		return Napi::Number::New(info.Env(), m_sensorList.count);
	}

	Napi::Value GetSensor(const Napi::CallbackInfo& info)
	{
		REQ_ARGS(1);
		REQ_INT_ARG(0, list_index);

		if (list_index >= 0 && list_index < m_sensorList.count)
		{
			HSLSensorListEntry &listEntry= m_sensorList.sensors[list_index];

			return Sensor::CreateNewSensor(info, listEntry.sensorID);
		}
		else
		{
			return info.Env().Null();
		}
	}

	static void Init(Napi::Env env, Napi::Object exports)
	{
		Napi::HandleScope scope(env);

		Napi::Function ctor = DefineClass(env, "SensorList", {
			InstanceMethod("getHostSerial", &SensorList::GetHostSerial),
			InstanceMethod("getSensorCount", &SensorList::GetSensorCount),
			InstanceMethod("getSensor", &SensorList::GetSensor),
		});

		constructor = Napi::Persistent(ctor);
		constructor.SuppressDestruct();
		exports.Set("SensorList", ctor);
	}

private:
	static Napi::FunctionReference constructor;

	HSLSensorList m_sensorList;
};
Napi::FunctionReference SensorList::constructor;

Napi::Object GetSensorList(const Napi::CallbackInfo& info)
{
	return SensorList::CreateNewSensorList(info);
}

class EventMessage : public Napi::ObjectWrap<EventMessage>
{
public:
	EventMessage(const Napi::CallbackInfo& info)
		: Napi::ObjectWrap<EventMessage>(info)
	{
		if (info.Length() >= 1 && info[0].IsExternal())
		{
			Napi::External<HSLEventMessage> inEventMessage = info[0].As<Napi::External<HSLEventMessage>>();

			m_eventMessage = *inEventMessage.Data();
		}
		else
		{
			Napi::TypeError::New(info.Env(), "Argument 0 invalid").ThrowAsJavaScriptException();
		}
	}

	// Create a new item using the constructor stored during Init.
	static Napi::Object CreateNewEventMessage(const Napi::CallbackInfo& info, HSLEventMessage &event_message)
	{
		return constructor.New({Napi::External<HSLEventMessage>::New(info.Env(), &event_message)});
	}

	Napi::Value GetEventType(const Napi::CallbackInfo& info)
	{
		return Napi::Number::New(info.Env(), m_eventMessage.event_type);
	}

	static void Init(Napi::Env env, Napi::Object exports)
	{
		Napi::HandleScope scope(env);

		/**
		* HSLEventType
		*/
		Napi::PropertyDescriptor Event_SensorListUpdated = 
			Napi::PropertyDescriptor::Value("Event_SensorListUpdated", Napi::Number::New(env, HSLEvent_SensorListUpdated));
		exports.DefineProperties({
			Event_SensorListUpdated
		});

		Napi::Function ctor = DefineClass(env, "EventMessage", {
			InstanceMethod("getEventType", &EventMessage::GetEventType),
		});

		// Create a persistent reference to the class constructor. This will allow
		// a function called on a class prototype and a function
		// called on instance of a class to be distinguished from each other.
		constructor = Napi::Persistent(ctor);
		constructor.SuppressDestruct();
		exports.Set("EventMessage", ctor);
	}

private:
	static Napi::FunctionReference constructor;

	HSLEventMessage m_eventMessage;
};
Napi::FunctionReference EventMessage::constructor;

Napi::Value PollNextMessage(const Napi::CallbackInfo& info)
{
	Napi::Env env = info.Env();

	HSLEventMessage mesg;
	if (HSL_PollNextMessage(&mesg))
	{
		return EventMessage::CreateNewEventMessage(info, mesg);
	}
	else
	{
		return env.Null();
	}
}

void Cleanup(void* arg)
{
	HSL_Shutdown();
}

Napi::Object Init(Napi::Env env, Napi::Object exports)
{
	exports.Set("getVersionString", Napi::Function::New(env, GetVersionString));

	exports.Set("update", Napi::Function::New(env, Update));
	exports.Set("updateNoPollEvents", Napi::Function::New(env, UpdateNoPollEvents));
	exports.Set("pollNextMessage", Napi::Function::New(env, PollNextMessage));

	exports.Set("hasSensorListChanged", Napi::Function::New(env, HasSensorListChanged));
	exports.Set("getSensorList", Napi::Function::New(env, GetSensorList));

	BufferIterator::Init(env, exports);
	EventMessage::Init(env, exports);
	Sensor::Init(env, exports);
	SensorList::Init(env, exports);

	HSL_Initialize(HSLLogSeverityLevel::HSLLogSeverityLevel_error);

	napi_add_env_cleanup_hook(env, &Cleanup, nullptr);

	return exports;
}

NODE_API_MODULE(heartsensorlibrary, Init)