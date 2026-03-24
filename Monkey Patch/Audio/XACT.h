#pragma once
#include <mmeapi.h>
namespace XACT
{

	extern void Init();

	enum XAUDIO2_WINDOWS_PROCESSOR_SPECIFIER : __int32
	{
		Processor1 = 0x1,
		Processor2 = 0x2,
		Processor3 = 0x4,
		Processor4 = 0x8,
		Processor5 = 0x10,
		Processor6 = 0x20,
		Processor7 = 0x40,
		Processor8 = 0x80,
		Processor9 = 0x100,
		Processor10 = 0x200,
		Processor11 = 0x400,
		Processor12 = 0x800,
		Processor13 = 0x1000,
		Processor14 = 0x2000,
		Processor15 = 0x4000,
		Processor16 = 0x8000,
		Processor17 = 0x10000,
		Processor18 = 0x20000,
		Processor19 = 0x40000,
		Processor20 = 0x80000,
		Processor21 = 0x100000,
		Processor22 = 0x200000,
		Processor23 = 0x400000,
		Processor24 = 0x800000,
		Processor25 = 0x1000000,
		Processor26 = 0x2000000,
		Processor27 = 0x4000000,
		Processor28 = 0x8000000,
		Processor29 = 0x10000000,
		Processor30 = 0x20000000,
		Processor31 = 0x40000000,
		Processor32 = 0x80000000,
		XAUDIO2_ANY_PROCESSOR = 0xFFFFFFFF,
		XAUDIO2_DEFAULT_PROCESSOR = 0xFFFFFFFF,
	};

	typedef XAUDIO2_WINDOWS_PROCESSOR_SPECIFIER XAUDIO2_PROCESSOR;
	typedef void* IXAudio2EngineCallback;
	typedef void* IXAudio2SourceVoice;
	typedef void* IXAudio2SubmixVoice;
	typedef void* IXAudio2MasteringVoice;
	typedef void* IXAudio2VoiceCallback;
	typedef uintptr_t XAUDIO2_VOICE_SENDS;
	typedef uintptr_t XAUDIO2_EFFECT_CHAIN;

	struct XAUDIO2_PERFORMANCE_DATA
	{
		unsigned __int64 AudioCyclesSinceLastQuery;
		unsigned __int64 TotalCyclesSinceLastQuery;
		unsigned int MinimumCyclesPerQuantum;
		unsigned int MaximumCyclesPerQuantum;
		unsigned int MemoryUsageInBytes;
		unsigned int CurrentLatencyInSamples;
		unsigned int GlitchesSinceEngineStarted;
		unsigned int ActiveSourceVoiceCount;
		unsigned int TotalSourceVoiceCount;
		unsigned int ActiveSubmixVoiceCount;
		unsigned int ActiveResamplerCount;
		unsigned int ActiveMatrixMixCount;
		unsigned int ActiveXmaSourceVoices;
		unsigned int ActiveXmaStreams;
	};

	const struct XAUDIO2_DEBUG_CONFIGURATION
	{
		unsigned int TraceMask;
		unsigned int BreakMask;
		int LogThreadID;
		int LogFileline;
		int LogFunctionName;
		int LogTiming;
	};


	struct WAVEFORMATEXTENSIBLE
	{
		tWAVEFORMATEX Format;
		union samples {
				unsigned __int16 wValidBitsPerSample;
				unsigned __int16 wSamplesPerBlock;
				unsigned __int16 wReserved;

		};
		unsigned int dwChannelMask;
		_GUID SubFormat;
	};


	enum XAUDIO2_DEVICE_ROLE : __int32
	{
		NotDefaultDevice = 0x0,
		DefaultConsoleDevice = 0x1,
		DefaultMultimediaDevice = 0x2,
		DefaultCommunicationsDevice = 0x4,
		DefaultGameDevice = 0x8,
		GlobalDefaultDevice = 0xF,
		InvalidDeviceRole = 0xFFFFFFF0,
	};


	struct XAUDIO2_DEVICE_DETAILS
	{
		wchar_t DeviceID[256];
		wchar_t DisplayName[256];
		XAUDIO2_DEVICE_ROLE Role;
		WAVEFORMATEXTENSIBLE OutputFormat;
	};


}

