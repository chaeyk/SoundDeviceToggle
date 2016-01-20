// SoundDeviceToggle.cpp : Defines the entry point for the application.

//

#include "stdafx.h"
#include "PolicyConfig.h"

#define HRFAILED(hr) \
	(FAILED(hr) ? PrintFailed(hr, __FILE__, __LINE__) : false)

#define LFAILED(lResult) \
	((lResult != ERROR_SUCCESS) ? PrintFailed(lResult, __FILE__, __LINE__) : false)

bool PrintFailed(LONG v, const char* file, int line)
{
	CHAR buf[512];
	_snprintf_s(buf, _TRUNCATE, "Failed (%ld) at %s:%d\r\n", v, file, line);
	OutputDebugStringA(buf);
	return true;
}

DWORD GetThisTimeDevice(DWORD dwMax)
{
	LONG lResult;

	HKEY hkBase;
	lResult = RegCreateKeyEx(HKEY_CURRENT_USER, _T("software\\chaeyk\\SoundDeviceToggle"), 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkBase, NULL);
	if (LFAILED(lResult))
		return NULL;

	DWORD dwIndex = (DWORD)-1;
	DWORD dwSize = sizeof(dwIndex);
	lResult = RegGetValue(hkBase, NULL, _T("CurrentIndex"), RRF_RT_DWORD, NULL, &dwIndex, &dwSize);
	if (lResult != ERROR_FILE_NOT_FOUND && LFAILED(lResult))
		return (DWORD)-1;

	if (++dwIndex >= dwMax)
		dwIndex = 0;

	lResult = RegSetValueEx(hkBase, _T("CurrentIndex"), 0, REG_DWORD, (const BYTE*) &dwIndex, sizeof(dwIndex));
	if (LFAILED(lResult))
		return (DWORD)-1;

	RegCloseKey(hkBase);

	return dwIndex;
}

HRESULT SetDefaultAudioPlaybackDevice(LPCWSTR devID)
{
	CComPtr<IPolicyConfigVista> pPolicyConfig;
	ERole reserved = eConsole;

	HRESULT hr = CoCreateInstance(__uuidof(CPolicyConfigVistaClient),
		NULL, CLSCTX_ALL, __uuidof(IPolicyConfigVista), (LPVOID *)& pPolicyConfig);
	if (SUCCEEDED(hr))
	{
		hr = pPolicyConfig->SetDefaultEndpoint(devID, reserved);
	}
	return hr;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		IMMDeviceEnumerator *pEnum = NULL;
		// Create a multimedia device enumerator.
		hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL,
			CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)& pEnum);
		if (HRFAILED(hr))
			return hr;

		CComPtr<IMMDeviceCollection> pDevices;
		// Enumerate the output devices.
		hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
		if (HRFAILED(hr))
			return hr;

		UINT count;
		pDevices->GetCount(&count);
		if (HRFAILED(hr))
			return hr;

		DWORD dwIndex = GetThisTimeDevice(count);
		if (dwIndex == (DWORD)-1) {
			OutputDebugString(_T("Can't get toggle information\r\n"));
			return -1;
		}

		CComPtr<IMMDevice> pDevice;
		hr = pDevices->Item(dwIndex, &pDevice);
		if (HRFAILED(hr))
			return -1;

		LPWSTR wstrID = NULL;
		hr = pDevice->GetId(&wstrID);
		if (HRFAILED(hr))
			return -1;

		CComPtr<IPropertyStore> pStore;
		hr = pDevice->OpenPropertyStore(STGM_READ, &pStore);
		if (HRFAILED(hr))
			return -1;

		PROPVARIANT friendlyName;
		PropVariantInit(&friendlyName);
		hr = pStore->GetValue(PKEY_Device_FriendlyName, &friendlyName);
		if (HRFAILED(hr))
			return -1;

		WCHAR buf[512];
		_snwprintf_s(buf, _TRUNCATE, L"Set to %ws\r\n", friendlyName.pwszVal);
		OutputDebugStringW(buf);
		SetDefaultAudioPlaybackDevice(wstrID);

		PropVariantClear(&friendlyName);
	}

	return 0;
}
