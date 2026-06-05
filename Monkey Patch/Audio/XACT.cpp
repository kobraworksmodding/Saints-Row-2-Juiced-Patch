// XACT.cpp (uzis, Scanti, Tervel, Clippy95)
// --------------------
// Created: 22/02/2025

#include "../Patcher/patch.h"
#include "../GameConfig.h"
#include "../SafeWrite.h"
#include "../RPCHandler.h"
#include "../Game/Game.h"
#include "../General/General.h"
#include "../FileLogger.h"
#include <safetyhook.hpp>
#include "Hooking.Patterns.h"
#include "../Game/CrashFixes.h"
#include "..\UtilsGlobal.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#define XACT_STEREO_MATRIX_CHANNELS 6
#define XACT_NEUTRAL_LPF_FREQUENCY 24000.0f
#define XACT_NEUTRAL_LPF_Q 1.0f
#define XACT_LPF_ACTIVE_EPSILON 0.000001f
#define XACT_ENABLE_LPF_DEBUG_LOG 0
#define XACT_LOAD_TUNING_FROM_INI 0

#define XACT_AUDIO_CHANNEL_BASE 0x00EEB8F8u
#define XACT_AUDIO_CHANNEL_SIZE 0x19Cu
#define XACT_AUDIO_CHANNEL_COUNT 0x80u
#define XACT_AUDIO_CHANNEL_END (XACT_AUDIO_CHANNEL_BASE + XACT_AUDIO_CHANNEL_SIZE * XACT_AUDIO_CHANNEL_COUNT)
#define XACT_AUDIO_CHANNEL_CUE_OFFSET 0x08
#define XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET 0x12
#define XACT_AUDIO_CHANNEL_INTERNAL_LPF_OFFSET 0x2C
#define XACT_AUDIO_CHANNEL_USER_LPF_OFFSET 0x30
#define XACT_AUDIO_CHANNEL_REVERB_INTERIOR_OFFSET 0x34
#define XACT_AUDIO_CHANNEL_REVERB_EXTERIOR_OFFSET 0x38
#define XACT_AUDIO_CHANNEL_REVERB_SCALE_OFFSET 0x3C
#define XACT_AUDIO_CHANNEL_CATEGORY_OFFSET 0xA4
#define XACT_AUDIO_CHANNEL_UID_OFFSET 0xB4
#define XACT_AUDIO_CHANNEL_DRY_VOLUME_OFFSET 0xFC
#define XACT_AUDIO_CHANNEL_DRY_VOLUME_SIZE 0x08
#define XACT_AUDIO_CHANNEL_FLAGS_OFFSET 0x194
#define XACT_AUDIO_CHANNEL_FLAGS2_OFFSET 0x196

#define XACT_STACK_SOURCE_CHANNELS_OFFSET 0x04
#define XACT_STACK_DESTINATION_CHANNELS_OFFSET 0x08
#define XACT_STACK_MATRIX_OFFSET 0x0C
#define XACT_STACK_AUDIO_CHANNEL_OFFSET 0x0C

#define XACT_WAVE_FORMAT_CHANNELS_OFFSET 0x02
#define XACT_WAVE_FORMAT_SAMPLE_RATE_OFFSET 0x04
#define XACT_SOURCE_SIZE 0x18
#define XACT_SOURCE_RAW_VOICE_OFFSET 0x04
#define XACT_SOURCE_SUBMIX_VOICE_OFFSET 0x0C
#define XACT_SOURCE_ALT_SUBMIX_VOICE_OFFSET 0x14
#define XACT_CUE_SOUND_OFFSET 0x38
#define XACT_WAVE_SOURCE_VOICE_OFFSET 0x58
#define XACT_SOUND_SIZE 0x3C
#define XACT_WAVE_SIZE 0x5C

#define XACT_LIST_GET_NEXT_OFFSET 0x00005DF0
#define XACT_SOUND_GET_TRACK_LIST_OFFSET 0x00006740
#define XACT_CLIP_GET_BRANCH_OFFSET 0x00020400
#define XACT_TRACK_GET_CLIP_OFFSET 0x00028E20
#define XACT_WAVE_UPDATE_FILTER_OFFSET 0x0002D400
#define XACT_SOURCE_VOICE_INITIALIZE_OFFSET 0x00030B00
#define XACT_SOURCE_SET_SEND_LEVELS_OFFSET 0x00031400

#define XACT_GET_VOICE_DETAILS_INDEX 0
#define XACT_SET_VOLUME_INDEX 10
#define XACT_SET_OUTPUT_MATRIX_INDEX 14
#define XACT_DESTROY_VOICE_INDEX 16
#define XACT_GET_DEVICE_DETAILS_INDEX 4
#define XACT_CREATE_SUBMIX_VOICE_INDEX 9
#define XACT_ENABLE_EFFECT_INDEX 3
#define XACT_SET_EFFECT_PARAMETERS_INDEX 6
#define XACT_RELEASE_INDEX 2
#define XACT_BRANCH_GET_WAVE_INDEX 16
#define XACT_CUE_MATRIX_INDEX 18
#define XACT_CATEGORY_VOLUMES_INDEX 0x21

#define XACT_PLAY_STATUS_SUCCESS 2
#define XACT_AUDIO_CATEGORY_VOLUME_COUNT 4
#define XACT_AUDIO_CATEGORY_SOUND_EFFECTS 0
#define XACT_AUDIO_CATEGORY_MUSIC 2
#define XACT_AUDIO_CATEGORY_AMBIENT 3
#define XACT_LPF_DEBUG_PATH_SOUND (1u << 0)
#define XACT_LPF_DEBUG_PATH_WAVE (1u << 1)
#define XACT_LPF_DEBUG_PATH_MATRIX (1u << 2)
#define XACT_LPF_DEBUG_PATH_3D_MATRIX (1u << 3)

#define XACT_REVERB_INPUT_CHANNELS 1u
#define XACT_REVERB_ROUTE_CHANNELS XACT_STEREO_MATRIX_CHANNELS
#define XACT_REVERB_FALLBACK_SAMPLE_RATE 48000u
#define XACT_CUE_WET_BUS_MIN_SEND 0.000001f
#define XACT_DEFAULT_REVERB_SCALE_2D 0.55f
#define XACT_DEFAULT_DISTANCE_REVERB_START 2.0f
#define XACT_DEFAULT_DISTANCE_REVERB_MAX 30.0f
#define XACT_REVERB_MAX_SEND 16.0f
#define XACT_REVERB_ROUTE_MAX_SEND 16.0f
#define XACT_REVERB_SAFE_MIN_EFFECT_GAIN_DB -60.0f
#define XACT_REVERB_SAFE_MAX_EFFECT_GAIN_DB 8.0f
#define XACT_REVERB_OUTPUT_VOLUME 2.5f
#define XACT_REVERB_ROUTE_TO_EFFECT_FOLD_SCALE 0.36939806f
#define XACT_REVERB_ROUTE_PROCESSING_STAGE 1u
#define XACT_REVERB_EFFECT_PROCESSING_STAGE 2u
#define XACT_EXTERIOR_SEND_COMPENSATION 3.54813389f
#define XACT_2D_REVERB_SPEAKER_FOLD_COMPENSATION 2.5f
#define XACT_FOLDED_2D_STEREO_PROMOTION_SCALE 0.30f
#define XACT_EXTERIOR_LOW_EQ_GAIN 6u
#define XACT_EXTERIOR_LOW_EQ_CUTOFF 5u

namespace XACT
{
	bool AudioStereoFixEnabled = true;
	bool Audio3DLpfFixEnabled = true;
	bool AudioReverbFixEnabled = true;
	bool AudioLpfDebugLog = true;
	float AudioLpfDefaultQ = 0.707f; // Default : 1.0f
	float AudioLpfStrength = 1.0f;
	float AudioLpfVolumeScale = 3.5f; // Default : 3.5f
	float AudioLpfOpenFrequency = 24000.0f;
	float AudioLpfClosedFrequency = 1000.0f; // Default : 2000.0f
	float AudioLpfShelfDepthToCutoffCompensation = 1.00f; // Default : 1.50f
	float AudioLpfCloseTransitionMs = 100.0f; // Default : 300.0f
	float AudioLpfOpenTransitionMs = 150.0f; // Default : 450.0f
	float AudioReverbWetScale = 1.25f; // Default : 1.0f
	float AudioReverbStereoEndpointScale = 0.35f; // Default : 0.20f
	float AudioReverbReflectionBoostDb = 2.0f; // Default 2.0f
	float AudioReverbLateBoostDb = 1.5f; // Default 0.0f
	float AudioReverbMinDecayTime = 2.0f; // Default 5.0f

	namespace
	{
		struct X3DAudioDSPSettingsLite
		{
			float* pMatrixCoefficients;
			float* pDelayTimes;
			uint32_t SrcChannelCount;
			uint32_t DstChannelCount;
			float LPFDirectCoefficient;
			float LPFReverbCoefficient;
			float ReverbLevel;
			float DopplerFactor;
			float EmitterToListenerAngle;
			float EmitterToListenerDistance;
			float EmitterVelocityComponent;
			float ListenerVelocityComponent;
		};

		struct Stereo3DMatrixCapture
		{
			const uint8_t* ac{};
			float matrix[XACT_STEREO_MATRIX_CHANNELS * 2]{};
			uint32_t src_channel_count{};
			uint32_t dst_channel_count{};
			float lpf_direct_coefficient{};
			float reverb_level{};
			float emitter_to_listener_distance{};
			bool valid{};
		};

		struct PendingXactSourceVoiceDsp
		{
			const uint8_t* ac{};
			float reverb_send{};
			bool strong_2d_send{};
			bool valid{};
		};

		struct NativeReverbOutputFormat
		{
			uint32_t sample_rate{};
			uint16_t channels{};
			uint32_t channel_mask{};
			bool valid{};
		};

		struct LpfTransitionState
		{
			uint32_t channel_uid{};
			float scalar{};
			DWORD update_tick{};
			bool valid{};
		};

		thread_local Stereo3DMatrixCapture gStereo3DMatrixCapture;
		thread_local PendingXactSourceVoiceDsp gPendingXactSourceVoiceDsp;
		NativeReverbOutputFormat gNativeReverbOutputFormat{};
		LpfTransitionState gLpfTransitionStates[XACT_AUDIO_CHANNEL_COUNT]{};
		uint32_t gLpfDebugLoggedPathMask[XACT_AUDIO_CHANNEL_COUNT]{};
		float gLast3DDistanceReverbScalar[XACT_AUDIO_CHANNEL_COUNT]{};
		bool gLast3DDistanceReverbScalarValid[XACT_AUDIO_CHANNEL_COUNT]{};
		SafetyHookInline XACT32SourceVoiceInitializeHook{};

		struct XAudio2EffectChainLite;
		struct XAudio2VoiceSendsLite;

		using ListGetNextFn = void* (__thiscall*)(void*);
		using SoundGetTrackListFn = void* (__thiscall*)(void*);
		using TrackGetClipFn = void* (__thiscall*)(void*);
		using ClipGetBranchToPrepareFn = void* (__thiscall*)(void*);
		using BranchGetWaveFn = void* (__thiscall*)(void*);
		using WaveUpdateFilterFn = int(__thiscall*)(void*, int, uint32_t, uint32_t);
		using VoiceEnableEffectFn = HRESULT(__stdcall*)(void*, uint32_t, uint32_t);
		using VoiceSetEffectParametersFn = HRESULT(__stdcall*)(void*, uint32_t, const void*, uint32_t, uint32_t);
		using RawVoiceGetVoiceDetailsFn = void(__stdcall*)(void*, void*);
		using RawVoiceSetVolumeFn = HRESULT(__stdcall*)(void*, float, uint32_t);
		using CoCreateInstanceFn = HRESULT(WINAPI*)(const GUID&, void*, DWORD, const GUID&, void**);
		using XAudio2CreateSubmixVoiceFn = HRESULT(__stdcall*)(void*, void**, uint32_t, uint32_t, uint32_t, uint32_t, const void*, const XAudio2EffectChainLite*);
		using XAudio2GetDeviceDetailsFn = HRESULT(__stdcall*)(void*, uint32_t, void*);
		using SourceVoiceSetSendLevelsFn = int(__thiscall*)(void*, float*, unsigned int);
		using RawVoiceSetOutputMatrixFn = HRESULT(__stdcall*)(void*, void*, uint32_t, uint32_t, const float*, uint32_t);
		using RawVoiceDestroyFn = void(__stdcall*)(void*);
		using UnknownReleaseFn = ULONG(__stdcall*)(void*);

		uintptr_t GetXactBase();
		bool EnsureNativeReverbSubmix(void* xaudio, uint32_t sample_rate);
		bool ApplyNativeReverbSend(
			void* xact_source_voice,
			float reverb_send,
			uint32_t src_channels = 0,
			bool strong_2d_send = false,
			bool require_raw_bus_matrix = false,
			const uint8_t* ac = nullptr);
		float ComputeNativeReverbBusSend(float reverb_send, bool strong_2d_send);
		float ComputeListenerEnvironmentReverbSend(const uint8_t* ac);
		bool UsesListenerEnvironmentReverb(const uint8_t* ac, bool is_3d);
		void** GetNativeReverbSubmixVoices();
		float* GetNativeReverbZeroSendLevels();
		uint32_t GetNativeReverbSendTargetChannels();
		uint32_t ReadNativeReverbSampleRate(void* xaudio, const void* wave_format);
		float ApplyAudioChannelCategoryToReverbSend(float reverb_send, const uint8_t* ac);

