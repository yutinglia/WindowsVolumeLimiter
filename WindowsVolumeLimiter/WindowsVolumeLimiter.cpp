
#include <iostream>
#include <windows.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <audiopolicy.h>
#include <psapi.h>


#include <string>
#include <memory>


#include <atlstr.h>
#include <tchar.h>


#include <thread>


std::wstring GetProcessNameFromPid(DWORD pid) {
	HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);

	if (handle) {
		TCHAR buffer[MAX_PATH];
		GetModuleBaseName(handle, 0, buffer, MAX_PATH);
		CloseHandle(handle);
		return buffer;
	}

	return L"Unknown(should be system process)";
}



void ProcessVolumes(IAudioSessionEnumerator* enumerator) {
	int sessionCount;
	enumerator->GetCount(&sessionCount);
	for (int index = 0; index < sessionCount; index++) {
		IAudioSessionControl* session = nullptr;
		IAudioSessionControl2* session2 = nullptr;
		enumerator->GetSession(index, &session);
		session->QueryInterface(__uuidof(IAudioSessionControl2), (void**)&session2);

		DWORD id = 0;
		session2->GetProcessId(&id);

		LPWSTR guid;
		session2->GetSessionInstanceIdentifier(&guid);

		ISimpleAudioVolume* saVol = nullptr;
		float volume;
		session2->QueryInterface(__uuidof(ISimpleAudioVolume), (void**)&saVol);
		saVol->GetMasterVolume(&volume);

		if (volume >= .7)
		{
			std::wstring processName = GetProcessNameFromPid(id);
			std::wcout << processName << ": " << volume << " -> " << "0.1" << std::endl;
			saVol->SetMasterVolume(.1, NULL);
		}

		session2->Release();
		session->Release();
	}
}



void backendThread(IAudioSessionEnumerator* enumerator, BOOL* runningFlagPtr)
{
	std::this_thread::sleep_for(std::chrono::seconds(1));
	while (*runningFlagPtr)
	{
		ProcessVolumes(enumerator);
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}



int main()
{
	// init com
	HRESULT hr = CoInitialize(NULL);
	if (S_OK != hr)
	{
		MessageBox(NULL, L"COM Initialize Failed!", L"Error", MB_ICONERROR);
		return 1;
	}
	// create MMDeviceEnumerator instance
	IMMDeviceEnumerator* deviceEnumerator = nullptr;
	hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID*)&deviceEnumerator);
	if (S_OK != hr)
	{
		MessageBox(NULL, L"Create MMDeviceEnumerator Instance Failed!", L"Error", MB_ICONERROR);
		return 1;
	}
	// get default audio device
	IMMDevice* device = nullptr;
	deviceEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device);
	
	IAudioSessionManager2* sessionManager = nullptr;
	device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, (void**)&sessionManager);

	IAudioSessionEnumerator* enumerator = nullptr;
	sessionManager->GetSessionEnumerator(&enumerator);


	// start backend thread
	BOOL runningFlag = TRUE;
	std::thread backend(backendThread, enumerator, &runningFlag);

	// user input loop
	std::string userInput;
	while (1)
	{
		std::cin >> userInput;
		std::cout << "User Input: " << userInput << std::endl;
		if (userInput.compare("quit") == 0)
		{
			std::cout << "Quit App" << std::endl;
			break;
		}
		else
		{
			std::cout << "Unknown Command: " << userInput << std::endl;
		}
		userInput.clear();
	}

	// wait backend thread stop
	runningFlag = FALSE;
	backend.join();

	// release 
	deviceEnumerator->Release();
	device->Release();
	sessionManager->Release();
	enumerator->Release();
}
