//
//    Copyright (C) Microsoft.  All rights reserved.
// Licensed under the terms described in the LICENSE file in the root of this project.
//

#include "stdafx.h"

// Helper class for calling WSL Functions:
// https://msdn.microsoft.com/en-us/library/windows/desktop/mt826874(v=vs.85).aspx
WslApiLoader g_wslApi(DistroSpecial::Name);


HRESULT InstallDistribution(bool createUser, PCWSTR tarGzFilename)
{
	// Register the distribution.
	Helpers::PrintMessage(MSG_STATUS_INSTALLING);
	HRESULT hr = g_wslApi.WslRegisterDistribution(tarGzFilename);
	if (FAILED(hr)) {
		std::wcout << "WslRegisterDistribution failed" << std::endl;
		return hr;
	}

	// Delete /etc/resolv.conf to allow WSL to generate a version based on Windows networking information.
	DWORD exitCode;
	hr = g_wslApi.WslLaunchInteractive(DistroSpecial::commandLineDeleteResolvConf, true, &exitCode);
	if (FAILED(hr)) {
		return hr;
	}

	// Pre create user actions, used to fix issues (if any) and/or install dependencies before a user account is created.
	// Like wrong permissions on /
	for (const std::wstring &commandLine : DistroSpecial::commandLinePreAddUser) {
		if (!commandLine.empty()) {
			std::wcout << "Executing bugfixing command: " << commandLine << std::endl;
			hr = g_wslApi.WslLaunchInteractive(commandLine.c_str(), true, &exitCode);
			if ((FAILED(hr)) || (exitCode != 0)) {
				std::wcerr << "Bugfixing command failed." << std::endl;
				break;
			}
		}
	}
	if ((FAILED(hr)) || (exitCode != 0)) {
		std::wcerr << std::endl << std::endl << "==== MANUAL STEP ====" << std::endl;
		std::wcerr << "If you're on Windows 10 1803 17134, manually execute this/these command(s), NOW:" << std::endl << "Or upgrade to the latest version of windows 10" << std::endl;
		for (const std::wstring &commandLine : DistroSpecial::commandLinePreAddUser) {
			if (!commandLine.empty()) {
				std::wcerr << commandLine << std::endl;
			}
		}
		std::wcerr << std::endl;
		std::wcerr << "Also you need to provide a /bin/bash executable to use bash.exe from Windows." << std::endl;
		std::wcerr << "Either by symlinking any other shell to /bin/bash or by installing bash." << std::endl;
		std::wcerr << "If you want to change the default user (after you created one), run:" << std::endl;
		std::wcerr << DistroSpecial::Name << ".exe /config --default-user <username>" << std::endl;
		std::wcerr << "== // MANUAL STEP // ==" << std::endl;
		return hr;
	}

	// Create a user account.
	if (createUser) {
		Helpers::PrintMessage(MSG_CREATE_USER_PROMPT);
		std::wstring userName;
		do {
			userName = Helpers::GetUserInput(MSG_ENTER_USERNAME, 32);
		} while (!DistributionInfo::CreateUser(userName));

		// Set this user account as the default.
		hr = SetDefaultUser(userName);
		if (FAILED(hr)) {
			std::wcerr << "SetDefaultUser failed." << std::endl;
			return hr;
		}
	}

	// Set a root password
	if (DistroSpecial::askForRootPassword) {
		std::wcout << std::endl << "To (re-)set root password run `wsl.exe --user root --distribution Alpine passwd`" << std::endl << std::endl;
		std::system("C:\\Windows\\System32\\wsl.exe --user root --distribution Alpine passwd");
	}

	return hr;
}

HRESULT SetDefaultUser(std::wstring_view userName)
{
	// Query the UID of the given user name and configure the distribution
	// to use this UID as the default.
	ULONG uid = DistributionInfo::QueryUid(userName);
	if (uid == UID_INVALID) {
		std::wcerr << "UID is invalid or query command failed." << std::endl;
		return E_INVALIDARG;
	}

	HRESULT hr = g_wslApi.WslConfigureDistribution(uid, WSL_DISTRIBUTION_FLAGS_DEFAULT);
	if (FAILED(hr)) {
		return hr;
	}

	return hr;
}