		int __fastcall Xact32SourceVoiceInitializeDetour(
			void* source_voice,
			void*,
			void* xaudio,
			void* wave_format,
			void* voice_callback,
			int no_pitch_shift,
			int /*use_filter*/,
			float pitch,
			void** submix_voices,
			float* send_levels,
			unsigned int send_count,
			unsigned int dst_channel_count,
			void* mastering_voice)
		{
			void** routed_submix_voices = submix_voices;
			float* routed_send_levels = send_levels;
			unsigned int routed_send_count = send_count;
			bool native_bus_routed = false;
			float routed_send_storage[8]{};

			ReadNativeReverbSampleRate(xaudio, wave_format);

			const bool pending_native_reverb = gPendingXactSourceVoiceDsp.valid &&
				gPendingXactSourceVoiceDsp.reverb_send;

			if (pending_native_reverb && xaudio)
			{
				const auto native_reverb_sample_rate = ReadNativeReverbSampleRate(xaudio, wave_format);
				const bool native_reverb_ready = EnsureNativeReverbSubmix(xaudio, native_reverb_sample_rate);
				const float pending_bus_send = native_reverb_ready ?
					ComputeNativeReverbBusSend(
						ApplyAudioChannelCategoryToReverbSend(gPendingXactSourceVoiceDsp.reverb_send, gPendingXactSourceVoiceDsp.ac),
						gPendingXactSourceVoiceDsp.strong_2d_send) :
					0.0f;
				if (native_reverb_ready && send_count == 0)
				{
					routed_submix_voices = GetNativeReverbSubmixVoices();
					routed_send_levels = routed_send_storage;
					routed_send_storage[0] = pending_bus_send;
					routed_send_count = 1;
					native_bus_routed = true;
				}
			}

			const uint32_t src_channels = wave_format ?
				*reinterpret_cast<unsigned short*>(reinterpret_cast<uint8_t*>(wave_format) + XACT_WAVE_FORMAT_CHANNELS_OFFSET) : 0;
			const int result = XACT32SourceVoiceInitializeHook.thiscall<int>(
				source_voice,
				xaudio,
				wave_format,
				voice_callback,
				no_pitch_shift,
				1,
				pitch,
				routed_submix_voices,
				routed_send_levels,
				routed_send_count,
				dst_channel_count,
				mastering_voice);

			if (native_bus_routed && result >= 0)
			{
				const float init_reverb = pending_native_reverb ? gPendingXactSourceVoiceDsp.reverb_send : 0.0f;
				ApplyNativeReverbSend(
					source_voice,
					init_reverb,
					src_channels,
					pending_native_reverb && gPendingXactSourceVoiceDsp.strong_2d_send,
					false,
					gPendingXactSourceVoiceDsp.ac);
			}

			return result;
		}

		float FloatMin(const float a, const float b)
		{
			return a < b ? a : b;
		}

		float FloatMax(const float a, const float b)
		{
			return a > b ? a : b;
		}

		float Clamp01(const float value)
		{
			return FloatMax(0.0f, FloatMin(1.0f, value));
		}

		float ClampGameReverbScalar(const float value)
		{
			if (!(value > 0.0f))
				return 0.0f;

			return Clamp01(value);
		}

		bool AudioDspFixEnabled()
		{
			return Audio3DLpfFixEnabled || AudioReverbFixEnabled;
		}

#pragma pack(push, 1)
		struct WaveFormatExLite
		{
			uint16_t wFormatTag;
			uint16_t nChannels;
			uint32_t nSamplesPerSec;
			uint32_t nAvgBytesPerSec;
			uint16_t nBlockAlign;
			uint16_t wBitsPerSample;
			uint16_t cbSize;
		};

		struct WaveFormatExtensibleLite
		{
			WaveFormatExLite Format;
			uint16_t Samples;
			uint32_t dwChannelMask;
			GUID SubFormat;
		};
#pragma pack(pop)

		static_assert(sizeof(WaveFormatExLite) == 18);
		static_assert(sizeof(WaveFormatExtensibleLite) == 40);

		struct XAudio2DeviceDetailsLite
		{
			wchar_t DeviceID[256];
			wchar_t DisplayName[256];
			uint32_t Role;
			WaveFormatExtensibleLite OutputFormat;
		};

		static_assert(sizeof(XAudio2DeviceDetailsLite) == 1068);

		LONG ReportAudioException(const char* where, EXCEPTION_POINTERS* info)
		{
			const auto* record = info ? info->ExceptionRecord : nullptr;
			const DWORD code = record ? record->ExceptionCode : 0;
			void* const address = record ? record->ExceptionAddress : nullptr;
			const ULONG_PTR detail0 = (record && record->NumberParameters > 0) ? record->ExceptionInformation[0] : 0;
			const ULONG_PTR detail1 = (record && record->NumberParameters > 1) ? record->ExceptionInformation[1] : 0;


			char message[512]{};
			std::snprintf(
				message,
				sizeof(message),
				"SR2 audio SEH caught\n\nwhere: %s\ncode: 0x%08X\naddress: %p\ndetail0: 0x%08X\ndetail1: 0x%08X\n\nThe exception was caught and the game will continue.",
				where ? where : "<unknown>",
				static_cast<unsigned int>(code),
				address,
				static_cast<unsigned int>(detail0),
				static_cast<unsigned int>(detail1));


			static LONG shown{};
			if (InterlockedIncrement(&shown) <= 3)
				MessageBoxA(nullptr, message, "Juiced Patch Audio SEH", MB_ICONWARNING | MB_OK);


			return EXCEPTION_EXECUTE_HANDLER;
		}

		LONG IgnoreExpectedAudioProbeException(const char*, EXCEPTION_POINTERS*)
		{
			return EXCEPTION_EXECUTE_HANDLER;
		}

		uint32_t ClampNativeReverbSampleRate(const uint32_t sample_rate)
		{
			if (sample_rate < 8000 || sample_rate > 192000)
				return XACT_REVERB_FALLBACK_SAMPLE_RATE;
			return sample_rate;
		}

		void CaptureNativeReverbOutputFormat(const WaveFormatExtensibleLite& format)
		{
			const uint32_t sample_rate = ClampNativeReverbSampleRate(format.Format.nSamplesPerSec);
			const uint16_t channels = format.Format.nChannels;
			if (channels < 1 || channels > 8)
				return;

			gNativeReverbOutputFormat.sample_rate = sample_rate;
			gNativeReverbOutputFormat.channels = channels;
			gNativeReverbOutputFormat.channel_mask = format.dwChannelMask;
			gNativeReverbOutputFormat.valid = true;
		}

		bool IsNativeReverbStereoEndpoint()
		{
			if (!gNativeReverbOutputFormat.valid)
				return false;

			if (gNativeReverbOutputFormat.channels <= 1)
				return true;

			const uint32_t channel_mask = gNativeReverbOutputFormat.channel_mask;
			if (channel_mask != 0)
				return (channel_mask & ~0x00000003u) == 0;

			return gNativeReverbOutputFormat.channels <= 2;
		}

		float NativeReverbEndpointWetScale()
		{
			return IsNativeReverbStereoEndpoint() ?
				FloatMax(0.0f, AudioReverbStereoEndpointScale) :
				1.0f;
		}

		uint32_t ReadWaveFormatSampleRate(const void* wave_format)
		{
			if (!wave_format)
				return 0;

			__try
			{
				return ClampNativeReverbSampleRate(
					*reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(wave_format) + XACT_WAVE_FORMAT_SAMPLE_RATE_OFFSET));
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
				return 0;
			}
		}

		uint32_t ReadXAudioOutputSampleRate(void* xaudio)
		{
			if (!xaudio)
				return 0;

			__try
			{
				auto** const vtable = *reinterpret_cast<void***>(xaudio);
				if (!vtable || !vtable[XACT_GET_DEVICE_DETAILS_INDEX])
					return 0;

				XAudio2DeviceDetailsLite details{};
				auto* const get_device_details = reinterpret_cast<XAudio2GetDeviceDetailsFn>(vtable[XACT_GET_DEVICE_DETAILS_INDEX]);
				const HRESULT hr = get_device_details(xaudio, 0, &details);
				if (FAILED(hr))
					return 0;

				CaptureNativeReverbOutputFormat(details.OutputFormat);
				return ClampNativeReverbSampleRate(details.OutputFormat.Format.nSamplesPerSec);
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
				return 0;
			}
		}

		uint32_t ReadNativeReverbSampleRate(void* xaudio, const void* wave_format)
		{
			if (const auto output_rate = ReadXAudioOutputSampleRate(xaudio))
				return output_rate;
			if (const auto wave_rate = ReadWaveFormatSampleRate(wave_format))
				return wave_rate;
			return XACT_REVERB_FALLBACK_SAMPLE_RATE;
		}

		bool IsReadableMemoryRange(const void* ptr, const size_t size)
		{
			if (!ptr || !size)
				return false;

			auto address = reinterpret_cast<uintptr_t>(ptr);
			const auto end = address + size;
			if (end < address)
				return false;

			while (address < end)
			{
				MEMORY_BASIC_INFORMATION mbi{};
				if (!VirtualQuery(reinterpret_cast<const void*>(address), &mbi, sizeof(mbi)))
					return false;

				if (mbi.State != MEM_COMMIT ||
					(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
				{
					return false;
				}

				const DWORD readable =
					PAGE_READONLY |
					PAGE_READWRITE |
					PAGE_WRITECOPY |
					PAGE_EXECUTE_READ |
					PAGE_EXECUTE_READWRITE |
					PAGE_EXECUTE_WRITECOPY;
				if ((mbi.Protect & readable) == 0)
					return false;

				const auto region_end =
					reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
				if (region_end <= address)
					return false;
				address = region_end;
			}

			return true;
		}

		bool IsExecutableMemoryAddress(const void* ptr, const HMODULE expected_module = nullptr)
		{
			if (!ptr)
				return false;

			MEMORY_BASIC_INFORMATION mbi{};
			if (!VirtualQuery(ptr, &mbi, sizeof(mbi)) ||
				mbi.State != MEM_COMMIT ||
				(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0)
			{
				return false;
			}

			const DWORD executable =
				PAGE_EXECUTE |
				PAGE_EXECUTE_READ |
				PAGE_EXECUTE_READWRITE |
				PAGE_EXECUTE_WRITECOPY;
			if ((mbi.Protect & executable) == 0)
				return false;

			return !expected_module || mbi.AllocationBase == expected_module;
		}

		bool IsInvalidFillPointer(const void* ptr)
		{
			const auto value = reinterpret_cast<uintptr_t>(ptr);
			const auto masked = value & 0xFFFFFFF0u;
			return masked == 0xABABABA0u ||
				masked == 0xCDCDCDA0u ||
				masked == 0xDDDDDDA0u ||
				masked == 0xFEEEFEE0u;
		}

		bool IsReadableObjectPointer(const void* ptr, const size_t size = sizeof(void*))
		{
			return ptr &&
				!IsInvalidFillPointer(ptr) &&
				IsReadableMemoryRange(ptr, size);
		}

		bool IsAudioChannelPointer(const uint8_t* ac)
		{
			const auto value = reinterpret_cast<uintptr_t>(ac);
			return value >= XACT_AUDIO_CHANNEL_BASE &&
				value < XACT_AUDIO_CHANNEL_END &&
				((value - XACT_AUDIO_CHANNEL_BASE) % XACT_AUDIO_CHANNEL_SIZE) == 0;
		}

		uint8_t GetAudioChannelSlot(const uint8_t* ac)
		{
			return static_cast<uint8_t>((reinterpret_cast<uintptr_t>(ac) - XACT_AUDIO_CHANNEL_BASE) / XACT_AUDIO_CHANNEL_SIZE);
		}

		float ReadChannelFloat(const uint8_t* ac, const size_t offset)
		{
			return *reinterpret_cast<const float*>(ac + offset);
		}

		uint32_t ReadChannelU32(const uint8_t* ac, const size_t offset)
		{
			return *reinterpret_cast<const uint32_t*>(ac + offset);
		}

		float ReadDryVolume(const uint8_t* ac, const size_t index)
		{
			return ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_DRY_VOLUME_OFFSET + index * XACT_AUDIO_CHANNEL_DRY_VOLUME_SIZE);
		}

		uint16_t ReadFlagsWord(const uint8_t* ac, const size_t offset)
		{
			return *reinterpret_cast<const uint16_t*>(ac + offset);
		}

		uint32_t ReadAudioChannelMap(const uint8_t* ac)
		{
			if (!IsAudioChannelPointer(ac))
				return 0;

			return (ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS_OFFSET) >> 14) & 0x03;
		}

		bool Is3DSource(const uint8_t* ac)
		{
			return (ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS2_OFFSET) & 0x0800) != 0;
		}

