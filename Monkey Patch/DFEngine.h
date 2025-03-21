#pragma once

void set_up_billboard_stuff();

extern int address_offset;

#define offset_addr(x)  ((x)+address_offset)


class CDFObjectInstance
{
public:
	virtual int		GetAbsoluteFilename(void* that, wchar_t* filenamepath_out, int param_3);
	virtual bool	IsCreativeChanged(void* that) { return false; }
	virtual int		UpdateOnEvent(void* that, int param_2, float* param_3);
	virtual int		CopyTexture(void* that, int param_2, int param_3, int param_4, int param_5, int param_6, int param_7) { return 0; }
	virtual int		GetTextureLevelCount(void* that) { return 1; }
	virtual int		GetTextureInfo(void* that, int param_2, int param_3) { return 0; }
	virtual int		GetCustomData(void* that, void* param_2, void* param_3) { return 0; }
	virtual int		GetCreativeInfo(void* that, int param_2) { return 0; }
	virtual int		GetTrackingInfo(void* that, int param_2) { return 0; }
	virtual int		PerformAction(void* that, int param_2, int param_3) { return 0; }
	virtual int		GetId(void* that) { return 0; }
	virtual void* GetClassAtOffset04(void* that) { return 0; }
	virtual int		CallClassMethod10AtOffset08(void* that) { return 0; }
	virtual int		CallClassMethod11AtOffset08(void* that) { return 0; }
	virtual void* CreateInstance(void* that) { return 0; }
	virtual int		ReleaseCurrentCreative(void* that) { return 0; }
	virtual int		RefreshCurrentCreative(void* that) { return 0; }
	virtual int		SetProperty(void* that, int param_2, int param_3) { return 0; }
	virtual int		GetProperty(void* that, int param_2, int param_3) { return 0; }
	virtual int		SetLocalBoundingBox(void* that, float* param_2, float* param_3);
	virtual float* SetLocalLookAt(void* that, float* param_2);
	virtual void	SetWorldMatrix(void* that, float* world_matrix) { return; }
	virtual int		GetContentDeliveryMode(void* that) { return 1; }						// Tells the engine the texture from is a filename
	virtual void* dtor(void* that, char memory_flags) { return that; }

	void* class_offest_04;
	void* class_offset_08;
};

class CDFEngine
{
public:
	virtual int		Start(void* param_1, int version, wchar_t** data_directory); // return fatal error code
	virtual void	Shutdown(void* param_1) { return; }
	virtual int		SetSoundSystem(void* param_1, int param_2, int param_3) { return 0; }
	virtual int		CreateDFObject(void* param_1, char* ObjectIdent, CDFObjectInstance** param_3);
	virtual int		RemoveDFObject(void* param_1, void* param_2) { return 0; }
	virtual int		DestroyAllDfObjects(void* param_1, int param_2) { return 0; }
	virtual int		RefreshCurrentCreatives(void* param_1, int param_2) { return 0; }
	virtual void	Update(void* param_1, float TimeSinceLastUpdate);
	virtual int		StartZone(void* param_1, char* ZoneName);
	virtual void	LeaveZone(void* param_1, char* ZoneName) { return; }
	virtual void	UpdateContentFromServerBlocking(void* param_1) { return; }
	virtual int		GetEngineParameter(void* param_1, char* pParamName, void* pReturnValue, void* param_4) { return 0; }
	virtual int		SetEngineParameter(void* param_1, char* pParamName, void* pParamValue) { return 0; }
	virtual void	SetCameraViewMatrix(void* param_1, float* pViewMatrix);
	virtual void	SetCameraProjMatrix(void* param_1, float* pProjMatrix);
	virtual void	SetNetworkDownloadLimit(void* param_1, int nRate) { return; }
	virtual void	SetNetworkUploadLimit(void* param_1, int nRate) { return; }
	virtual void	SetNetworkEnable(void* param_1, bool bEnable) { return; }
	virtual void* dtor(void* param_1, bool bFreeMemory) { return param_1; }
};

extern "C" __declspec(dllimport) CDFEngine * CreateDFEngine(void);

