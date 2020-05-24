#include "script.h"
#include <inc/nativeCaller.h>
#include "Util/Paths.h"
#include "Util/Logger.hpp"
#include "Util/Versions.h"

#define DISPLAY_VERSION "1.0.0"

std::string modDir;
BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved)
{
	modDir = "\\" + Paths::GetModuleNameWithoutExtension(hInstance);
	std::string logFile = Paths::GetModuleFolder(hInstance) + modDir +
		"\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";
	logger.SetFile(logFile);
	Paths::SetOurModuleHandle(hInstance);
	switch (reason)	{
	case DLL_PROCESS_ATTACH:
		scriptRegister(hInstance, ScriptMain);
		logger.Clear();
		logger.Write(INFO, Paths::GetModuleNameWithoutExtension(hInstance) + " " + std::string(DISPLAY_VERSION));
		logger.Write(INFO, "Game version " + eGameVersionToString(getGameVersion()));
		logger.Write(INFO, "Script registered");
		break;
	case DLL_PROCESS_DETACH:
		scriptUnregister(hInstance);
		break;
	}
	return TRUE;
}
