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
#define NTDDI_VERSION NTDDI_WIN7
#include "MinHook.inl"
#include "openvr_driver.h"
#include <Psapi.h>

//Type definition of the function we want to hook
typedef uint32_t (*GetStringProp_Type)(vr::ITrackedDeviceServerDriver* thisptr, vr::ETrackedDeviceProperty prop, char *pchValue, uint32_t unBufferSize, vr::ETrackedPropertyError *pError);

//A templated function so we can easily define an array for multiple hooks which use the same master functions
template <size_t idx> static uint32_t GetStringProp_Template(vr::ITrackedDeviceServerDriver* thisptr, vr::ETrackedDeviceProperty prop, char *pchValue, uint32_t unBufferSize, vr::ETrackedPropertyError *pError)
	{ return GetStringProp_Master(idx, thisptr, prop, pchValue, unBufferSize, pError); }

//We define an array for pointers to the original functions and our overrides
enum { MANYHOOK_MAXCOUNT = 10 };
static GetStringProp_Type GetStringProp_Original[MANYHOOK_MAXCOUNT], GetStringProp_Overrides[MANYHOOK_MAXCOUNT] = { 
	GetStringProp_Template<0>, GetStringProp_Template<1>, GetStringProp_Template<2>, GetStringProp_Template<3>, GetStringProp_Template<4>,
	GetStringProp_Template<5>, GetStringProp_Template<6>, GetStringProp_Template<7>, GetStringProp_Template<8>, GetStringProp_Template<9>,
};

//Our implementation of the function returning the strings we want
static uint32_t GetStringProp_Master(size_t HookIndex, vr::ITrackedDeviceServerDriver* thisptr, vr::ETrackedDeviceProperty prop, char *pchValue, uint32_t unBufferSize, vr::ETrackedPropertyError *pError)
{
	//Decide what string to return or if to just call the original drivers get string function
	const char* str = nullptr;
	vr::ETrackedPropertyError Dummy;
	if (prop == vr::Prop_ManufacturerName_String) str = "HTC";
	else if (prop == vr::Prop_ModelNumber_String)
	{
		vr::ETrackedDeviceClass TrackedDeviceClass = (vr::ETrackedDeviceClass)thisptr->GetInt32TrackedDeviceProperty(vr::Prop_DeviceClass_Int32, &Dummy);
		if      (TrackedDeviceClass == vr::TrackedDeviceClass_HMD) str = "Vive MV HTC LHR-00000000";
		else if (TrackedDeviceClass == vr::TrackedDeviceClass_TrackingReference) str = "HTC V2-XD/XE LHB-00000000";
		else if (TrackedDeviceClass == vr::TrackedDeviceClass_Controller)
		{
			vr::ETrackedControllerRole TrackedControllerRole = (vr::ETrackedControllerRole)thisptr->GetInt32TrackedDeviceProperty(vr::Prop_ControllerRoleHint_Int32, &Dummy);
			str = (TrackedControllerRole == vr::TrackedControllerRole_RightHand ? "Vive Controller MV HTC LHR-00000001" : "Vive Controller MV HTC LHR-00000000");
		}
	}
	else return GetStringProp_Original[HookIndex](thisptr, prop, pchValue, unBufferSize, pError);

	//Return the string in str if it fits into the given buffer
	uint32_t len = (uint32_t)strlen(str);
	if (len + 1 > unBufferSize) { if (pError) *pError = vr::TrackedProp_BufferTooSmall; if (unBufferSize) pchValue[0] = '\0'; }
	else { if (pError) *pError = vr::TrackedProp_Success; strcpy(pchValue, str); }
	return len + 1;
}

static void SetupNewHooks()
{
	//Loop through all currently loaded modules
	DWORD ModulesSize = 0;
	HMODULE Modules[256];
	EnumProcessModules(GetCurrentProcess(), Modules, sizeof(Modules), &ModulesSize);
	for (DWORD i = 0; i < (ModulesSize / sizeof(HMODULE)); i++)
	{
		//Get the main driver function HmdDriverFactory from the module if available
		typedef void* (*HmdDriverFactoryFunc)(const char *pInterfaceName, int *pReturnCode);
		HmdDriverFactoryFunc HmdDriverFactoryPtr = (HmdDriverFactoryFunc)GetProcAddress(Modules[i], "HmdDriverFactory");
		if (!HmdDriverFactoryPtr) continue; //no HmdDriverFactory function in this module

		//Get the server tracked device provider from this driver if available
		int ReturnCode;
		vr::IServerTrackedDeviceProvider* ServerProvider = (vr::IServerTrackedDeviceProvider*)HmdDriverFactoryPtr(vr::IServerTrackedDeviceProvider_Version, &ReturnCode);
		if (!ServerProvider) continue; //no ServerTrackedDeviceProvider in this module

		//Loop through all devices this provider offers
		for (int32_t i = 0, iMax = ServerProvider->GetTrackedDeviceCount(); i < iMax; i++)
		{
			vr::ITrackedDeviceServerDriver* Device = ServerProvider->GetTrackedDeviceDriver(i);
			if (!Device) continue;

			//Get the pointer to the GetStringTrackedDeviceProperty function (its the 11th entry in the virtual function table)
			//(Would be better to first check if ServerProvider->GetInterfaceVersions() has IServerTrackedDeviceProvider_Version otherwise this might be wrong)
			GetStringProp_Type GetStringTrackedDevicePropertyFunc = (GetStringProp_Type)(*(void***)Device)[11];

			//Get the index of the next available hook
			int32_t AvailableHook = -1;
			for (int32_t h = 0; h < MANYHOOK_MAXCOUNT; h++)
				if (!GetStringProp_Original[h]) { AvailableHook = h; break; }
			if (AvailableHook == -1) continue;

			//Create and enable the hook to the function of this device
			MH_STATUS StatusCreate = MH_CreateHook(GetStringTrackedDevicePropertyFunc, GetStringProp_Overrides[AvailableHook], (void**)&GetStringProp_Original[AvailableHook]);
			if (StatusCreate == MH_ERROR_ALREADY_CREATED) continue;
			if (StatusCreate != MH_OK) { MessageBoxA(NULL, "MH_CreateHook for GetStringTrackedDeviceProperty failed", "Oculus Touch Error", 0); continue; }
			MH_STATUS StatusEnable = MH_EnableHook(GetStringTrackedDevicePropertyFunc);
			if (StatusEnable != MH_OK) { MessageBoxA(NULL, "MH_EnableHook for GetStringTrackedDeviceProperty failed", "Oculus Touch Error", 0); continue; }
		}
	}
}

extern "C" BOOL __stdcall _DllMainCRTStartup(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		//add reference to self so we are kept loaded in the vr server process
		WCHAR modname[MAX_PATH];
		GetModuleFileNameW(hinstDLL, modname, MAX_PATH);
		LoadLibraryW(modname);

		//initialize minhook
		if (MH_Initialize() != MH_OK) { MessageBoxA(NULL, "MH_Initialize failed", "Oculus Touch Error", 0); return FALSE; }
	}

	//DllMain gets called on a few occasions, every time we check for new hooks we can create
	SetupNewHooks();
	return TRUE;
}
