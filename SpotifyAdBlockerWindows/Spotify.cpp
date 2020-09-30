#include <stdio.h>
#include <tchar.h>
#include "Spotify.h"


Spotify::Spotify()
{
	GetWindowHandle();
	GetSpotifyAudioSession();
}


Spotify::~Spotify()
{
}

BYTE Spotify::Contanins(PWCHAR Text,
	PCWCHAR Contained)
{
	for (DWORD i = 0; i < _tcslen(Text); i++)
	{
		DWORD same;

		same = 0;

		for (DWORD j = 0; j < _tcslen(Contained); j++)
		{
			if (Text[i + j] == Contained[j])
			{
				same++;
			}
			else
			{
				break;
			}
		}

		if (same == _tcslen(Contained))
		{
			return 1;
		}
	}

	return 0;
}

void Spotify::GetAllWindowsFromProcessID(DWORD DWProcessID)
{
	// find all hWnds (vhWnds) associated with a process id (dwProcessID)
	HWND hCurWnd;
	wchar_t text[500];
	std::vector<const wchar_t*> exceptions;

	hCurWnd = NULL;

	exceptions.push_back(L"MSCTFIME UI");
	exceptions.push_back(L"Default IME");
	exceptions.push_back(L"GDI+ Window (Spotify.exe)");

	do
	{
		hCurWnd = FindWindowEx(NULL, hCurWnd, NULL, NULL);
		if (hCurWnd == NULL)
		{
			wprintf(L"hCurWnd null\n");
		}
		DWORD dwProcessID = 0;
		GetWindowThreadProcessId(hCurWnd, &dwProcessID);
		if (dwProcessID == DWProcessID)
		{
			BYTE isException;

			isException = 0;

			GetWindowText(hCurWnd, text, 500);
			if (2 > _tcslen(text))
			{
				continue;
			}

			for (size_t i = 0; i < exceptions.size(); i++)
			{
				if (Contanins(text, exceptions[i]))
				{
					isException = 1;
					break;
				}
			}
			if (1 == isException)
			{
				continue;
			}

			wprintf(L"Found hWnd %d %d\n", hCurWnd, dwProcessID);
			_tprintf(TEXT("--+-- %s \n"), text);

			_spotifyWindow = hCurWnd;
			break;
		}
	} while (hCurWnd != NULL);
}

void Spotify::GetWindowHandle()
{
	PROCESSENTRY32 process;
	HANDLE hh;
	std::vector<PROCESSENTRY32> spotifyProcesses;

	hh = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hh == INVALID_HANDLE_VALUE)
	{
		return;
	}

	process.dwSize = sizeof(PROCESSENTRY32);
	Process32First(hh,
		&process);

	//add all spotify processes
	while (Process32Next(hh, &process))
	{
		if (Contanins(process.szExeFile, TEXT("Spotify")))
		{
			_tprintf(TEXT("%s %d %d\n"), process.szExeFile, process.th32ProcessID, process.th32ParentProcessID);
			spotifyProcesses.push_back(process);

		}
	}

	if (0 == spotifyProcesses.size())
	{
		_spotifyWindow = NULL;
		return;
	}

	//substract child processes
	for (int i = 0; i < spotifyProcesses.size(); i++)
	{
		BYTE contains;

		contains = 0;

		for (int j = 0; j < spotifyProcesses.size(); j++)
		{
			if (spotifyProcesses[j].th32ProcessID == spotifyProcesses[i].th32ParentProcessID)
			{
				contains = 1;
				break;
			}
		}

		if (1 == contains)
		{
			spotifyProcesses.erase(spotifyProcesses.begin() + i);
			i--;
		}
	}

	_tprintf(L"Substracted processes \n");
	for (int i = 0; i < spotifyProcesses.size(); i++)
	{
		process = spotifyProcesses[i];
		_tprintf(TEXT("%s %d %d\n"), process.szExeFile, process.th32ProcessID, process.th32ParentProcessID);
	}

	process = spotifyProcesses[0];

	GetAllWindowsFromProcessID(process.th32ProcessID);
}

BYTE Spotify::IsPlaying()
{
	wchar_t text[500];

	GetCurrentTrack(text, 500);

	if (0 == wcscmp(text, L"Spotify"))
	{
		return 0;
	}

	return 1;
}