		bool Allows2DLpf(const uint8_t* ac)
		{
			return IsAudioChannelPointer(ac) &&
				(ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS_OFFSET) & 0x0010) != 0;
		}

		bool IsAudioChannelIgnoringPause(const uint8_t* ac)
		{
			return IsAudioChannelPointer(ac) &&
				(ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS_OFFSET) & 0x0001) != 0;
		}

		bool IsAudioChannelUsed(const uint8_t* ac)
		{
			return (ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS2_OFFSET) & 0x0400) != 0;
		}

		bool IsCutsceneAudioChannel(const uint8_t* ac)
		{
			return IsAudioChannelPointer(ac) &&
				(ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS_OFFSET) & 0x2000) != 0;
		}

		bool IsCutsceneActive()
		{
			return General::InCutscene && *General::InCutscene;
		}

		uint32_t ReadAudioChannelCategoryIndex(const uint8_t* ac)
		{
			if (!IsAudioChannelPointer(ac))
				return 0;

			return ReadChannelU32(ac, XACT_AUDIO_CHANNEL_CATEGORY_OFFSET);
		}

		float CombineLpfScalars(const float a, const float b, const float c)
		{
			return Clamp01(1.0f - (1.0f - Clamp01(a)) * (1.0f - Clamp01(b)) * (1.0f - Clamp01(c)));
		}

		float ReadGlobalLpfScalar()
		{
			return Clamp01(*reinterpret_cast<const float*>(0x02526AC0));
		}

		float ReadAudioFloat(const uintptr_t address, const float fallback = 0.0f)
		{
			__try
			{
				return *reinterpret_cast<const float*>(address);
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
				return fallback;
			}
		}

		float ReadAudioCategoryVolume(const uint32_t category_index)
		{
			if (category_index >= XACT_AUDIO_CATEGORY_VOLUME_COUNT)
				return 1.0f;

			const float category_volume = Clamp01(ReadAudioFloat(
				0x00EE34D8 + category_index * sizeof(float),
				1.0f));
			if (category_index == XACT_AUDIO_CATEGORY_AMBIENT)
			{
				const float effects_volume = Clamp01(ReadAudioFloat(
					0x00EE34D8 + XACT_AUDIO_CATEGORY_SOUND_EFFECTS * sizeof(float),
					1.0f));
				return category_volume < effects_volume ? category_volume : effects_volume;
			}

			return category_volume;
		}

		bool ShouldSuppressCutsceneWorldAudio(const uint8_t* ac, const uint32_t category_index)
		{
			return IsAudioChannelPointer(ac) &&
				IsCutsceneActive() &&
				Is3DSource(ac) &&
				!IsCutsceneAudioChannel(ac); //&&
				//category_index != XACT_AUDIO_CATEGORY_MUSIC;
		}

		float ReadAudioChannelCategoryVolume(const uint8_t* ac)
		{
			if (!IsAudioChannelPointer(ac))
				return 1.0f;

			const uint32_t category_index = ReadAudioChannelCategoryIndex(ac);
			if (ShouldSuppressCutsceneWorldAudio(ac, category_index))
				return 0.0f;

			return ReadAudioCategoryVolume(category_index);
		}

		float SmoothLpfScalar(const uint8_t* ac, const float target_scalar)
		{
			const float target = Clamp01(target_scalar);
			if (!IsAudioChannelPointer(ac))
				return target;

			auto& state = gLpfTransitionStates[GetAudioChannelSlot(ac)];
			const uint32_t channel_uid = ReadChannelU32(ac, XACT_AUDIO_CHANNEL_UID_OFFSET);
			const DWORD now = GetTickCount();
			if (!state.valid || state.channel_uid != channel_uid)
			{
				state.channel_uid = channel_uid;
				state.scalar = target;
				state.update_tick = now;
				state.valid = true;
				return target;
			}

			const DWORD elapsed_ms = now - state.update_tick;
			state.update_tick = now;
			if (elapsed_ms == 0)
				return state.scalar;

			const float transition_ms = target > state.scalar ?
				AudioLpfCloseTransitionMs :
				AudioLpfOpenTransitionMs;
			if (transition_ms <= 0.0f)
			{
				state.scalar = target;
				return target;
			}

			const float delta = target - state.scalar;
			const float max_step = static_cast<float>(elapsed_ms) / transition_ms;
			if (std::fabs(delta) <= max_step)
				state.scalar = target;
			else
				state.scalar += delta > 0.0f ? max_step : -max_step;

			return Clamp01(state.scalar);
		}

		void ResetLpfDebugPath(const uint8_t* ac, const uint32_t path_bit)
		{
			if (!IsAudioChannelPointer(ac) || path_bit == 0)
				return;

			gLpfDebugLoggedPathMask[GetAudioChannelSlot(ac)] &= ~path_bit;
		}

		bool ShouldLogLpfDebugPath(const uint8_t* ac, const uint32_t path_bit)
		{
			if (!XACT_ENABLE_LPF_DEBUG_LOG)
				return false;

			if (!AudioLpfDebugLog || !IsAudioChannelPointer(ac) || path_bit == 0)
				return false;

			const uint8_t slot = GetAudioChannelSlot(ac);
			if ((gLpfDebugLoggedPathMask[slot] & path_bit) != 0)
				return false;

			gLpfDebugLoggedPathMask[slot] |= path_bit;
			return true;
		}

		void LogLpfDebugPath(
			const char* path,
			const uint32_t path_bit,
			const uint8_t* ac,
			const bool is_3d,
			const float lpf_scalar,
			const float filter_frequency,
			const void* ptr0,
			const void* ptr1,
			const float value0,
			const float value1)
		{
			if (!ShouldLogLpfDebugPath(ac, path_bit))
				return;

			const uint32_t category = ReadAudioChannelCategoryIndex(ac);
			const float category_volume = ReadAudioCategoryVolume(category);
			const float internal_lpf = Clamp01(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_INTERNAL_LPF_OFFSET));
			const float user_lpf = Clamp01(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_USER_LPF_OFFSET));
			const float global_lpf = ReadGlobalLpfScalar();
			const float x3d_lpf = (gStereo3DMatrixCapture.valid && gStereo3DMatrixCapture.ac == ac) ?
				1.0f - Clamp01(gStereo3DMatrixCapture.lpf_direct_coefficient) :
				-1.0f;

			Logger::TypedLog(
				CHN_XACT,
				"LPFDBG {} slot={} ac=0x{:08X} cat={} catvol={:.3f} is3d={} used={} flags194=0x{:04X} flags196=0x{:04X} ch={} chmap={} lpf={:.3f} hz={:.1f} internal={:.3f} user={:.3f} global={:.3f} x3d={:.3f} ptr0=0x{:08X} ptr1=0x{:08X} value0={:.3f} value1={:.3f}\n",
				path,
				static_cast<unsigned int>(GetAudioChannelSlot(ac)),
				static_cast<unsigned int>(reinterpret_cast<uintptr_t>(ac)),
				static_cast<unsigned int>(category),
				category_volume,
				is_3d ? 1 : 0,
				IsAudioChannelUsed(ac) ? 1 : 0,
				static_cast<unsigned int>(ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS_OFFSET)),
				static_cast<unsigned int>(ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS2_OFFSET)),
				static_cast<unsigned int>(*reinterpret_cast<const uint8_t*>(ac + XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET)),
				static_cast<unsigned int>(ReadAudioChannelMap(ac)),
				lpf_scalar,
				filter_frequency,
				internal_lpf,
				user_lpf,
				global_lpf,
				x3d_lpf,
				static_cast<unsigned int>(reinterpret_cast<uintptr_t>(ptr0)),
				static_cast<unsigned int>(reinterpret_cast<uintptr_t>(ptr1)),
				value0,
				value1);
		}

		float ReadReverbScaleFactor2D()
		{
			const float scale = ReadAudioFloat(0x00EE337C, XACT_DEFAULT_REVERB_SCALE_2D);
			if (!(scale > 0.0f))
				return XACT_DEFAULT_REVERB_SCALE_2D;
			return Clamp01(scale);
		}

		float ReadDistanceReverbStart()
		{
			const float start = ReadAudioFloat(0x00EE3380, XACT_DEFAULT_DISTANCE_REVERB_START);
			return start >= 0.0f ? start : XACT_DEFAULT_DISTANCE_REVERB_START;
		}

		float ReadDistanceReverbMax()
		{
			const float max_distance = ReadAudioFloat(0x00EE3384, XACT_DEFAULT_DISTANCE_REVERB_MAX);
			return max_distance >= 0.0f ? max_distance : XACT_DEFAULT_DISTANCE_REVERB_MAX;
		}

		float ComputeDistanceReverbScalar(const float distance)
		{
			const float start = ReadDistanceReverbStart();
			const float max_distance = ReadDistanceReverbMax();

			if (max_distance <= start)
				return 0.0f;

			const float t = Clamp01(
				(FloatMax(0.0f, distance) - start) /
				(max_distance - start));

			return std::sqrt(t);
		}

		float ReadListenerEnvironmentReverbSend()
		{
			const float interior = ClampGameReverbScalar(ReadAudioFloat(0x02526B2C));
			const float exterior = ClampGameReverbScalar(ReadAudioFloat(0x02526B30));
			return FloatMax(interior, exterior);
		}

		bool IsTinnitusAudioChannelByInstance(const uint8_t* ac)
		{
			if (!IsAudioChannelPointer(ac))
				return false;

			__try
			{
				const uint32_t instance = *reinterpret_cast<const uint32_t*>(0x00E9A45C);
				const uint8_t tinnitus_channel = static_cast<uint8_t>(instance);
				const uint32_t tinnitus_uid = instance >> 8;
				if (tinnitus_channel == 0xFF || tinnitus_uid == 0)
					return false;

				return tinnitus_channel == GetAudioChannelSlot(ac) &&
					tinnitus_uid == ReadChannelU32(ac, XACT_AUDIO_CHANNEL_UID_OFFSET);
			}
			__except (ReportAudioException(__FUNCTION__, GetExceptionInformation()))
			{
				return false;
			}
		}

		bool ShouldSuppressGlobalLpfForChannel(const uint8_t* ac, const float global_lpf)
		{
			(void)global_lpf;
			return IsAudioChannelIgnoringPause(ac) ||
				IsTinnitusAudioChannelByInstance(ac);
		}

		float ComputeCurrentLpfScalar(const uint8_t* ac)
		{
			// Native SR2 keeps the 2D per-instance and global LPF routes separate.
			// Only allow_2d_lpf cues consume the internal/user values, while normal
			// 2D cues can still receive world/global muffle. Pause-surviving menu
			// audio bypasses only that global term.
			const bool allow_instance_lpf = Allows2DLpf(ac);
			const float internal_lpf = allow_instance_lpf ?
				Clamp01(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_INTERNAL_LPF_OFFSET)) : 0.0f;
			const float user_lpf = allow_instance_lpf ?
				Clamp01(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_USER_LPF_OFFSET)) : 0.0f;
			const float raw_global_lpf = ReadGlobalLpfScalar();
			const float global_lpf = ShouldSuppressGlobalLpfForChannel(ac, raw_global_lpf) ? 0.0f : raw_global_lpf;
			return SmoothLpfScalar(ac, CombineLpfScalars(internal_lpf, user_lpf, global_lpf));
		}

		float Compute3DLpfScalar(const uint8_t* ac)
		{
			float x3d_lpf = 0.0f;
			if (gStereo3DMatrixCapture.valid && gStereo3DMatrixCapture.ac == ac)
				x3d_lpf = 1.0f - Clamp01(gStereo3DMatrixCapture.lpf_direct_coefficient);

			const float raw_global_lpf = ReadGlobalLpfScalar();
			const float global_lpf = ShouldSuppressGlobalLpfForChannel(ac, raw_global_lpf) ? 0.0f : raw_global_lpf;
			return SmoothLpfScalar(ac, CombineLpfScalars(
				Clamp01(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_INTERNAL_LPF_OFFSET)),
				Clamp01(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_USER_LPF_OFFSET)),
				FloatMax(x3d_lpf, global_lpf)));
		}

		float LpfScalarToFilterFrequency(const float scalar)
		{
			const float clamped = Clamp01(scalar * FloatMax(0.0f, AudioLpfStrength));
			if (clamped <= XACT_LPF_ACTIVE_EPSILON)
				return XACT_NEUTRAL_LPF_FREQUENCY;

			const float cutoff = AudioLpfOpenFrequency + (AudioLpfClosedFrequency - AudioLpfOpenFrequency) * clamped;
			return FloatMax(80.0f, cutoff * std::pow(10.0f, -AudioLpfShelfDepthToCutoffCompensation * clamped));
		}

		bool IsLpfScalarActive(const float scalar)
		{
			return Clamp01(scalar * FloatMax(0.0f, AudioLpfStrength)) > XACT_LPF_ACTIVE_EPSILON;
		}

		float LpfScalarToDryMatrixGain(const float scalar)
		{
			if (!Audio3DLpfFixEnabled ||
				!std::isfinite(scalar) ||
				!std::isfinite(AudioLpfStrength) ||
				!std::isfinite(AudioLpfVolumeScale))
			{
				return 1.0f;
			}

			const float depth = Clamp01(scalar * FloatMax(0.0f, AudioLpfStrength));
			const float full_gain = FloatMax(0.0f, FloatMin(4.0f, AudioLpfVolumeScale));
			return 1.0f + (full_gain - 1.0f) * depth;
		}

		void ApplyLpfDryMatrixGain(
			float* matrix,
			const uint32_t src_channels,
			const uint32_t dst_channels,
			const float scalar)
		{
			// These matrices are rebuilt for every XACT update. Scaling them here
			// avoids retaining or overriding raw source-voice volume state.
			if (!matrix || src_channels < 1 || src_channels > 2 || dst_channels != XACT_STEREO_MATRIX_CHANNELS)
				return;

			const float gain = LpfScalarToDryMatrixGain(scalar);
			if (!std::isfinite(gain) || std::fabs(gain - 1.0f) <= 0.000001f)
				return;

			const size_t coefficient_count = static_cast<size_t>(src_channels) * dst_channels;
			for (size_t i = 0; i < coefficient_count; ++i)
			{
				if (std::isfinite(matrix[i]))
					matrix[i] *= gain;
			}
		}

		uint32_t FloatBits(const float value)
		{
			uint32_t bits{};
			std::memcpy(&bits, &value, sizeof(bits));
			return bits;
		}

		float ComputeChannelReverbSend(const uint8_t* ac)
		{
			const float interior = ClampGameReverbScalar(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_REVERB_INTERIOR_OFFSET));
			const float exterior = ClampGameReverbScalar(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_REVERB_EXTERIOR_OFFSET));
			const float scale = ClampGameReverbScalar(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_REVERB_SCALE_OFFSET));
			return Clamp01(FloatMax(interior, exterior) * scale);
		}

		float Compute3DReverbSend(const uint8_t* ac, const bool use_recent_environment = true)
		{
			float reverb = ComputeChannelReverbSend(ac);

			if (reverb <= XACT_CUE_WET_BUS_MIN_SEND && UsesListenerEnvironmentReverb(ac, true))
				reverb = ComputeListenerEnvironmentReverbSend(ac);

			if (!use_recent_environment)
				return Clamp01(reverb);

			float distance_reverb = 1.0f;

			if (gStereo3DMatrixCapture.valid && gStereo3DMatrixCapture.ac == ac)
			{
				distance_reverb =
					ComputeDistanceReverbScalar(
						gStereo3DMatrixCapture.emitter_to_listener_distance);
			}
			else if (IsAudioChannelPointer(ac))
			{
				const uint8_t slot = GetAudioChannelSlot(ac);

				if (gLast3DDistanceReverbScalarValid[slot])
					distance_reverb = gLast3DDistanceReverbScalar[slot];
			}


			const float distance_scale = 0.50f + 0.50f * Clamp01(distance_reverb);

			reverb *= distance_scale;

			return Clamp01(reverb);
		}

		bool UsesListenerEnvironmentReverb(const uint8_t* ac, const bool is_3d)
		{
			return is_3d || (ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS_OFFSET) & 0x20) != 0;
		}

		bool Is2DListenerReverbChannel(const uint8_t* ac)
		{
			return IsAudioChannelPointer(ac) &&
				!Is3DSource(ac) &&
				(ReadFlagsWord(ac, XACT_AUDIO_CHANNEL_FLAGS_OFFSET) & 0x20) != 0;
		}

		float ComputeListenerEnvironmentReverbSend(const uint8_t* ac)
		{
			const float scale = ClampGameReverbScalar(ReadChannelFloat(ac, XACT_AUDIO_CHANNEL_REVERB_SCALE_OFFSET));
			if (scale <= XACT_CUE_WET_BUS_MIN_SEND)
				return 0.0f;

			return Clamp01(ReadListenerEnvironmentReverbSend() * scale);
		}

		float Compute2DReverbSend(const uint8_t* ac)
		{
			// Initial play hooks run before listener wetness is copied into the audio channel.
			float reverb = UsesListenerEnvironmentReverb(ac, false) ?
				ComputeListenerEnvironmentReverbSend(ac) :
				ComputeChannelReverbSend(ac);
			return Clamp01(reverb * ReadReverbScaleFactor2D());
		}

		float ComputeInitialXactPlayReverbSend(const uint8_t* ac, const bool is_3d)
		{
			if (!is_3d)
				return Compute2DReverbSend(ac);

			return Compute3DReverbSend(ac, false);
		}

		struct XAudio2EffectDescriptorLite
		{
			void* pEffect;
			uint32_t InitialState;
			uint32_t OutputChannels;
		};

		struct XAudio2EffectChainLite
		{
			uint32_t EffectCount;
			XAudio2EffectDescriptorLite* pEffectDescriptors;
		};

		struct XAudio2VoiceSendsLite
		{
			uint32_t SendCount;
			// XAudio 2.3 uses an IXAudio2Voice* array here, not the newer
			// XAUDIO2_SEND_DESCRIPTOR array with Flags + pOutputVoice.
			void** pOutputVoices;
		};

		struct XAudio2VoiceDetailsLite
		{
			uint32_t CreationFlags;
			uint32_t ActiveFlags;
			uint32_t InputChannels;
			uint32_t InputSampleRate;
		};