int wmain(int argc, wchar_t const *argv[])
{
	//while (!::IsDebuggerPresent())
	//	::Sleep(100); // to avoid 100% CPU load

	// Update the title bar of the console window.
	SetConsoleTitleW(DistroSpecial::WindowTitle.c_str());

	// Initialize a vector of arguments.
	std::vector<std::wstring_view> arguments;
	for (int index = 1; index < argc; index += 1) {
		arguments.push_back(argv[index]);
	}

	// Ensure that the Windows Subsystem for Linux optional component is installed.
	DWORD exitCode = 1;
	if (!g_wslApi.WslIsOptionalComponentInstalled()) {
		Helpers::PrintMessage(MSG_MISSING_OPTIONAL_COMPONENT);
		if (arguments.empty()) {
			Helpers::PromptForInput();
		}

		return exitCode;
	}

	// Install the distribution if it is not already.
	bool installOnly = ((arguments.size() > 0) && (arguments[0] == ARG_INSTALL));
	HRESULT hr = S_OK;
	if (!g_wslApi.WslIsDistributionRegistered()) {
		// Switch workingdirectory to temporary folder
		const std::wstring tempWorkingDir = Helpers::CreateTemporaryDirectory();
		Helpers::SetWorkingDirectory(tempWorkingDir);
		fs::path file(L"install.tar.gz");
		fs::path full_path = tempWorkingDir / file;

		do {
			DownloadUserland();
			//Verify if download completed successfully with SHA256
		} while (!ChecksumVerify::Verify(full_path.c_str(), DistroSpecial::UserlandChecksum));

		// If the "--root" option is specified, do not create a user account.
		bool useRoot = ((installOnly) && (arguments.size() > 1) && (arguments[1] == ARG_INSTALL_ROOT));
		hr = InstallDistribution(!useRoot, full_path.c_str());
		if (FAILED(hr)) {
			if (hr == HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS)) {
				Helpers::PrintMessage(MSG_INSTALL_ALREADY_EXISTS);
			}

		}
		else {
			Helpers::PrintMessage(MSG_INSTALL_SUCCESS);
		}

		exitCode = SUCCEEDED(hr) ? 0 : 1;
	}

	// Parse the command line arguments.
	if ((SUCCEEDED(hr)) && (!installOnly)) {
		if (arguments.empty()) {
			hr = g_wslApi.WslLaunchInteractive(L"", false, &exitCode);

		}
		else if ((arguments[0] == ARG_RUN) ||
			(arguments[0] == ARG_RUN_C)) {

			std::wstring command = L"";
			for (size_t index = 1; index < arguments.size(); index += 1) {
				command += arguments[index];
				command += L" ";
			}

			hr = g_wslApi.WslLaunchInteractive(command.c_str(), true, &exitCode);

		}
		else if (arguments[0] == ARG_CONFIG) {
			hr = E_INVALIDARG;
			if (arguments.size() == 3) {
				if (arguments[1] == ARG_CONFIG_DEFAULT_USER) {
					hr = SetDefaultUser(arguments[2]);
				}
			}

			if (SUCCEEDED(hr)) {
				exitCode = 0;
			}

		}
		else {
			Helpers::PrintMessage(MSG_USAGE);
			return exitCode;
		}
	}

	// If an error was encountered, print an error message.
	if (FAILED(hr)) {
		if (hr == HRESULT_FROM_WIN32(ERROR_LINUX_SUBSYSTEM_NOT_PRESENT)) {
			Helpers::PrintMessage(MSG_MISSING_OPTIONAL_COMPONENT);

		}
		else {
			Helpers::PrintErrorMessage(hr);
		}

		if (arguments.empty()) {
			Helpers::PromptForInput();
		}
	}

	return SUCCEEDED(hr) ? exitCode : 1;
}
