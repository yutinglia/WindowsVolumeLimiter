
#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <psapi.h>
#include <filesystem>


#include <string>
#include <memory>


#include <atlstr.h>
#include <atlconv.h>
#include <tchar.h>


#include <thread>


#include "Config.hpp"


namespace fs = std::filesystem;

const std::wstring REG_APP_NAME = L"YutingliaWindowsVolumeLimiter";
Config config = Config();


void enableAutoStart()
{
	HKEY hKey;

	// get reg
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)     
	{
		// get bat path
		auto batPath = config.getBatPath();

		// check reg exist
		TCHAR strDir[MAX_PATH] = {};
		DWORD nLength = MAX_PATH;
		long result = RegGetValue(hKey, nullptr, REG_APP_NAME.c_str(), RRF_RT_REG_SZ, 0, strDir, &nLength);

		if (result != ERROR_SUCCESS || _tcscmp(batPath.c_str(), strDir) != 0)
		{
			RegSetValueEx(hKey, REG_APP_NAME.c_str(), 0, REG_SZ, (LPBYTE)batPath.c_str(), (lstrlen(batPath.c_str()) + 1) * sizeof(TCHAR));
			RegCloseKey(hKey);
		}
	}
}



void disableAutoStart()
{
	HKEY hKey;
	// get reg
	if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"), 0, KEY_ALL_ACCESS, &hKey) == ERROR_SUCCESS)
	{
		// delete reg value
		RegDeleteValue(hKey, REG_APP_NAME.c_str());
		RegCloseKey(hKey);
	}
}



std::wstring getProcessNameFromPid(DWORD pid) {
	HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

	if (handle) {
		TCHAR buffer[MAX_PATH];
		GetModuleBaseName(handle, 0, buffer, MAX_PATH);
		CloseHandle(handle);
		return buffer;
	}

	return L"Unknown(should be system process)";
}



void handleVolumes(IAudioSessionEnumerator* enumerator) {
	int sessionCount;
	enumerator->GetCount(&sessionCount);
	// loop through all session
	for (int index = 0; index < sessionCount; index++) {
		// get session
		IAudioSessionControl* session = nullptr;
		IAudioSessionControl2* session2 = nullptr;
		enumerator->GetSession(index, &session);
		session->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&session2);

		DWORD pid = 0;
		session2->GetProcessId(&pid);

		LPWSTR guid;
		session2->GetSessionInstanceIdentifier(&guid);

		// get process volume
		ISimpleAudioVolume* simpleAudioVolume = nullptr;
		float volume;
		session2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&simpleAudioVolume);
		simpleAudioVolume->GetMasterVolume(&volume);

		// set volume if too loud
		if (volume >= config.getMaxVolume())
		{
			std::wstring processName = getProcessNameFromPid(pid);
			std::wcout << processName << ": " << volume << " -> " << config.getSetVolume() << std::endl;
			simpleAudioVolume->SetMasterVolume(config.getSetVolume(), NULL);
		}

		simpleAudioVolume->Release();
		session2->Release();
		session->Release();
	}
}



void handleVolumeLoop(BOOL* isRunningFlagPtr)
{
	// init com
	HRESULT hr = CoInitialize(NULL);

	// create MMDeviceEnumerator instance
	IMMDeviceEnumerator* deviceEnumerator = nullptr;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);

	// get default audio device
	IMMDevice* device = nullptr;
	deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);

	// get audio session manager
	IAudioSessionManager2* sessionManager = nullptr;
	device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&sessionManager);
	std::this_thread::sleep_for(std::chrono::seconds(1));

	deviceEnumerator->Release();
	device->Release();

	IAudioSessionEnumerator* enumerator = nullptr;

	// loop
	while (*isRunningFlagPtr)
	{
		sessionManager->GetSessionEnumerator(&enumerator);
		handleVolumes(enumerator);
		enumerator->Release();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	sessionManager->Release();
}


int main()
{
	std::cout << "Current config: maxVolume => " << config.getMaxVolume() << ", setVolume =>" << config.getSetVolume() << std::endl;

	// start handle volume thread
	BOOL isRunning = TRUE;
	std::thread handleVolumeThread(handleVolumeLoop, &isRunning);

	// user input loop
	std::string userInput;
	while (1)
	{
		std::cin >> userInput;
		if (userInput.compare("q") == 0)
		{
			std::cout << "Quit App" << std::endl;
			break;
		}
		else if (userInput.compare("es") == 0)
		{
			std::cout << "Enable Auto Start" << std::endl;
			enableAutoStart();
		}
		else if (userInput.compare("ds") == 0)
		{
			std::cout << "Disable Auto Start" << std::endl;
			disableAutoStart();
		}
		else
		{
			std::cout << "Unknown Command: " << userInput << std::endl;
		}
		userInput.clear();
	}
	 
	// wait backend thread stop
	isRunning = FALSE;
	handleVolumeThread.join();
}