#pragma pack(push, 1)
		struct XAudio2FxReverbParametersLite
		{
			float WetDryMix;
			uint32_t ReflectionsDelay;
			uint8_t ReverbDelay;
			uint8_t RearDelay;
			uint8_t PositionLeft;
			uint8_t PositionRight;
			uint8_t PositionMatrixLeft;
			uint8_t PositionMatrixRight;
			uint8_t EarlyDiffusion;
			uint8_t LateDiffusion;
			uint8_t LowEQGain;
			uint8_t LowEQCutoff;
			uint8_t HighEQGain;
			uint8_t HighEQCutoff;
			float RoomFilterFreq;
			float RoomFilterMain;
			float RoomFilterHF;
			float ReflectionsGain;
			float ReverbGain;
			float DecayTime;
			float Density;
			float RoomSize;
		};
#pragma pack(pop)
		static_assert(sizeof(XAudio2FxReverbParametersLite) == 52, "XAUDIO2FX_REVERB_PARAMETERS packing mismatch");

		XAudio2FxReverbParametersLite MakeHardcodedReverbParameters(const bool exterior)
		{
			XAudio2FxReverbParametersLite params{};

			const int room = exterior ? -1000 : -10000;
			const int room_hf = exterior ? 0 : -600;
			const int reflections = exterior ? -602 : -400;
			const int reverb = exterior ? -302 : 500;
			const float decay_time = exterior ? 2.91f : 1.10f;
			const float decay_hf_ratio = exterior ? 1.30f : 0.83f;
			const float diffusion = 100.0f;
			const float density = 100.0f;
			const float hf_reference = 5000.0f;
			const uint32_t reflections_delay_ms = exterior ? 15u : 5u;
			const uint8_t reverb_delay_ms = exterior ? 22u : 10u;

			params.WetDryMix = 100.0f;
			params.ReflectionsDelay = reflections_delay_ms;
			params.ReverbDelay = reverb_delay_ms;
			params.RearDelay = 5;
			params.PositionLeft = 6;
			params.PositionRight = 6;
			params.PositionMatrixLeft = 27;
			params.PositionMatrixRight = 27;
			params.EarlyDiffusion = static_cast<uint8_t>(15.0f * diffusion / 100.0f);
			params.LateDiffusion = params.EarlyDiffusion;
			params.LowEQCutoff = 4;
			params.HighEQCutoff = 6;
			params.RoomFilterFreq = hf_reference;

			if (decay_hf_ratio >= 1.0f)
			{
				int index = static_cast<int>(-4.0f * std::log10(decay_hf_ratio));
				if (index < -8)
					index = -8;
				params.LowEQGain = static_cast<uint8_t>(index < 0 ? index + 8 : 8);
				params.HighEQGain = 8;
				params.DecayTime = decay_time * decay_hf_ratio;
			}
			else
			{
				int index = static_cast<int>(4.0f * std::log10(decay_hf_ratio));
				if (index < -8)
					index = -8;
				params.LowEQGain = 8;
				params.HighEQGain = static_cast<uint8_t>(index < 0 ? index + 8 : 8);
				params.DecayTime = decay_time;
			}

			if (exterior)
			{
				params.LowEQGain = XACT_EXTERIOR_LOW_EQ_GAIN;
				params.LowEQCutoff = XACT_EXTERIOR_LOW_EQ_CUTOFF;
			}

			params.RoomFilterMain = static_cast<float>(room) / 100.0f;
			params.RoomFilterHF = static_cast<float>(room_hf) / 100.0f;
			params.ReflectionsGain = static_cast<float>(reflections) / 100.0f;
			params.ReverbGain = static_cast<float>(reverb) / 100.0f;
			params.Density = density;
			params.RoomSize = 100.0f;
			return params;
		}

		float ClampNativeReverbEffectGainDb(const float gain_db)
		{
			return FloatMax(
				XACT_REVERB_SAFE_MIN_EFFECT_GAIN_DB,
				FloatMin(XACT_REVERB_SAFE_MAX_EFFECT_GAIN_DB, gain_db));
		}

		XAudio2FxReverbParametersLite MakeNativeReverbBusParameters()
		{
			auto params = MakeHardcodedReverbParameters(true);
			params.WetDryMix = 100.0f;
			//params.ReflectionsGain = ClampNativeReverbEffectGainDb(params.ReflectionsGain + AudioReverbReflectionBoostDb);
			//params.ReverbGain = ClampNativeReverbEffectGainDb(params.ReverbGain + AudioReverbLateBoostDb);
			//params.DecayTime = FloatMax(params.DecayTime, AudioReverbMinDecayTime);
			return params;
		}

		const GUID* GetAudioReverbClsidForEngine(void* xaudio)
		{
			// XAudio2FX class IDs changed with every legacy XAudio release. Mixing
			// an APO from another release leaves the realtime graph with an
			// incompatible object lifetime (2.3 was previously given the 2.7 APO).
			static const GUID kClsidAudioReverb22 = {
				0x629cf0de, 0x3ecc, 0x41e7, { 0x99, 0x26, 0xf7, 0xe4, 0x3e, 0xeb, 0xec, 0x51 }
			};
			static const GUID kClsidAudioReverb23 = {
				0x9cab402c, 0x1d37, 0x44b4, { 0x88, 0x6d, 0xfa, 0x4f, 0x36, 0x17, 0x0a, 0x4c }
			};
			struct ReverbClass
			{
				const wchar_t* module_name;
				const GUID* clsid;
			};
			const ReverbClass classes[] = {
				{ L"XAudio2_3.dll", &kClsidAudioReverb23 },
				{ L"XAudio2_2.dll", &kClsidAudioReverb22 }
			};

			HMODULE engine_module{};
			__try
			{
				auto** const vtable = xaudio ? *reinterpret_cast<void***>(xaudio) : nullptr;
				if (IsReadableObjectPointer(vtable) &&
					!GetModuleHandleExW(
						GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						reinterpret_cast<LPCWSTR>(vtable),
						&engine_module))
				{
					engine_module = nullptr;
				}
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
				engine_module = nullptr;
			}

			if (!engine_module)
				return nullptr;

			for (const auto& entry : classes)
			{
				if (GetModuleHandleW(entry.module_name) == engine_module)
					return entry.clsid;
			}

			// Multiple legacy XAudio DLLs may coexist in-process. If the object's
			// vtable is not owned by a supported release, fail closed rather than
			// selecting an APO from whichever DLL happened to load first.
			return nullptr;
		}

		HRESULT CreateHardcodedReverbEffect(void* xaudio, void** effect)
		{
			if (!effect)
				return E_POINTER;

			*effect = nullptr;
			const GUID* const reverb_clsid = GetAudioReverbClsidForEngine(xaudio);
			if (!reverb_clsid)
				return E_NOINTERFACE;

			auto* ole32 = GetModuleHandleW(L"ole32.dll");
			if (!ole32)
				ole32 = LoadLibraryW(L"ole32.dll");
			if (!ole32)
				return HRESULT_FROM_WIN32(GetLastError());

			auto* co_create_instance = reinterpret_cast<CoCreateInstanceFn>(GetProcAddress(ole32, "CoCreateInstance"));
			if (!co_create_instance)
				return HRESULT_FROM_WIN32(GetLastError());

			static const GUID kIidIUnknown = {
				0x00000000, 0x0000, 0x0000, { 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46 }
			};

			return co_create_instance(*reverb_clsid, nullptr, 1, kIidIUnknown, effect);
		}

		struct NativeReverbSubmixWrapper
		{
			void** vtable{};
			void* raw_voice{};
		};

		void* __fastcall NativeReverbSubmixGetRaw(NativeReverbSubmixWrapper* self, void*)
		{
			return self ? self->raw_voice : nullptr;
		}

		void* gNativeReverbSubmixVtable[] = {
			nullptr,
			reinterpret_cast<void*>(&NativeReverbSubmixGetRaw)
		};
		NativeReverbSubmixWrapper gNativeReverbSubmixWrapper{ gNativeReverbSubmixVtable, nullptr };
		void* gNativeReverbEffect{};
		void* gNativeReverbSubmixRaw{};
		void* gNativeReverbRouteSubmixRaw{};
		void* gNativeReverbEngine{};
		bool gNativeReverbRouteAvailable{};
		void* gNativeReverbSubmixVoices[] = { &gNativeReverbSubmixWrapper };
		float gNativeReverbZeroSendLevels[] = { 0.0f };
		HRESULT gNativeReverbCreateHr{ E_PENDING };
		HRESULT gNativeReverbParamsHr{ E_PENDING };
		bool gNativeReverbAttempted{};

		void DestroyNativeReverbVoice(void*& voice)
		{
			void* const dying_voice = voice;
			voice = nullptr;
			if (!dying_voice)
				return;

			__try
			{
				auto** const vtable = *reinterpret_cast<void***>(dying_voice);
				if (!IsReadableObjectPointer(vtable, sizeof(void*) * (XACT_DESTROY_VOICE_INDEX + 1)) ||
					!IsExecutableMemoryAddress(vtable[XACT_DESTROY_VOICE_INDEX]))
				{
					return;
				}

				auto* const destroy_voice = reinterpret_cast<RawVoiceDestroyFn>(vtable[XACT_DESTROY_VOICE_INDEX]);
				destroy_voice(dying_voice);
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
			}
		}

		void ReleaseNativeReverbEffectReference()
		{
			void* const effect = gNativeReverbEffect;
			gNativeReverbEffect = nullptr;
			if (!effect)
				return;

			__try
			{
				auto** const vtable = *reinterpret_cast<void***>(effect);
				if (!IsReadableObjectPointer(vtable, sizeof(void*) * (XACT_RELEASE_INDEX + 1)) || !IsExecutableMemoryAddress(vtable[XACT_RELEASE_INDEX]))
					return;

				auto* const release = reinterpret_cast<UnknownReleaseFn>(vtable[XACT_RELEASE_INDEX]);
				release(effect);
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
			}
		}

		void DestroyNativeReverbGraph(const bool allow_retry)
		{
			// XACT has already destroyed every source voice at the normal shutdown
			// hook. Disconnect our wrapper before destroying its two raw buses.
			gNativeReverbSubmixWrapper.raw_voice = nullptr;
			gNativeReverbRouteAvailable = false;
			DestroyNativeReverbVoice(gNativeReverbRouteSubmixRaw);
			DestroyNativeReverbVoice(gNativeReverbSubmixRaw);
			ReleaseNativeReverbEffectReference();
			gNativeReverbEngine = nullptr;
			gNativeReverbAttempted = !allow_retry;
		}


		void** GetNativeReverbSubmixVoices()
		{
			return gNativeReverbSubmixVoices;
		}

		float* GetNativeReverbZeroSendLevels()
		{
			return gNativeReverbZeroSendLevels;
		}

		void* GetNativeReverbSendTargetRaw()
		{
			return (gNativeReverbRouteAvailable && gNativeReverbRouteSubmixRaw) ?
				gNativeReverbRouteSubmixRaw :
				gNativeReverbSubmixRaw;
		}

		uint32_t GetNativeReverbSendTargetChannels()
		{
			return (gNativeReverbRouteAvailable && gNativeReverbRouteSubmixRaw) ?
				XACT_REVERB_ROUTE_CHANNELS :
				XACT_REVERB_INPUT_CHANNELS;
		}

		void WriteNativeReverbRouteToEffectMatrix(float* matrix)
		{
			if (!matrix)
				return;

			for (uint32_t i = 0; i < XACT_REVERB_ROUTE_CHANNELS * XACT_REVERB_INPUT_CHANNELS; ++i)
				matrix[i] = 0.0f;

			if (XACT_REVERB_INPUT_CHANNELS == 1)
			{
				// Keep a full six-speaker 2D wet map at unity instead of
				// overdriving the mono XAudio reverb input over time.
				matrix[0] = 0.5f * XACT_REVERB_ROUTE_TO_EFFECT_FOLD_SCALE;
				matrix[1] = 0.5f * XACT_REVERB_ROUTE_TO_EFFECT_FOLD_SCALE;
				matrix[2] = 0.70710678f * XACT_REVERB_ROUTE_TO_EFFECT_FOLD_SCALE;
				matrix[4] = 0.5f * XACT_REVERB_ROUTE_TO_EFFECT_FOLD_SCALE;
				matrix[5] = 0.5f * XACT_REVERB_ROUTE_TO_EFFECT_FOLD_SCALE;
			}
			else
			{
				matrix[0 * XACT_REVERB_INPUT_CHANNELS + 0] = 1.0f;
				matrix[1 * XACT_REVERB_INPUT_CHANNELS + 1] = 1.0f;
				matrix[2 * XACT_REVERB_INPUT_CHANNELS + 0] = 0.70710678f;
				matrix[2 * XACT_REVERB_INPUT_CHANNELS + 1] = 0.70710678f;
				matrix[4 * XACT_REVERB_INPUT_CHANNELS + 0] = 0.70710678f;
				matrix[5 * XACT_REVERB_INPUT_CHANNELS + 1] = 0.70710678f;
			}
		}

		bool EnsureNativeReverbSubmix(void* xaudio, const uint32_t sample_rate)
		{
			if (!AudioReverbFixEnabled || !xaudio)
				return false;

			if (gNativeReverbSubmixRaw)
			{
				return gNativeReverbEngine == xaudio &&
					gNativeReverbSubmixWrapper.raw_voice != nullptr &&
					GetNativeReverbSendTargetRaw() != nullptr;
			}

			if (gNativeReverbEngine && gNativeReverbEngine != xaudio)
				return false;

			if (gNativeReverbAttempted)
				return false;

			gNativeReverbAttempted = true;
			gNativeReverbEngine = xaudio;
			const auto native_sample_rate = ClampNativeReverbSampleRate(sample_rate);

			gNativeReverbCreateHr = CreateHardcodedReverbEffect(xaudio, &gNativeReverbEffect);
			if (FAILED(gNativeReverbCreateHr) || !gNativeReverbEffect)
			{
				DestroyNativeReverbGraph(false);
				return false;
			}

			XAudio2EffectDescriptorLite descriptor{};
			descriptor.pEffect = gNativeReverbEffect;
			descriptor.InitialState = 1;
			descriptor.OutputChannels = XACT_REVERB_INPUT_CHANNELS;

			XAudio2EffectChainLite chain{};
			chain.EffectCount = 1;
			chain.pEffectDescriptors = &descriptor;

			__try
			{
				auto** const xaudio_vtable = *reinterpret_cast<void***>(xaudio);
				if (!IsReadableObjectPointer(xaudio_vtable, sizeof(void*) * (XACT_CREATE_SUBMIX_VOICE_INDEX + 1)) ||
					!IsExecutableMemoryAddress(xaudio_vtable[XACT_CREATE_SUBMIX_VOICE_INDEX]))
				{
					gNativeReverbCreateHr = E_POINTER;
					DestroyNativeReverbGraph(false);
					return false;
				}

				auto* const create_submix_voice = reinterpret_cast<XAudio2CreateSubmixVoiceFn>(xaudio_vtable[XACT_CREATE_SUBMIX_VOICE_INDEX]);
				gNativeReverbCreateHr = create_submix_voice(
					xaudio,
					&gNativeReverbSubmixRaw,
					XACT_REVERB_INPUT_CHANNELS,
					native_sample_rate,
					0,
					XACT_REVERB_EFFECT_PROCESSING_STAGE,
					nullptr,
					&chain);
				if (FAILED(gNativeReverbCreateHr) || !gNativeReverbSubmixRaw)
				{
					DestroyNativeReverbGraph(false);
					return false;
				}

				const auto params = MakeNativeReverbBusParameters();

				auto** const submix_vtable = *reinterpret_cast<void***>(gNativeReverbSubmixRaw);
				if (!IsReadableObjectPointer(submix_vtable, sizeof(void*) * (XACT_DESTROY_VOICE_INDEX + 1)) ||
					!IsExecutableMemoryAddress(submix_vtable[XACT_ENABLE_EFFECT_INDEX]) ||
					!IsExecutableMemoryAddress(submix_vtable[XACT_SET_EFFECT_PARAMETERS_INDEX]))
				{
					gNativeReverbParamsHr = E_POINTER;
					DestroyNativeReverbGraph(false);
					return false;
				}

				auto* const enable_effect = reinterpret_cast<VoiceEnableEffectFn>(submix_vtable[XACT_ENABLE_EFFECT_INDEX]);
				auto* const set_effect_parameters = reinterpret_cast<VoiceSetEffectParametersFn>(submix_vtable[XACT_SET_EFFECT_PARAMETERS_INDEX]);
				gNativeReverbParamsHr = set_effect_parameters(gNativeReverbSubmixRaw, 0, &params, sizeof(params), 0);
				if (FAILED(gNativeReverbParamsHr))
				{
					DestroyNativeReverbGraph(false);
					return false;
				}

				gNativeReverbParamsHr = enable_effect(gNativeReverbSubmixRaw, 0, 0);
				if (FAILED(gNativeReverbParamsHr))
				{
					DestroyNativeReverbGraph(false);
					return false;
				}
				if (IsExecutableMemoryAddress(submix_vtable[XACT_SET_VOLUME_INDEX]))
				{
					auto* const set_volume = reinterpret_cast<RawVoiceSetVolumeFn>(submix_vtable[XACT_SET_VOLUME_INDEX]);
					set_volume(gNativeReverbSubmixRaw, XACT_REVERB_OUTPUT_VOLUME, 0);
				}

				gNativeReverbRouteAvailable = false;
				gNativeReverbRouteSubmixRaw = nullptr;
				gNativeReverbSubmixWrapper.raw_voice = gNativeReverbSubmixRaw;

				__try
				{
					void* route_output_voices[] = { gNativeReverbSubmixRaw };
					XAudio2VoiceSendsLite route_send_list{};
					route_send_list.SendCount = 1;
					route_send_list.pOutputVoices = route_output_voices;

					gNativeReverbCreateHr = create_submix_voice(
						xaudio,
						&gNativeReverbRouteSubmixRaw,
						XACT_REVERB_ROUTE_CHANNELS,
						native_sample_rate,
						0,
						XACT_REVERB_ROUTE_PROCESSING_STAGE,
						&route_send_list,
						nullptr);
					if (SUCCEEDED(gNativeReverbCreateHr) && gNativeReverbRouteSubmixRaw)
					{
						auto** const route_vtable = *reinterpret_cast<void***>(gNativeReverbRouteSubmixRaw);
						if (IsReadableObjectPointer(route_vtable, sizeof(void*) * (XACT_DESTROY_VOICE_INDEX + 1)) &&
							IsExecutableMemoryAddress(route_vtable[XACT_SET_OUTPUT_MATRIX_INDEX]))
						{
							float route_to_effect_matrix[XACT_REVERB_ROUTE_CHANNELS * XACT_REVERB_INPUT_CHANNELS]{};
							WriteNativeReverbRouteToEffectMatrix(route_to_effect_matrix);
							auto* const set_output_matrix = reinterpret_cast<RawVoiceSetOutputMatrixFn>(route_vtable[XACT_SET_OUTPUT_MATRIX_INDEX]);
							const HRESULT route_matrix_hr = set_output_matrix(
								gNativeReverbRouteSubmixRaw,
								gNativeReverbSubmixRaw,
								XACT_REVERB_ROUTE_CHANNELS,
								XACT_REVERB_INPUT_CHANNELS,
								route_to_effect_matrix,
								0);
							gNativeReverbRouteAvailable = SUCCEEDED(route_matrix_hr);
						}
						if (gNativeReverbRouteAvailable &&
							IsExecutableMemoryAddress(route_vtable[XACT_SET_VOLUME_INDEX]))
						{
							auto* const set_volume = reinterpret_cast<RawVoiceSetVolumeFn>(route_vtable[XACT_SET_VOLUME_INDEX]);
							set_volume(gNativeReverbRouteSubmixRaw, 1.0f, 0);
						}
					}

					if (!gNativeReverbRouteAvailable)
						DestroyNativeReverbVoice(gNativeReverbRouteSubmixRaw);
				}
				__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
				{
					gNativeReverbRouteAvailable = false;
					DestroyNativeReverbVoice(gNativeReverbRouteSubmixRaw);
				}

				gNativeReverbSubmixWrapper.raw_voice = GetNativeReverbSendTargetRaw();
			}
			__except (ReportAudioException(__FUNCTION__, GetExceptionInformation()))
			{
				DestroyNativeReverbGraph(false);
				return false;
			}

			return gNativeReverbSubmixRaw != nullptr && gNativeReverbSubmixWrapper.raw_voice != nullptr;
		}

		bool TryCompute3DReverbStereoPan(
			const uint8_t* ac,
			const uint32_t src_channel,
			float* left,
			float* right)
		{
			if (!left || !right)
				return false;

			*left = 0.0f;
			*right = 0.0f;

			const float center = 0.70710678f;
			const float surround = 0.70710678f;
			float speaker[XACT_STEREO_MATRIX_CHANNELS]{};
			bool valid = false;

			if (gStereo3DMatrixCapture.valid &&
				gStereo3DMatrixCapture.ac == ac &&
				gStereo3DMatrixCapture.dst_channel_count == XACT_STEREO_MATRIX_CHANNELS &&
				src_channel < gStereo3DMatrixCapture.src_channel_count)
			{
				const float* src_matrix = gStereo3DMatrixCapture.matrix + src_channel * XACT_STEREO_MATRIX_CHANNELS;
				for (uint32_t i = 0; i < XACT_STEREO_MATRIX_CHANNELS; ++i)
					speaker[i] = FloatMax(0.0f, src_matrix[i]);
				valid = true;
			}
			else if (IsAudioChannelPointer(ac) && Is3DSource(ac) && src_channel == 0)
			{
				for (uint32_t i = 0; i < XACT_STEREO_MATRIX_CHANNELS; ++i)
					speaker[i] = FloatMax(0.0f, ReadDryVolume(ac, i));
				valid = true;
			}

			if (!valid)
				return false;

			const float mixed_left = speaker[0] + speaker[2] * center + speaker[4] * surround;
			const float mixed_right = speaker[1] + speaker[2] * center + speaker[5] * surround;
			const float energy = std::sqrt(mixed_left * mixed_left + mixed_right * mixed_right);
			if (energy <= XACT_CUE_WET_BUS_MIN_SEND)
				return false;

			*left = mixed_left / energy;
			*right = mixed_right / energy;
			return true;
		}

		void WriteNativeReverbBusMatrix(
			float* matrix,
			const uint8_t* ac,
			const uint32_t src_channels,
			const float send,
			const bool strong_2d_send,
			const uint32_t dst_channels)
		{
			if (!matrix)
				return;

			const uint32_t output_channels =
				(dst_channels >= 1 && dst_channels <= XACT_REVERB_ROUTE_CHANNELS) ?
				dst_channels :
				XACT_REVERB_INPUT_CHANNELS;

			for (uint32_t i = 0; i < src_channels * output_channels; ++i)
				matrix[i] = 0.0f;

			if (output_channels == 1)
			{
				if (src_channels == 1)
				{
					matrix[0] = send;
				}
				else
				{
					const float stereo_send = strong_2d_send ? send : send * 0.70710678f;
					matrix[0] = stereo_send;
					matrix[1] = stereo_send;
					for (uint32_t ch = 2; ch < src_channels; ++ch)
						matrix[ch] = send * 0.5f;
				}
				return;
			}

			if (output_channels != XACT_REVERB_ROUTE_CHANNELS)
			{
				if (src_channels == 1)
				{
					const float mono_send = strong_2d_send ? send : send * 0.70710678f;
					matrix[0] = mono_send;
					matrix[1] = mono_send;
				}
				else if (src_channels >= 2)
				{
					const float stereo_send = strong_2d_send ? send : send * 0.70710678f;
					matrix[0] = stereo_send;
					matrix[output_channels + 1] = stereo_send;
				}
				return;
			}

			if (strong_2d_send)
			{
				const float per_source_send = send / static_cast<float>(src_channels);
				for (uint32_t ch = 0; ch < src_channels; ++ch)
				{
					for (uint32_t dst = 0; dst < output_channels; ++dst)
						matrix[ch * output_channels + dst] = per_source_send;
				}
				return;
			}

			if (!strong_2d_send &&
				gStereo3DMatrixCapture.valid &&
				gStereo3DMatrixCapture.ac == ac &&
				gStereo3DMatrixCapture.src_channel_count == src_channels &&
				gStereo3DMatrixCapture.dst_channel_count == output_channels)
			{
				bool wrote_spatial_matrix = false;
				for (uint32_t ch = 0; ch < src_channels; ++ch)
				{
					for (uint32_t dst = 0; dst < output_channels; ++dst)
					{
						const float value = gStereo3DMatrixCapture.matrix[ch * output_channels + dst];
						matrix[ch * output_channels + dst] = send * value;
						wrote_spatial_matrix = wrote_spatial_matrix || value != 0.0f;
					}
				}

				if (wrote_spatial_matrix)
					return;
			}

			if (src_channels == 1)
			{
				const float mono_send = strong_2d_send ? send : send * 0.70710678f;
				float pan_left = 1.0f;
				float pan_right = 1.0f;
				if (!strong_2d_send && TryCompute3DReverbStereoPan(ac, 0, &pan_left, &pan_right))
				{
					matrix[0] = send * pan_left;
					matrix[1] = send * pan_right;
				}
				else
				{
					matrix[0] = mono_send;
					matrix[1] = mono_send;
				}
			}
			else if (src_channels >= 2)
			{
				const float stereo_send = strong_2d_send ? send : send * 0.70710678f;
				float pan_left = 1.0f;
				float pan_right = 0.0f;
				if (!strong_2d_send && TryCompute3DReverbStereoPan(ac, 0, &pan_left, &pan_right))
				{
					matrix[0] = send * pan_left;
					matrix[1] = send * pan_right;
				}
				else
				{
					matrix[0] = stereo_send;
				}

				pan_left = 0.0f;
				pan_right = 1.0f;
				if (!strong_2d_send && TryCompute3DReverbStereoPan(ac, 1, &pan_left, &pan_right))
				{
					matrix[output_channels + 0] = send * pan_left;
					matrix[output_channels + 1] = send * pan_right;
				}
				else
				{
					matrix[output_channels + 1] = stereo_send;
				}

				for (uint32_t ch = 2; ch < src_channels; ++ch)
				{
					const uint32_t dst = ch < output_channels ? ch : 0;
					matrix[ch * output_channels + dst] = send * 0.5f;
				}
			}
		}

		uint32_t ReadRawVoiceInputChannels(void* raw_source_voice, const uint32_t fallback_channels)
		{
			const uint32_t fallback = (fallback_channels >= 1 && fallback_channels <= XACT_STEREO_MATRIX_CHANNELS) ? fallback_channels : 0;
			if (!raw_source_voice)
				return fallback;

			__try
			{
				auto** const raw_vtable = *reinterpret_cast<void***>(raw_source_voice);
				if (!IsReadableObjectPointer(raw_vtable) ||
					!IsExecutableMemoryAddress(raw_vtable[XACT_GET_VOICE_DETAILS_INDEX]))
					return fallback;

				XAudio2VoiceDetailsLite details{};
				auto* const get_voice_details = reinterpret_cast<RawVoiceGetVoiceDetailsFn>(raw_vtable[XACT_GET_VOICE_DETAILS_INDEX]);
				get_voice_details(raw_source_voice, &details);
				if (details.InputChannels >= 1 && details.InputChannels <= XACT_STEREO_MATRIX_CHANNELS)
					return details.InputChannels;
			}
			__except (ReportAudioException(__FUNCTION__, GetExceptionInformation()))
			{
			}

			return fallback;
		}

		float ComputeNativeReverbBusSend(const float reverb_send, const bool strong_2d_send)
		{
			const float send = Clamp01(reverb_send);
			const float wet_scale = FloatMax(0.0f, AudioReverbWetScale) * NativeReverbEndpointWetScale();
			if (send <= XACT_CUE_WET_BUS_MIN_SEND || wet_scale <= 0.0f)
				return 0.0f;

			const bool using_route_bus = gNativeReverbRouteAvailable && gNativeReverbRouteSubmixRaw;
			if (using_route_bus)
			{
				return FloatMin(
					XACT_REVERB_ROUTE_MAX_SEND,
					send * wet_scale * XACT_EXTERIOR_SEND_COMPENSATION);
			}

			const float native_bus_gain = XACT_EXTERIOR_SEND_COMPENSATION *
				(strong_2d_send ? XACT_2D_REVERB_SPEAKER_FOLD_COMPENSATION : 1.0f);

			return FloatMin(
				XACT_REVERB_MAX_SEND,
				send * wet_scale * native_bus_gain);
		}

		float ApplyAudioChannelCategoryToReverbSend(const float reverb_send, const uint8_t* ac)
		{
			return Clamp01(reverb_send * ReadAudioChannelCategoryVolume(ac));
		}

		bool ApplyNativeReverbSend(
			void* xact_source_voice,
			const float reverb_send,
			const uint32_t src_channels,
			const bool strong_2d_send,
			const bool require_raw_bus_matrix,
			const uint8_t* ac)
		{
			void* const native_send_target = GetNativeReverbSendTargetRaw();
			const uint32_t native_send_channels = GetNativeReverbSendTargetChannels();
			if (!AudioReverbFixEnabled || !native_send_target || !xact_source_voice)
				return false;
			if (!IsReadableObjectPointer(xact_source_voice, XACT_SOURCE_SIZE))
				return false;

			const auto xact_base = GetXactBase();
			if (!xact_base)
				return false;

			int hr = E_FAIL;
			const float category_reverb_send = ApplyAudioChannelCategoryToReverbSend(reverb_send, ac);
			float send_levels[] = { ComputeNativeReverbBusSend(category_reverb_send, strong_2d_send) };
			void* raw_source_voice{};
			void* raw_submix_voice{};
			void* raw_submix_voice_alt{};
			bool source_has_native_send{};
			HRESULT raw_matrix_hr = E_NOTIMPL;
			bool raw_matrix_attempted = false;

			__try
			{
				raw_source_voice = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(xact_source_voice) + XACT_SOURCE_RAW_VOICE_OFFSET);
				raw_submix_voice = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(xact_source_voice) + XACT_SOURCE_SUBMIX_VOICE_OFFSET);
				raw_submix_voice_alt = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(xact_source_voice) + XACT_SOURCE_ALT_SUBMIX_VOICE_OFFSET);
				source_has_native_send =
					raw_submix_voice == native_send_target ||
					raw_submix_voice_alt == native_send_target;

				if (raw_source_voice && !source_has_native_send)
					return false;

				const bool route_bus_matrix = native_send_channels > XACT_REVERB_INPUT_CHANNELS;
				if (!route_bus_matrix)
				{
					auto* const set_send_levels = reinterpret_cast<SourceVoiceSetSendLevelsFn>(xact_base + XACT_SOURCE_SET_SEND_LEVELS_OFFSET);
					hr = set_send_levels(xact_source_voice, send_levels, 1);
				}

				const uint32_t matrix_src_channels = ReadRawVoiceInputChannels(raw_source_voice, src_channels);
				if (raw_source_voice && source_has_native_send &&
					matrix_src_channels >= 1 && matrix_src_channels <= XACT_STEREO_MATRIX_CHANNELS)
				{
					auto** const raw_vtable = *reinterpret_cast<void***>(raw_source_voice);
					if (IsReadableObjectPointer(raw_vtable, sizeof(void*) * (XACT_SET_OUTPUT_MATRIX_INDEX + 1)) &&
						IsExecutableMemoryAddress(raw_vtable[XACT_SET_OUTPUT_MATRIX_INDEX]))
					{
						float bus_matrix[XACT_STEREO_MATRIX_CHANNELS * XACT_REVERB_ROUTE_CHANNELS]{};
						WriteNativeReverbBusMatrix(bus_matrix, ac, matrix_src_channels, send_levels[0], strong_2d_send, native_send_channels);
						auto* const set_output_matrix = reinterpret_cast<RawVoiceSetOutputMatrixFn>(raw_vtable[XACT_SET_OUTPUT_MATRIX_INDEX]);
						raw_matrix_attempted = true;
						raw_matrix_hr = set_output_matrix(
							raw_source_voice,
							native_send_target,
							matrix_src_channels,
							native_send_channels,
							bus_matrix,
							0);
					}
				}
			}
			__except (ReportAudioException(__FUNCTION__, GetExceptionInformation()))
			{
				return false;
			}

			if (require_raw_bus_matrix && send_levels[0] > 0.0f)
				return raw_matrix_attempted && SUCCEEDED(raw_matrix_hr);
			return SUCCEEDED(hr) || (raw_matrix_attempted && SUCCEEDED(raw_matrix_hr));
		}

		uintptr_t GetXactBase()
		{
			return reinterpret_cast<uintptr_t>(GetModuleHandleW(L"xactengine3_2.dll"));
		}

		bool ApplyXact32ActiveWaveDsp(
			void* sound,
			const uint8_t* ac,
			const float filter_frequency,
			const float filter_q,
			const float reverb_send,
			const bool lpf_active,
			const float lpf_scalar)
		{
			if (!AudioDspFixEnabled())
				return false;

			const auto xact_base = GetXactBase();
			if (!xact_base || !IsReadableObjectPointer(sound, XACT_SOUND_SIZE))
				return false;

			bool applied = false;
			const bool strong_2d_send = reverb_send > XACT_CUE_WET_BUS_MIN_SEND && Is2DListenerReverbChannel(ac);

			__try
			{
				auto* get_track_list = reinterpret_cast<SoundGetTrackListFn>(xact_base + XACT_SOUND_GET_TRACK_LIST_OFFSET);
				auto* list_get_next = reinterpret_cast<ListGetNextFn>(xact_base + XACT_LIST_GET_NEXT_OFFSET);
				auto* get_clip = reinterpret_cast<TrackGetClipFn>(xact_base + XACT_TRACK_GET_CLIP_OFFSET);
				auto* get_branch = reinterpret_cast<ClipGetBranchToPrepareFn>(xact_base + XACT_CLIP_GET_BRANCH_OFFSET);
				auto* wave_update_filter = Audio3DLpfFixEnabled ?
					reinterpret_cast<WaveUpdateFilterFn>(xact_base + XACT_WAVE_UPDATE_FILTER_OFFSET) : nullptr;

				void* const track_list = get_track_list(sound);
				if (!IsReadableObjectPointer(track_list))
					return false;

				for (void* track = list_get_next(track_list), *next = nullptr;
					track && track != track_list;
					track = next)
				{
					if (!IsReadableObjectPointer(track))
						break;

					next = list_get_next(track);

					void* const clip = get_clip(track);
					if (!IsReadableObjectPointer(clip))
						continue;

					void* const branch = get_branch(clip);
					if (!IsReadableObjectPointer(branch))
						continue;

					auto** const vtable = *reinterpret_cast<void***>(branch);
					if (!IsReadableObjectPointer(vtable, sizeof(void*) * (XACT_BRANCH_GET_WAVE_INDEX + 1)) ||
						!IsExecutableMemoryAddress(vtable[XACT_BRANCH_GET_WAVE_INDEX], reinterpret_cast<HMODULE>(xact_base)))
						continue;

					auto* get_wave = reinterpret_cast<BranchGetWaveFn>(vtable[XACT_BRANCH_GET_WAVE_INDEX]);
					void* const wave = get_wave(branch);
					if (!IsReadableObjectPointer(wave, XACT_WAVE_SIZE))
						continue;

					if (wave_update_filter)
					{
						wave_update_filter(wave, 0, FloatBits(filter_frequency), FloatBits(filter_q));
						applied = true;
					}

					void* xact_source_voice = nullptr;
					__try
					{
						xact_source_voice = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(wave) + XACT_WAVE_SOURCE_VOICE_OFFSET);
					}
					__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
					{
						xact_source_voice = nullptr;
					}

					if (Audio3DLpfFixEnabled)
					{
						const bool is_3d_source = Is3DSource(ac);
						const float category_volume = ReadAudioChannelCategoryVolume(ac);
						const float source_lpf_scalar = lpf_active ? lpf_scalar : 0.0f;
						if (lpf_active)
							LogLpfDebugPath("wave", XACT_LPF_DEBUG_PATH_WAVE, ac, is_3d_source, lpf_scalar, filter_frequency, wave, xact_source_voice, category_volume, source_lpf_scalar);
						else
							ResetLpfDebugPath(ac, XACT_LPF_DEBUG_PATH_WAVE);
					}

					if (AudioReverbFixEnabled)
					{
						const uint32_t src_channels = IsAudioChannelPointer(ac) ?
							*reinterpret_cast<const uint8_t*>(ac + XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET) : 0;
						applied = ApplyNativeReverbSend(
							xact_source_voice,
							Clamp01(reverb_send),
							src_channels,
							strong_2d_send,
							false,
							ac) || applied;
					}
				}
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
				return false;
			}

			return applied;
		}

		void EnsureXact32FilterHooks()
		{
			if (!AudioDspFixEnabled())
				return;

			static bool attempted{};
			if (attempted)
				return;

			const auto xact_base = GetXactBase();
			if (!xact_base)
				return;

			attempted = true;
			XACT32SourceVoiceInitializeHook = safetyhook::create_inline(
				reinterpret_cast<void*>(xact_base + XACT_SOURCE_VOICE_INITIALIZE_OFFSET),
				&Xact32SourceVoiceInitializeDetour);
		}

		void* GetCueSound(void* cue)
		{
			if (!cue)
				return nullptr;

			if (!IsReadableMemoryRange(cue, 0x3C))
				return nullptr;

			__try
			{
				auto* const sound = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(cue) + XACT_CUE_SOUND_OFFSET);
				return IsReadableObjectPointer(sound, XACT_SOUND_SIZE) ? sound : nullptr;
			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
				return nullptr;
			}
		}

		void* ReadAudioChannelCue(const uint8_t* ac)
		{
			if (!IsAudioChannelPointer(ac))
				return nullptr;

			if (*reinterpret_cast<const int*>(ac) == -1 || !IsAudioChannelUsed(ac))
				return nullptr;

			__try
			{
				auto* const cue = *reinterpret_cast<void* const*>(ac + XACT_AUDIO_CHANNEL_CUE_OFFSET);
				return IsReadableMemoryRange(cue, 0x3C) ? cue : nullptr;
			}
			__except (ReportAudioException(__FUNCTION__, GetExceptionInformation()))
			{
				return nullptr;
			}
		}

		void ApplySoundDsp(void* sound, const uint8_t* ac, const float filter_frequency, const float reverb_send, const bool lpf_active, const float lpf_scalar)
		{
			if (!AudioDspFixEnabled())
				return;

			const auto xact_base = GetXactBase();
			if (!xact_base || !IsReadableObjectPointer(sound, XACT_SOUND_SIZE))
				return;

			__try
			{
				EnsureXact32FilterHooks();

				const float filter_q = lpf_active ? AudioLpfDefaultQ : XACT_NEUTRAL_LPF_Q;
				if (lpf_active)
					LogLpfDebugPath("sound", XACT_LPF_DEBUG_PATH_SOUND, ac, Is3DSource(ac), lpf_scalar, filter_frequency, sound, nullptr, reverb_send, filter_q);
				else
					ResetLpfDebugPath(ac, XACT_LPF_DEBUG_PATH_SOUND);

				const float active_reverb_send = AudioReverbFixEnabled ? Clamp01(reverb_send) : 0.0f;
				ApplyXact32ActiveWaveDsp(
					sound,
					ac,
					filter_frequency,
					filter_q,
					active_reverb_send,
					Audio3DLpfFixEnabled && lpf_active,
					Audio3DLpfFixEnabled ? lpf_scalar : 0.0f);

			}
			__except (IgnoreExpectedAudioProbeException(__FUNCTION__, GetExceptionInformation()))
			{
			}
		}

		void ApplyLiveCueDsp(void* cue, const uint8_t* ac, const bool is_3d)
		{
			if (!AudioDspFixEnabled())
				return;

			if (!IsAudioChannelPointer(ac))
				return;

			auto* sound = GetCueSound(cue);
			if (!sound)
				return;

			const float lpf = Audio3DLpfFixEnabled ? (is_3d ? Compute3DLpfScalar(ac) : ComputeCurrentLpfScalar(ac)) : 0.0f;
			const float reverb = AudioReverbFixEnabled ? (is_3d ? Compute3DReverbSend(ac) : Compute2DReverbSend(ac)) : 0.0f;
			const bool lpf_active = IsLpfScalarActive(lpf);
			ApplySoundDsp(sound, ac, LpfScalarToFilterFrequency(lpf), reverb, lpf_active, lpf);
		}



		uint8_t* Read2DAudioChannelFromContext(const SafetyHookContext& ctx)
		{
			auto* ac = reinterpret_cast<uint8_t*>(ctx.esi);
			if (IsAudioChannelPointer(ac))
				return ac;

			__try
			{
				auto* stack_ac = *reinterpret_cast<uint8_t**>(ctx.ebp + XACT_STACK_AUDIO_CHANNEL_OFFSET);
				if (IsAudioChannelPointer(stack_ac))
					return stack_ac;
			}
			__except (ReportAudioException(__FUNCTION__, GetExceptionInformation()))
			{
			}

			return ac;
		}

		float ReadPromotedLegacy2DScalar(const uint8_t* ac)
		{
			const float front = Clamp01(ReadDryVolume(ac, 0));
			const float promoted_front = Clamp01(front * 2.0f);
			return FloatMax(Clamp01(ReadDryVolume(ac, 12)), promoted_front);
		}

		float ReadStock2DScalarFromMatrix(const float* matrix)
		{
			if (!matrix)
				return 0.0f;

			float peak = 0.0f;
			float energy = 0.0f;
			float sum_abs = 0.0f;

			for (size_t i = 0; i < XACT_STEREO_MATRIX_CHANNELS * 2; ++i)
			{
				const float value = std::fabs(matrix[i]);
				peak = FloatMax(peak, value);
				energy += value * value;
				sum_abs += value;
			}

			const float stereo_equivalent = std::sqrt(energy * 0.5f);
			const float coherent_equivalent = sum_abs * 0.5f;

			return FloatMax(peak, FloatMax(stereo_equivalent, coherent_equivalent));
		}

		bool IsStock2DMatrixFoldedToCenter(const float* matrix)
		{
			if (!matrix)
				return false;

			const float direct =
				std::fabs(matrix[0]) +
				std::fabs(matrix[3]);
			const float cross =
				std::fabs(matrix[1]) +
				std::fabs(matrix[2]);

			if (cross <= 0.0001f)
				return false;

			if (direct <= 0.0001f)
				return true;

			return cross >= direct * 0.25f;
		}

		bool ShouldSoften2DStereoPromotion(const uint8_t* ac, const float* stock_matrix)
		{
			return ReadAudioChannelMap(ac) == 0 ||
				IsStock2DMatrixFoldedToCenter(stock_matrix);
		}

		bool ShouldFoldStereo3DSource(const uint8_t* ac)
		{
			return IsAudioChannelPointer(ac) &&
				Is3DSource(ac) &&
				*reinterpret_cast<const uint8_t*>(ac + XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET) == 2 &&
				ReadAudioChannelCategoryIndex(ac) != XACT_AUDIO_CATEGORY_MUSIC &&
				ReadAudioChannelMap(ac) == 0;
		}

		float WriteGameScaledStereo3DMatrix(
			float* matrix,
			const uint8_t* ac,
			const bool fold_to_mono)
		{
			float peak = 0.0f;
			for (size_t dst = 0; dst < XACT_STEREO_MATRIX_CHANNELS; ++dst)
			{
				const float left = ReadDryVolume(ac, dst);
				const float right = ReadDryVolume(ac, XACT_STEREO_MATRIX_CHANNELS + dst);
				peak = FloatMax(peak, FloatMax(std::fabs(left), std::fabs(right)));
				if (fold_to_mono)
				{
					const float per_source = (left + right) * 0.25f;
					matrix[dst] = per_source;
					matrix[XACT_STEREO_MATRIX_CHANNELS + dst] = per_source;
				}
				else
				{
					matrix[dst] = left;
					matrix[XACT_STEREO_MATRIX_CHANNELS + dst] = right;
				}
			}

			return peak;
		}

		void ClearStereoMatrix(float* matrix)
		{
			for (size_t i = 0; i < XACT_STEREO_MATRIX_CHANNELS * 2; ++i)
				matrix[i] = 0.0f;
		}

		void WriteDirectStereo2DMatrix(float* matrix, const uint8_t* ac, const float stock_scalar, const float* stock_matrix)
		{
			const float category_volume = ReadAudioChannelCategoryVolume(ac);
			const float legacy_scalar = ReadPromotedLegacy2DScalar(ac);
			const float scalar = FloatMax(legacy_scalar, stock_scalar);
			if (category_volume < 0.999f || legacy_scalar > 0.0f || stock_scalar > 0.0f)
				LogLpfDebugPath("matrix2d", XACT_LPF_DEBUG_PATH_MATRIX, ac, false, 0.0f, 0.0f, matrix, stock_matrix, legacy_scalar, stock_scalar);
			else
				ResetLpfDebugPath(ac, XACT_LPF_DEBUG_PATH_MATRIX);

			ClearStereoMatrix(matrix);

			if (stock_matrix)
			{
				for (size_t i = 0; i < XACT_STEREO_MATRIX_CHANNELS * 2; ++i)
					matrix[i] = stock_matrix[i] * 0.25f;
			}

			const float promoted_scalar = ShouldSoften2DStereoPromotion(ac, stock_matrix) ?
				scalar * XACT_FOLDED_2D_STEREO_PROMOTION_SCALE :
				scalar;
			matrix[0] = FloatMax(matrix[0], promoted_scalar);
			matrix[3] = FloatMax(matrix[3], promoted_scalar);

			// Hack to boost stereo gain to match vanilla audio gain, without this, audio is quiet as all hell
			for (size_t i = 0; i < XACT_STEREO_MATRIX_CHANNELS * 2; ++i)
				matrix[i] *= 1.5f;
		}






		void InstallAudioMixFix()
		{
			if (!AudioStereoFixEnabled && !AudioDspFixEnabled())
				return;

			uint8_t x3d_flags = 0x21;
			if (Audio3DLpfFixEnabled)
				x3d_flags |= 0x04;

			if (AudioDspFixEnabled())
				SafeWrite8(0x00482203, x3d_flags);

			if (AudioStereoFixEnabled || Audio3DLpfFixEnabled)
			{
				static auto xact_2d_matrix_fix = safetyhook::create_mid(0x004825F1, [](SafetyHookContext& ctx)
				{
					if (!AudioStereoFixEnabled && !Audio3DLpfFixEnabled)
						return;

					auto* ac = Read2DAudioChannelFromContext(ctx);
					const auto src_channels = *reinterpret_cast<uint32_t*>(ctx.esp + XACT_STACK_SOURCE_CHANNELS_OFFSET);
					const auto dst_channels = *reinterpret_cast<uint32_t*>(ctx.esp + XACT_STACK_DESTINATION_CHANNELS_OFFSET);
					auto* matrix = *reinterpret_cast<float**>(ctx.esp + XACT_STACK_MATRIX_OFFSET);

					if (!IsAudioChannelPointer(ac) || Is3DSource(ac) || !matrix)
						return;

					const auto channel_count = *reinterpret_cast<uint8_t*>(ac + XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET);
					if (src_channels < 1 || src_channels > 2 ||
						dst_channels != XACT_STEREO_MATRIX_CHANNELS || channel_count != src_channels)
						return;

					if (AudioStereoFixEnabled && src_channels == 2)
					{
						float stock_matrix[XACT_STEREO_MATRIX_CHANNELS * 2]{};
						std::memcpy(stock_matrix, matrix, sizeof(stock_matrix));
						const float stock_scalar = ReadStock2DScalarFromMatrix(matrix);
						WriteDirectStereo2DMatrix(matrix, ac, stock_scalar, stock_matrix);
					}

					if (Audio3DLpfFixEnabled)
						ApplyLpfDryMatrixGain(matrix, src_channels, dst_channels, ComputeCurrentLpfScalar(ac));
				});
			}

			if (AudioDspFixEnabled())
			{
				static auto xact_2d_dsp_fix = safetyhook::create_mid(0x0048254F, [](SafetyHookContext& ctx)
				{
					if (!AudioDspFixEnabled())
						return;

					auto* ac = Read2DAudioChannelFromContext(ctx);
					if (!IsAudioChannelPointer(ac) || Is3DSource(ac))
						return;

					ApplyLiveCueDsp(ReadAudioChannelCue(ac), ac, false);
				});
			}

			if (AudioStereoFixEnabled || AudioDspFixEnabled())
			{
				static auto xact_3d_matrix_capture = safetyhook::create_mid(0x00482220, [](SafetyHookContext& ctx)
				{
					if (!AudioStereoFixEnabled && !AudioDspFixEnabled())
						return;

					auto* ac = reinterpret_cast<uint8_t*>(ctx.esi);
					auto* dsp = reinterpret_cast<X3DAudioDSPSettingsLite*>(ctx.edi);
					if (!IsAudioChannelPointer(ac) || !dsp || !dsp->pMatrixCoefficients)
					{
						gStereo3DMatrixCapture.valid = false;
						return;
					}

					const uint8_t channel_count = *reinterpret_cast<uint8_t*>(ac + XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET);
					if (channel_count < 1 || channel_count > 2 ||
						dsp->SrcChannelCount != channel_count || dsp->DstChannelCount != XACT_STEREO_MATRIX_CHANNELS)
					{
						gStereo3DMatrixCapture.valid = false;
						return;
					}

					gStereo3DMatrixCapture.ac = ac;
					gStereo3DMatrixCapture.src_channel_count = dsp->SrcChannelCount;
					gStereo3DMatrixCapture.dst_channel_count = dsp->DstChannelCount;
					std::fill(
						gStereo3DMatrixCapture.matrix,
						gStereo3DMatrixCapture.matrix + (XACT_STEREO_MATRIX_CHANNELS * 2),
						0.0f);
					std::memcpy(
						gStereo3DMatrixCapture.matrix,
						dsp->pMatrixCoefficients,
						sizeof(float) * dsp->SrcChannelCount * dsp->DstChannelCount);
					gStereo3DMatrixCapture.lpf_direct_coefficient = Clamp01(dsp->LPFDirectCoefficient);
					gStereo3DMatrixCapture.reverb_level = Clamp01(dsp->ReverbLevel);
					gStereo3DMatrixCapture.emitter_to_listener_distance = FloatMax(0.0f, dsp->EmitterToListenerDistance);
					gStereo3DMatrixCapture.valid = true;

					const uint8_t slot = GetAudioChannelSlot(ac);
					gLast3DDistanceReverbScalar[slot] =
						ComputeDistanceReverbScalar(gStereo3DMatrixCapture.emitter_to_listener_distance);
					gLast3DDistanceReverbScalarValid[slot] = true;
				});

				static auto xact_3d_matrix_fix = safetyhook::create_mid(0x00482391, [](SafetyHookContext& ctx)
				{
					if (!AudioStereoFixEnabled && !AudioDspFixEnabled())
						return;

					auto* ac = reinterpret_cast<uint8_t*>(ctx.esi);
					auto* dsp = reinterpret_cast<X3DAudioDSPSettingsLite*>(ctx.edi);
					if (!IsAudioChannelPointer(ac) || !dsp || !dsp->pMatrixCoefficients)
						return;

					if (AudioStereoFixEnabled &&
						*reinterpret_cast<uint8_t*>(ac + XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET) == 2 &&
						dsp->SrcChannelCount == 2 && dsp->DstChannelCount == XACT_STEREO_MATRIX_CHANNELS &&
						gStereo3DMatrixCapture.valid && gStereo3DMatrixCapture.ac == ac)
					{
						if (ShouldFoldStereo3DSource(ac))
						{
							const float matrix_peak =
								WriteGameScaledStereo3DMatrix(dsp->pMatrixCoefficients, ac, true);
							const float lpf_scalar = Compute3DLpfScalar(ac);
							LogLpfDebugPath(
								"matrix3d_fold",
								XACT_LPF_DEBUG_PATH_3D_MATRIX,
								ac,
								true,
								lpf_scalar,
								LpfScalarToFilterFrequency(lpf_scalar),
								dsp->pMatrixCoefficients,
								gStereo3DMatrixCapture.matrix,
								matrix_peak,
								1.0f);
						}
						else
						{
							WriteGameScaledStereo3DMatrix(dsp->pMatrixCoefficients, ac, false);
							ResetLpfDebugPath(ac, XACT_LPF_DEBUG_PATH_3D_MATRIX);
						}
					}

					const auto channel_count = *reinterpret_cast<uint8_t*>(ac + XACT_AUDIO_CHANNEL_CHANNEL_COUNT_OFFSET);
					if (Audio3DLpfFixEnabled && Is3DSource(ac) &&
						dsp->SrcChannelCount >= 1 && dsp->SrcChannelCount <= 2 &&
						dsp->DstChannelCount == XACT_STEREO_MATRIX_CHANNELS &&
						channel_count == dsp->SrcChannelCount)
					{
						ApplyLpfDryMatrixGain(
							dsp->pMatrixCoefficients,
							dsp->SrcChannelCount,
							dsp->DstChannelCount,
							Compute3DLpfScalar(ac));
					}

					if (AudioDspFixEnabled())
						ApplyLiveCueDsp(ReadAudioChannelCue(ac), ac, true);

					if (gStereo3DMatrixCapture.ac == ac)
						gStereo3DMatrixCapture.valid = false;
				});
			}

			auto capture_initial_dsp = [](SafetyHookContext& ctx)
			{
				gPendingXactSourceVoiceDsp = {};
				if (!AudioReverbFixEnabled)
					return;

				auto* ac = reinterpret_cast<uint8_t*>(ctx.esi);
				if (!IsAudioChannelPointer(ac) || !IsAudioChannelUsed(ac))
					return;

				const bool is_3d = Is3DSource(ac);
				const float reverb = ComputeInitialXactPlayReverbSend(ac, is_3d);
				if (reverb <= XACT_CUE_WET_BUS_MIN_SEND)
					return;

				gPendingXactSourceVoiceDsp.ac = ac;
				gPendingXactSourceVoiceDsp.reverb_send = reverb;
				gPendingXactSourceVoiceDsp.strong_2d_send = Is2DListenerReverbChannel(ac);
				gPendingXactSourceVoiceDsp.valid = true;
			};

			if (AudioReverbFixEnabled)
			{
				static auto xact_prepared_play_dsp = safetyhook::create_mid(0x0047EB91, capture_initial_dsp);
				static auto xact_normal_play_dsp = safetyhook::create_mid(0x0047EBA4, capture_initial_dsp);
			}

			if (AudioDspFixEnabled())
			{
				static auto xact_initial_play_dsp = safetyhook::create_mid(0x0047EBA9, [](SafetyHookContext& ctx)
				{
					gPendingXactSourceVoiceDsp = {};
					if (!AudioDspFixEnabled() || ctx.eax != XACT_PLAY_STATUS_SUCCESS)
						return;

					auto* ac = reinterpret_cast<uint8_t*>(ctx.esi);
					if (!IsAudioChannelPointer(ac) || !IsAudioChannelUsed(ac))
						return;

					void* const cue = ReadAudioChannelCue(ac);
					auto* const sound = GetCueSound(cue);
					if (!sound)
						return;

					const bool is_3d = Is3DSource(ac);
					const float lpf = Audio3DLpfFixEnabled ? (is_3d ? Compute3DLpfScalar(ac) : ComputeCurrentLpfScalar(ac)) : 0.0f;
					const float reverb = AudioReverbFixEnabled ? ComputeInitialXactPlayReverbSend(ac, is_3d) : 0.0f;
					const bool lpf_active = IsLpfScalarActive(lpf);

					ApplySoundDsp(sound, ac, LpfScalarToFilterFrequency(lpf), reverb, lpf_active, lpf);
				});

				static auto xact_dsp_shutdown_cleanup = safetyhook::create_mid(0x00481A4C, [](SafetyHookContext&)
				{
					DestroyNativeReverbGraph(true);
					gNativeReverbCreateHr = E_PENDING;
					gNativeReverbParamsHr = E_PENDING;
					gNativeReverbOutputFormat = {};
					gPendingXactSourceVoiceDsp = {};
					gStereo3DMatrixCapture = {};
					for (auto& state : gLpfTransitionStates)
						state = {};
					for (auto& path_mask : gLpfDebugLoggedPathMask)
						path_mask = 0;
					for (auto& scalar : gLast3DDistanceReverbScalar)
						scalar = 0.0f;
					for (auto& valid : gLast3DDistanceReverbScalarValid)
						valid = false;
				});
			}
		}

	}


	void FixAudioHack()
	{
		if (GameConfig::GetValue("Debug", "FixAudio", 0, "Enable directional audio in cutscenes."))
		{
			static auto xact_3d_audio_hack = safetyhook::create_mid(0x0047EA5E, [](SafetyHookContext& ctx)
			{
				if (*General::InCutscene)
				{
					auto* flag = reinterpret_cast<char*>(ctx.esi + XACT_AUDIO_CHANNEL_FLAGS2_OFFSET);
					*flag &= ~0x0001;
				}
			});
		}
	}
	float empty[9]{};
	SAFETYHOOK_NOINLINE float* __fastcall XACT_CCue_GetMatrixCoefficients(DWORD* CCue)
	{
		if (CCue != NULL)
			return (float*)*((DWORD*)CCue + XACT_CUE_MATRIX_INDEX);
		else {
			AssertHandler::AssertOnce("XACT_CCue_GetMatrixCoefficients hook", "CCue is null\n", true);
			return empty;
		}
	}

	float* __fastcall XACT_CGlobalSettings_GetCategoryVolumes(DWORD* CGlobalSettings)
	{
		static float* lastValidPointer = nullptr;

		float* currentPointer = (float*)*((DWORD*)CGlobalSettings + XACT_CATEGORY_VOLUMES_INDEX);

		if (currentPointer != nullptr) {
			lastValidPointer = currentPointer;
			return currentPointer;
		}
		else {
			AssertHandler::AssertOnce("XACT_CGlobalSettings_GetCategoryVolumes hook", "CCue the return is NULL, so we return last valid one.\n", true);
			return lastValidPointer;
		}
	}

	void __cdecl setup_audio_hook() {
		((void(__cdecl*)())0x465020)();
		HMODULE hXAct32 = GetModuleHandle(L"xactengine3_2.dll");

		if (hXAct32 != NULL) {
			if (AudioReverbFixEnabled)
				EnsureXact32FilterHooks();
		}
		else {
			return;
		}
		auto pattern = hook::pattern(hXAct32, "8B FF 55 8B EC 51 89 4D ? 8B 45 ? 8B 40 ? 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC CC 8B FF 55 8B EC 51 89 4D ? 8B 45 ? 66 8B 40 ? 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC CC CC 8B FF 55 8B EC 51");
		if (pattern.empty()) {
			return;
		}

		patchJmp(pattern.get_first<void*>(), XACT_CCue_GetMatrixCoefficients);
		pattern = hook::pattern(hXAct32, "8B FF 55 8B EC 51 89 4D ? 8B 45 ? 8B 80 ? ? ? ? 8B E5 5D C3 CC CC CC CC CC CC CC CC CC CC 8B FF 55 8B EC 51 89 4D ? 8B 45 ? 8B 4D ? 89 88 ? ? ? ? 8B E5 5D C2 ? ? CC CC CC CC CC 8B FF 55 8B EC 51 89 4D ? 8B 45 ? 05");
		patchJmp(pattern.get_first<void*>(), XACT_CGlobalSettings_GetCategoryVolumes);

	}


	void UpdateToNewerXACT()
	{
		int turn = UtilsGlobal::is_wine() ? 2 : 1;
		if (static_cast<int>(GameConfig::GetValue("Audio", "UseFixedXAudio", 1, "Fixes audio playback across the board, Replaces the ingame XAudio 2.2 to XAudio 2.3. (Creds to Scanti)\nSetting this to 1 enables it for Windows Systems, requires 2 for Linux/Wine and proper XACT install (use winetricks or protontricks)")) >= turn) // Scanti the Goat
		{
			// Forces the game to use a newer version of XACT which in turn fixes all of the audio issues
			// in SR2 aside from 3D Panning.
			GUID xaudio = { 0x4c5e637a, 0x16c7, 0x4de3, 0x9c, 0x46, 0x5e, 0xd2, 0x21, 0x81, 0x96, 0x2d };        // version 2.3
			GUID ixaudio = { 0x8bcf1f58, 0x9fe7, 0x4583, 0x8a, 0xc6, 0xe2, 0xad, 0xc4, 0x65, 0xc8, 0xbb };
			SafeWriteBuf((0x00DD8A08), &xaudio, sizeof(xaudio));
			SafeWriteBuf((0x00DD8A18), &ixaudio, sizeof(ixaudio));

			//GUID xact_engine = { 0x94c1affa, 0x66e7, 0x4961, 0x95, 0x21, 0xcf, 0xde, 0xf3, 0x12, 0x8d, 0x4f };
			//SafeWriteBuf((0xDD89AC), &xact_engine, sizeof(xact_engine));
			patchCall((void*)0x51FC96, setup_audio_hook);

		}
	}
	void ChangeSpeakerCount()
	{
		if (GameConfig::GetValue("Audio", "51Surround", 0, "Toggles the games in-built 5.1 Surround Support. (Scanti)") == 1)
			return;

		UINT32 number_of_speakers = 2;
		UINT32 frequency = 48000;
		//SafeWrite8(0x004818E3, number_of_speakers);         // Causes major audio glitches
		SafeWrite8(0x00482B08, number_of_speakers);
		SafeWrite8(0x00482B41, number_of_speakers);
		SafeWrite8(0x00482B96, number_of_speakers);

		SafeWrite32(0x00482B03, frequency);
		SafeWrite32(0x00482B3C, frequency);
		SafeWrite32(0x00482B91, frequency);
	}

	void LoadAudioTuningConfig()
	{
#define LOAD_AUDIO_FLOAT(key, value, comment) value = static_cast<float>(GameConfig::GetDoubleValue("Audio", key, value, comment))
		AudioStereoFixEnabled = GameConfig::GetValue("Audio", "StereoFixEnabled", AudioStereoFixEnabled ? 1u : 0u, "Enable stereo matrix fixes.") != 0;
		Audio3DLpfFixEnabled = GameConfig::GetValue("Audio", "3DLpfFixEnabled", Audio3DLpfFixEnabled ? 1u : 0u, "Enable 3D audio LPF/muffle fixes.") != 0;
		AudioReverbFixEnabled = GameConfig::GetValue("Audio", "ReverbFixEnabled", AudioReverbFixEnabled ? 1u : 0u, "Enable native reverb bus routing.") != 0;
		if (!XACT_LOAD_TUNING_FROM_INI)
			return;
		AudioLpfDebugLog = GameConfig::GetValue("Audio", "LpfDebugLog", AudioLpfDebugLog ? 1u : 0u, "Log one LPF debug line per audio channel/path while LPF is active.") != 0;
		LOAD_AUDIO_FLOAT("LpfDefaultQ", AudioLpfDefaultQ, "XACT low-pass filter Q value.");
		LOAD_AUDIO_FLOAT("LpfStrength", AudioLpfStrength, "Multiplier applied to SR2/X3DAudio LPF scalar before converting to Hz.");
		LOAD_AUDIO_FLOAT("LpfVolumeScale", AudioLpfVolumeScale, "Dry-output gain reached at full LPF/muffle (0 to 4).");
		LOAD_AUDIO_FLOAT("LpfOpenFrequency", AudioLpfOpenFrequency, "Filter cutoff used when no muffling is active.");
		LOAD_AUDIO_FLOAT("LpfClosedFrequency", AudioLpfClosedFrequency, "Base filter cutoff used at full muffling.");
		LOAD_AUDIO_FLOAT("LpfShelfDepthToCutoffCompensation", AudioLpfShelfDepthToCutoffCompensation, "Extra logarithmic cutoff reduction as LPF approaches fully closed.");
		LOAD_AUDIO_FLOAT("LpfCloseTransitionMs", AudioLpfCloseTransitionMs, "Time in milliseconds for LPF to close when entering a muffled environment.");
		LOAD_AUDIO_FLOAT("LpfOpenTransitionMs", AudioLpfOpenTransitionMs, "Time in milliseconds for LPF to open when leaving a muffled environment.");
		LOAD_AUDIO_FLOAT("ReverbWetScale", AudioReverbWetScale, "Scales SR2's reverb send into the native XAudio2 reverb bus send.");
		LOAD_AUDIO_FLOAT("ReverbStereoEndpointScale", AudioReverbStereoEndpointScale, "Extra reverb send scale applied only when the Windows/XAudio endpoint is stereo or mono.");
		LOAD_AUDIO_FLOAT("ReverbReflectionBoostDb", AudioReverbReflectionBoostDb, "Extra dB applied to early reflections on the native reverb bus.");
		LOAD_AUDIO_FLOAT("ReverbLateBoostDb", AudioReverbLateBoostDb, "Extra dB applied to late reverb on the native reverb bus.");
		LOAD_AUDIO_FLOAT("ReverbMinDecayTime", AudioReverbMinDecayTime, "Minimum native reverb bus decay time in seconds.");
		if (std::fabs(AudioReverbReflectionBoostDb - 2.0f) <= 0.001f &&
			std::fabs(AudioReverbLateBoostDb - 15.0f) <= 0.001f)
		{
			AudioReverbReflectionBoostDb = 0.0f;
			AudioReverbLateBoostDb = 0.0f;
		}
#undef LOAD_AUDIO_FLOAT
	}

	void SaveAudioTuningConfig()
	{
#define SAVE_AUDIO_FLOAT(key, value) GameConfig::SetDoubleValue("Audio", key, value)
		GameConfig::SetValue("Audio", "StereoFixEnabled", AudioStereoFixEnabled ? 1u : 0u);
		GameConfig::SetValue("Audio", "3DLpfFixEnabled", Audio3DLpfFixEnabled ? 1u : 0u);
		GameConfig::SetValue("Audio", "ReverbFixEnabled", AudioReverbFixEnabled ? 1u : 0u);
		GameConfig::SetValue("Audio", "LpfDebugLog", AudioLpfDebugLog ? 1u : 0u);
		SAVE_AUDIO_FLOAT("LpfDefaultQ", AudioLpfDefaultQ);
		SAVE_AUDIO_FLOAT("LpfStrength", AudioLpfStrength);
		SAVE_AUDIO_FLOAT("LpfVolumeScale", AudioLpfVolumeScale);
		SAVE_AUDIO_FLOAT("LpfOpenFrequency", AudioLpfOpenFrequency);
		SAVE_AUDIO_FLOAT("LpfClosedFrequency", AudioLpfClosedFrequency);
		SAVE_AUDIO_FLOAT("LpfShelfDepthToCutoffCompensation", AudioLpfShelfDepthToCutoffCompensation);
		SAVE_AUDIO_FLOAT("LpfCloseTransitionMs", AudioLpfCloseTransitionMs);
		SAVE_AUDIO_FLOAT("LpfOpenTransitionMs", AudioLpfOpenTransitionMs);
		SAVE_AUDIO_FLOAT("ReverbWetScale", AudioReverbWetScale);
		SAVE_AUDIO_FLOAT("ReverbStereoEndpointScale", AudioReverbStereoEndpointScale);
		SAVE_AUDIO_FLOAT("ReverbReflectionBoostDb", AudioReverbReflectionBoostDb);
		SAVE_AUDIO_FLOAT("ReverbLateBoostDb", AudioReverbLateBoostDb);
		SAVE_AUDIO_FLOAT("ReverbMinDecayTime", AudioReverbMinDecayTime);
#undef SAVE_AUDIO_FLOAT
	}

	void Init()
	{
		LoadAudioTuningConfig();
		FixAudioHack();
		UpdateToNewerXACT();
		ChangeSpeakerCount();
		if (GameConfig::GetValue("Audio", "FixX360AudioMix", 1, "Fixes stereo, 3D matrix, LPF and reverb routing.") == 1)
		{
			InstallAudioMixFix();
		}

		// removes "FILE_FLAG_NO_BUFFERING" from XACT soundbank createfileA fixes 4kn loading (clippy95)
		Patch<uint32_t>(0x481183 + 1, FILE_FLAG_OVERLAPPED);
	}
}