void Spotify::GetCurrentTrack(PWCHAR PCurrentTrack, int MaxLenght)
{
	if (NULL == _spotifyWindow)
	{
		GetWindowHandle();

		if (NULL == _spotifyWindow)
		{
			return;
		}
	}

	GetWindowText(_spotifyWindow, PCurrentTrack, MaxLenght);
}

BYTE Spotify::IsAdsPlaying()
{
	wchar_t text[500];

	GetCurrentTrack(text, 500);

	if (Contanins(text, L"Afla mai multe"))
	{
		return 1;
	}
	else if (Contanins(text, L"Pureboost Go"))
	{
		return 1;
	}
	else if (Contanins(text, L"Advertisement"))
	{
		return 1;
	}
	else if (Contanins(text, L"Spotify"))
	{
		float peakVolume = DoesProduceSound();
		if (peakVolume > 0)
			return 1;
		return 0;
	}

	return 0;
}


HRESULT Spotify::GetAudioSessionEnumerator()
{
	HRESULT hr;

	hr = S_OK;

	hr = CoInitialize(NULL);
	if (FAILED(hr)) {
		LOG(L"CoInitialize failed: hr = 0x%08x", hr);
		return -__LINE__;
	}

	// get default device
	CComPtr<IMMDeviceEnumerator> pMMDeviceEnumerator;
	hr = pMMDeviceEnumerator.CoCreateInstance(__uuidof(MMDeviceEnumerator));
	if (FAILED(hr)) {
		LOG(L"CoCreateInstance(IMMDeviceEnumerator) failed: hr = 0x%08x", hr);
		return -__LINE__;
	}

	EDataFlow flows[] = { eRender };

	for (UINT f = 0; f < ARRAYSIZE(flows); f++) {

		CComPtr<IMMDeviceCollection> pMMDeviceCollection;
		hr = pMMDeviceEnumerator->EnumAudioEndpoints(flows[f], DEVICE_STATE_ACTIVE, &pMMDeviceCollection);
		if (FAILED(hr)) {
			LOG(L"IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = 0x%08x", hr);
			continue;
		}

		CComPtr<IMMDevice> pMMDeviceDefault;
		hr = pMMDeviceEnumerator->GetDefaultAudioEndpoint(flows[f], eMultimedia, &pMMDeviceDefault);
		if (FAILED(hr)) {
			LOG(L"IMMDeviceEnumerator::EnumAudioEndpoints failed: hr = 0x%08x", hr);
			continue;
		}

		UINT32 nDevices;
		hr = pMMDeviceCollection->GetCount(&nDevices);
		if (FAILED(hr)) {
			LOG(L"IMMDeviceCollection::GetCount failed: hr = 0x%08x", hr);
			continue;
		}

		CComPtr<IMMDevice> pMMDevice = pMMDeviceDefault;
		_pMMDevice = pMMDevice;

		// get the name of the endpoint
		CComPtr<IPropertyStore> pPropertyStore;
		hr = pMMDevice->OpenPropertyStore(STGM_READ, &pPropertyStore);
		if (FAILED(hr)) {
			LOG(L"IMMDevice::OpenPropertyStore failed: hr = 0x%08x", hr);
			continue;
		}

		PROPVARIANT v; PropVariantInit(&v);
		hr = pPropertyStore->GetValue(PKEY_Device_FriendlyName, &v);
		if (FAILED(hr)) {
			LOG(L"IPropertyStore::GetValue(PKEY_Device_FriendlyName) failed: hr = 0x%08x", hr);
			continue;
		}

		if (VT_LPWSTR != v.vt) {
			LOG(L"PKEY_Device_FriendlyName has unexpected vartype %u", v.vt);
			continue;
		}

		// get the current audio peak meter level for this endpoint
		CComPtr<IAudioMeterInformation> pAudioMeterInformation_Endpoint;
		hr = pMMDevice->Activate(
			__uuidof(IAudioMeterInformation),
			CLSCTX_ALL,
			NULL,
			reinterpret_cast<void**>(&pAudioMeterInformation_Endpoint)
		);
		if (FAILED(hr)) {
			LOG(L"IMMDevice::Activate(IAudioMeterInformation) failed: hr = 0x%08x", hr);
			continue;
		}

		float peak_endpoint = 0.0f;
		hr = pAudioMeterInformation_Endpoint->GetPeakValue(&peak_endpoint);
		if (FAILED(hr)) {
			LOG(L"IAudioMeterInformation::GetPeakValue() failed: hr = 0x%08x", hr);
			continue;
		}

		// get an endpoint volume interface
		CComPtr<IAudioEndpointVolume> pAudioEndpointVolume;
		hr = pMMDevice->Activate(
			__uuidof(IAudioEndpointVolume),
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&pAudioEndpointVolume)
		);
		if (FAILED(hr)) {
			LOG(L"IMMDevice::Activate(IAudioEndpointVolume) failed: hr = 0x%08x", hr);
			continue;
		}
		_pAudioEndpointVolume = pAudioEndpointVolume;

		BOOL mute;
		hr = pAudioEndpointVolume->GetMute(&mute);
		if (FAILED(hr)) {
			LOG(L"IAudioEndpointVolume::GetMute failed: hr = 0x%08x", hr);
			continue;
		}


		float pctMaster;
		hr = pAudioEndpointVolume->GetMasterVolumeLevelScalar(&pctMaster);
		if (FAILED(hr)) {
			LOG(L"IAudioEndpointVolume::GetMasterVolumeLevelScalar failed: hr = 0x%08x", hr);
			continue;
		}

		float dbMaster;
		hr = pAudioEndpointVolume->GetMasterVolumeLevel(&dbMaster);
		if (FAILED(hr)) {
			LOG(L"IAudioEndpointVolume::GetMasterVolumeLevel failed: hr = 0x%08x", hr);
			continue;
		}

		LOG(
			L"%s\n"
			L"    Peak: %g\n"
			L"    Mute: %d\n"
			L"    Master: %g%% (%g dB)",
			v.pwszVal,
			peak_endpoint,
			mute,
			pctMaster * 100.0f, dbMaster
		);

		// get a session enumerator
		CComPtr<IAudioSessionManager2> pAudioSessionManager2;
		hr = pMMDevice->Activate(
			__uuidof(IAudioSessionManager2),
			CLSCTX_ALL,
			nullptr,
			reinterpret_cast<void**>(&pAudioSessionManager2)
		);
		if (FAILED(hr)) {
			LOG(L"IMMDevice::Activate(IAudioSessionManager2) failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		_pAudioSessionManager2 = pAudioSessionManager2;

		CComPtr<IAudioSessionEnumerator> pAudioSessionEnumerator;
		hr = pAudioSessionManager2->GetSessionEnumerator(&pAudioSessionEnumerator);
		if (FAILED(hr)) {
			LOG(L"IAudioSessionManager2::GetSessionEnumerator() failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		_pAudioSessionEnumerator = pAudioSessionEnumerator;

		// iterate over all the sessions
		int count = 0;
		hr = pAudioSessionEnumerator->GetCount(&count);
		if (FAILED(hr)) {
			LOG(L"IAudioSessionEnumerator::GetCount() failed: hr = 0x%08x", hr);
			return -__LINE__;
		}

		for (int session = 0; session < count; session++) {
			// get the session identifier
			CComPtr<IAudioSessionControl> pAudioSessionControl;
			hr = pAudioSessionEnumerator->GetSession(session, &pAudioSessionControl);
			if (FAILED(hr)) {
				LOG(L"IAudioSessionEnumerator::GetSession() failed: hr = 0x%08x", hr);
				return -__LINE__;
			}
		}

		return hr;
	}
	return hr;
}

HRESULT Spotify::GetSpotifyAudioSession()
{
	HRESULT hr = S_OK;

	hr = GetAudioSessionEnumerator();
	if (FAILED(hr)) {
		LOG(L"GetAudioEndpointVolume failed: hr = 0x%08x", hr);
		return hr;
	}

	CComPtr<IAudioSessionEnumerator> pAudioSessionEnumerator;
	pAudioSessionEnumerator = _pAudioSessionEnumerator;

	// iterate over all the sessions
	int count = 0;
	hr = pAudioSessionEnumerator->GetCount(&count);
	if (FAILED(hr)) {
		LOG(L"IAudioSessionEnumerator::GetCount() failed: hr = 0x%08x", hr);
		return -__LINE__;
	}

	for (int session = 0; session < count; session++) {
		// get the session identifier
		CComPtr<IAudioSessionControl> pAudioSessionControl;
		hr = pAudioSessionEnumerator->GetSession(session, &pAudioSessionControl);
		if (FAILED(hr)) {
			LOG(L"IAudioSessionEnumerator::GetSession() failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		AudioSessionState state;
		hr = pAudioSessionControl->GetState(&state);
		if (FAILED(hr)) {
			LOG(L"IAudioSessionControl::GetState() failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		if (AudioSessionStateActive != state) {
			// skip this session
			hr = -__LINE__;
			continue;
		}

		CComPtr<IAudioSessionControl2> pAudioSessionControl2;
		hr = pAudioSessionControl->QueryInterface(IID_PPV_ARGS(&pAudioSessionControl2));
		if (FAILED(hr)) {
			LOG(L"IAudioSessionControl::QueryInterface(IAudioSessionControl2) failed: hr = 0x%08x", hr);
			return -__LINE__;
		}

		DWORD pid = 0;
		hr = pAudioSessionControl2->GetProcessId(&pid);
		if (FAILED(hr)) {
			LOG(L"IAudioSessionControl2::GetProcessId() failed: hr = 0x%08x", hr);
			return -__LINE__;
		}

		bool bMultiProcess = (AUDCLNT_S_NO_SINGLE_PROCESS == hr);

		// get the current audio peak meter level for this session
		CComPtr<IAudioMeterInformation> pAudioMeterInformation_Session;
		hr = pAudioSessionControl->QueryInterface(IID_PPV_ARGS(&pAudioMeterInformation_Session));
		if (FAILED(hr)) {
			LOG(L"IAudioSessionControl::QueryInterface(IAudioMeterInformation) failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		_pAudioMeterInformation_Session = pAudioMeterInformation_Session;
		float peak_session = 0.0f;
		hr = pAudioMeterInformation_Session->GetPeakValue(&peak_session);
		if (FAILED(hr)) {
			LOG(L"IAudioMeterInformation::GetPeakValue() failed: hr = 0x%08x", hr);
			return -__LINE__;
		}

		CComHeapPtr<WCHAR> szSessionIdentifier;
		hr = pAudioSessionControl2->GetSessionIdentifier(&szSessionIdentifier);
		if (FAILED(hr)) {
			LOG(L"IAudioSessionControl2::GetSessionIdentifier() failed: hr = 0x%08x", hr);
			return -__LINE__;
		}

		if (!Contanins(szSessionIdentifier, L"Spotify"))
			continue;

		LOG(
			L"        Peak value: %g\n"
			L"        Process ID: %u%s\n"
			L"        Session identifier: %s\n"
			,
			peak_session,
			pid, (bMultiProcess ? L" (multi-process)" : L" (single-process)"),
			static_cast<LPCWSTR>(szSessionIdentifier)
		);


		// query the volumes
		CComPtr<ISimpleAudioVolume> pSimpleAudioVolume;
		hr = pAudioSessionControl->QueryInterface(IID_PPV_ARGS(&pSimpleAudioVolume));
		if (FAILED(hr)) {
			LOG(L"IAudioSessionControl::QueryInterface(ISimpleAudioVolume) failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		_pSpotifySimpleAudioVolume = pSimpleAudioVolume;

		float fMasterVolume;
		hr = pSimpleAudioVolume->GetMasterVolume(&fMasterVolume);
		//pSimpleAudioVolume->SetMasterVolume(0, NULL);
		if (FAILED(hr)) {
			LOG(L"ISimpleAudioVolume::GetMasterVolume failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		LOG(L"        Master volume: %g", fMasterVolume);

		BOOL bMute;
		hr = pSimpleAudioVolume->GetMute(&bMute);
		if (FAILED(hr)) {
			LOG(L"ISimpleAudioVolume::GetMute failed: hr = 0x%08x", hr);
			return -__LINE__;
		}
		LOG(L"        %s", (bMute ? L"Muted" : L"Not muted"));

		hr = S_OK;
		break;

		LOG(L"");
	}

	return hr;

}

void Spotify::Mute()
{
	if (!_pSpotifySimpleAudioVolume)
		return;
	_pSpotifySimpleAudioVolume->SetMute(TRUE, NULL);
}

void Spotify::Unmute()
{
	if (!_pSpotifySimpleAudioVolume)
		return;
	_pSpotifySimpleAudioVolume->SetMute(FALSE, NULL);
}

float Spotify::DoesProduceSound()
{
	float peak_session = 0.0f;
	if (_pAudioMeterInformation_Session == NULL || _pSpotifySimpleAudioVolume == NULL) {
		//try to get the session
		HRESULT hr = S_OK;
		hr = GetSpotifyAudioSession();
		if (FAILED(hr))
		{
			LOG(L"_pAudioMeterInformation_Session is NULL");
			return -__LINE__;
		}
	}
	_pAudioMeterInformation_Session->GetPeakValue(&peak_session);

	return peak_session;
}