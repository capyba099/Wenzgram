/*
This file is part of Wenzgram,
a minimal Windows helper that replaces Wenzgram.exe while it is closed.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#include <windows.h>
#include <shellapi.h>
#include <string>

namespace {

using String = std::wstring;

[[nodiscard]] String Argument(int index) {
	const auto buffer = GetCommandLineW();
	// Simple argv parsing via CommandLineToArgvW.
	int argc = 0;
	const auto argv = CommandLineToArgvW(buffer, &argc);
	if (!argv || index >= argc) {
		if (argv) {
			LocalFree(argv);
		}
		return {};
	}
	const auto result = String(argv[index]);
	LocalFree(argv);
	return result;
}

[[nodiscard]] bool CopyWithRetry(
		const String &from,
		const String &to) {
	for (auto i = 0; i != 60; ++i) {
		if (CopyFileW(from.c_str(), to.c_str(), FALSE)) {
			return true;
		}
		const auto error = GetLastError();
		if (error != ERROR_SHARING_VIOLATION
			&& error != ERROR_ACCESS_DENIED) {
			return false;
		}
		Sleep(500);
	}
	return false;
}

} // namespace

int APIENTRY wWinMain(
		HINSTANCE,
		HINSTANCE,
		LPWSTR,
		int) {
	const auto mode = Argument(1);
	if (mode != L"--update") {
		return 1;
	}
	const auto pidText = Argument(2);
	const auto targetExe = Argument(3);
	const auto sourceExe = Argument(4);
	if (pidText.empty() || targetExe.empty() || sourceExe.empty()) {
		return 2;
	}
	const auto pid = static_cast<DWORD>(std::wcstoul(pidText.c_str(), nullptr, 10));
	if (pid) {
		const auto process = OpenProcess(SYNCHRONIZE, FALSE, pid);
		if (process) {
			WaitForSingleObject(process, INFINITE);
			CloseHandle(process);
		} else {
			Sleep(3000);
		}
	}
	if (!CopyWithRetry(sourceExe, targetExe)) {
		MessageBoxW(
			nullptr,
			L"Не удалось установить обновление Wenzgram.",
			L"Wenzgram Updater",
			MB_ICONERROR);
		return 3;
	}
	ShellExecuteW(nullptr, L"open", targetExe.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
	return 0;
}
