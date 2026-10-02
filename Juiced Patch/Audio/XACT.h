#pragma once
namespace XACT
{
	extern bool AudioStereoFixEnabled;
	extern bool Audio3DLpfFixEnabled;
	extern bool AudioReverbFixEnabled;
	extern bool AudioLpfDebugLog;
	extern bool AudioSourceReverbEnabled;
	extern float AudioLpfDefaultQ;
	extern float AudioLpfStrength;
	extern float AudioLpfVolumeScale;
	extern float AudioLpfOpenFrequency;
	extern float AudioLpfClosedFrequency;
	extern float AudioLpfShelfDepthToCutoffCompensation;
	extern float AudioLpfCloseTransitionMs;
	extern float AudioLpfOpenTransitionMs;
	extern float AudioReverbWetScale;
	extern float AudioReverbStereoEndpointScale;
	extern float AudioReverbReflectionBoostDb;
	extern float AudioReverbLateBoostDb;
	extern float AudioReverbMinDecayTime;

	extern void Init();
	extern void SaveAudioTuningConfig();
}
