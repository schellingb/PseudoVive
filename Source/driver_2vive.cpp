/*
  PseudoVive
  Copyright (C) 2016 Bernhard Schelling

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include "MinHook.inl"
#include "openvr_driver.h"

static const char* STR_MANUFACTURERNAME =  "HTC";
static const char* STR_MODELNUMBER_HMD = "Vive MV HTC LHR-00000000";
static const char* STR_MODELNUMBER_TRACKINGREFERENCE = "HTC V2-XD/XE LHB-00000000";
static const char* STR_MODELNUMBER_CONTROLLERLEFT = "Vive Controller MV HTC LHR-00000000";
static const char* STR_MODELNUMBER_CONTROLLERRIGHT = "Vive Controller MV HTC LHR-00000001";

typedef vr::ETrackedPropertyError (__thiscall *WritePropertyBatch_Type)(vr::IVRProperties* thisptr, vr::PropertyContainerHandle_t ulContainerHandle, vr::PropertyWrite_t *pBatch, uint32_t unBatchEntryCount);
WritePropertyBatch_Type Org_WritePropertyBatch;

struct HookVirtualFunctions
{
	enum { VFTIDX_WRITEPROPERTYBATCH };

	virtual vr::ETrackedPropertyError WritePropertyBatch(vr::PropertyContainerHandle_t ulContainerHandle, vr::PropertyWrite_t *pBatch, uint32_t unBatchEntryCount)
	{
		bool bUpdateModelNumber = false;
		vr::IVRProperties* self = (vr::IVRProperties*)this;
		for (vr::PropertyWrite_t *it = pBatch, *itEnd = it + unBatchEntryCount; it != itEnd; it++)
		{
			if (it->writeType != vr::PropertyWrite_Set) continue;
			else if (it->prop == vr::Prop_ManufacturerName_String) { it->pvBuffer = (void*)STR_MANUFACTURERNAME; it->unBufferSize = (uint32_t)strlen(STR_MANUFACTURERNAME) + 1; }
			else if (it->prop == vr::Prop_ModelNumber_String || it->prop == vr::Prop_DeviceClass_Int32 || it->prop == vr::Prop_ControllerRoleHint_Int32) bUpdateModelNumber = true;
		}

		if (bUpdateModelNumber)
		{
			vr::PropertyWrite_t* PropertyModelNumber = NULL;
			vr::ETrackedDeviceClass DeviceClass = vr::TrackedDeviceClass_Invalid;
			vr::ETrackedControllerRole ControllerRole = vr::TrackedControllerRole_Invalid;
			for (vr::PropertyWrite_t *it = pBatch, *itEnd = it + unBatchEntryCount; it != itEnd; it++)
			{
				if (it->writeType != vr::PropertyWrite_Set) continue;
				else if (it->prop == vr::Prop_ModelNumber_String) PropertyModelNumber = it;
				else if (it->prop == vr::Prop_DeviceClass_Int32) DeviceClass = *(vr::ETrackedDeviceClass*)it->pvBuffer;
				else if (it->prop == vr::Prop_ControllerRoleHint_Int32) ControllerRole = *(vr::ETrackedControllerRole*)it->pvBuffer;
			}
			if (DeviceClass == vr::TrackedDeviceClass_Invalid) DeviceClass = (vr::ETrackedDeviceClass)vr::CVRPropertyHelpers(self).GetInt32Property(ulContainerHandle, vr::Prop_DeviceClass_Int32);
			if (ControllerRole == vr::TrackedControllerRole_Invalid && DeviceClass == vr::TrackedDeviceClass_Controller) ControllerRole = (vr::ETrackedControllerRole)vr::CVRPropertyHelpers(self).GetInt32Property(ulContainerHandle, vr::Prop_ControllerRoleHint_Int32);

			const char* str = STR_MODELNUMBER_HMD;
			if      (DeviceClass == vr::TrackedDeviceClass_TrackingReference)                                                   str = STR_MODELNUMBER_TRACKINGREFERENCE;
			else if (DeviceClass == vr::TrackedDeviceClass_Controller && ControllerRole == vr::TrackedControllerRole_RightHand) str = STR_MODELNUMBER_CONTROLLERRIGHT;
			else if (DeviceClass == vr::TrackedDeviceClass_Controller)                                                          str = STR_MODELNUMBER_CONTROLLERLEFT;

			if (PropertyModelNumber) { PropertyModelNumber->pvBuffer = (void*)str; PropertyModelNumber->unBufferSize = (uint32_t)strlen(str) + 1; }
			else
			{
				vr::PropertyWrite_t batch;
				batch.writeType = vr::PropertyWrite_Set;
				batch.prop = vr::Prop_ModelNumber_String;
				batch.pvBuffer = (void*)str;
				batch.unBufferSize = (uint32_t)strlen(str) + 1;
				batch.unTag = vr::k_unStringPropertyTag;
				Org_WritePropertyBatch(self, ulContainerHandle, &batch, 1);
			}
		}
		return Org_WritePropertyBatch(self, ulContainerHandle, pBatch, unBatchEntryCount);
	}

	static void Hook(void* Object, int ObjectVFTIdx, int OurVFTIdx, void** Out_OriginalFunction)
	{
		const HookVirtualFunctions DummyHookVirtualFunctions;
		void *TrackedDeviceAddedFunc = (*(void***)Object)[ObjectVFTIdx], *Our_TrackedDeviceAdded = (*(void***)&DummyHookVirtualFunctions)[OurVFTIdx];
		MH_STATUS StatusCreate = MH_CreateHook(TrackedDeviceAddedFunc, Our_TrackedDeviceAdded, Out_OriginalFunction);
		if      (StatusCreate == MH_ERROR_ALREADY_CREATED)       { MessageBoxA(NULL, "MH_CreateHook failed", "PseudoVive Error", 0); }
		else if (StatusCreate != MH_OK)                          { MessageBoxA(NULL, "MH_CreateHook failed", "PseudoVive Error", 0); }
		else if (MH_EnableHook(TrackedDeviceAddedFunc) != MH_OK) { MessageBoxA(NULL, "MH_EnableHook failed", "PseudoVive Error", 0); }
	}
};

struct CServerTrackedDeviceProvider : public vr::IServerTrackedDeviceProvider
{
	virtual vr::EVRInitError Init(vr::IVRDriverContext *pDriverContext)
	{
		//initialize minhook
		if (MH_Initialize() != MH_OK) { MessageBoxA(NULL, "MH_Initialize failed", "Oculus Touch Error", 0); return vr::VRInitError_Unknown; }

		VR_INIT_SERVER_DRIVER_CONTEXT(pDriverContext);
		//We hook the 2nd function in the virtual function table of vr::IVRProperties (WritePropertyBatch)
		HookVirtualFunctions::Hook(vr::VRPropertiesRaw(), 1, HookVirtualFunctions::VFTIDX_WRITEPROPERTYBATCH, (void**)&Org_WritePropertyBatch);
		VR_CLEANUP_SERVER_DRIVER_CONTEXT();

		return vr::VRInitError_Unknown; //return error, we don't provide an actual device, we just hook functions
	}
	virtual void Cleanup() { }
	virtual const char * const * GetInterfaceVersions() { return NULL; }
	virtual void RunFrame() { }
	virtual bool ShouldBlockStandbyMode() { return false; }
	virtual void EnterStandby() { }
	virtual void LeaveStandby() { }
};

extern "C" BOOL __stdcall _DllMainCRTStartup(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		//add reference to self so we are kept loaded in the vr server process
		WCHAR modname[MAX_PATH];
		GetModuleFileNameW(hinstDLL, modname, MAX_PATH);
		LoadLibraryW(modname);
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void *HmdDriverFactory(const char *pInterfaceName, int *pReturnCode)
{
	//manual string compare to keep number of linked libraries low
	size_t Len = (pInterfaceName ? strlen(pInterfaceName) : 0), Len2 = strlen(vr::IServerTrackedDeviceProvider_Version);
	if (Len != Len2) return NULL;
	for (size_t i = 0; i != Len && i <= Len; i++) if (pInterfaceName[i] != vr::IServerTrackedDeviceProvider_Version[i]) return NULL;
	static CServerTrackedDeviceProvider Server;
	return &Server;
}

extern "C" int _purecall() { return 0; }
