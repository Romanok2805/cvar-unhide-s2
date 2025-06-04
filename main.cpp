#include <cstdint>
#include "steam/steamtypes.h"
#include "appframework/IAppSystem.h"
#include "eiface.h"
#include "filesystem.h"
#include "icvar.h"
#include "interface.h"
#include "vscript/ivscript.h"

CreateInterfaceFn g_pfnServerCreateInterface = NULL;

bool(*g_pfnServerConfigConnect)(IAppSystem *appSystem, CreateInterfaceFn factory);
float(*g_pfnServerConfigGetTickInterval)(const ISource2ServerConfig *config);

IScriptVM*(*g_pfnScriptManagerCreateVM)(IScriptManager *manager, ScriptLanguage_t language);

template<typename ReturnType, typename ...ArgTypes>
static auto PatchVtable(void *object, size_t index, ReturnType(*hook)(ArgTypes...))
{
	const auto **vtable = *(const void***)object;

	DWORD oldProtect;
	if (!VirtualProtect(vtable, sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect))
	{
		Plat_FatalErrorFunc("VirtualProtect PAGE_EXECUTE_READWRITE failed: %d", GetLastError());
	}

	const auto original = (decltype(hook))vtable[index];
	vtable[index] = hook;

	if (!VirtualProtect(vtable, sizeof(void*), oldProtect, &oldProtect))
	{
		Plat_FatalErrorFunc("VirtualProtect restore failed: %d", GetLastError());
	}

	return original;
}

static void SendToServerConsoleAsClient(int slot, const char *commandString)
{
	if (auto command = CCommand(); command.Tokenize(commandString))
		g_pSource2GameClients->ClientCommand(slot, command);
}

static IScriptVM *CreateVM(IScriptManager *manager, ScriptLanguage_t language)
{
	auto *vm = g_pfnScriptManagerCreateVM(manager, SL_LUA);
	//ScriptRegisterFunction(vm, SendToServerConsoleAsClient, "Run a server command as if sent by the given client");
	return vm;
}

static bool Connect(IAppSystem* appSystem, CreateInterfaceFn factory)
{
	const auto result = g_pfnServerConfigConnect(appSystem, factory);

	ConnectInterfaces(&factory, 1);

	ConVar_Register(FCVAR_RELEASE | FCVAR_GAMEDLL);

	g_pfnScriptManagerCreateVM = PatchVtable(g_pScriptManager, 11, CreateVM);

	return result;
}

float GetTickInterval(const ISource2ServerConfig *config)
{
	// override if tick rate specified in command line
	if (CommandLine()->CheckParm("-tickrate"))
	{
		const auto tickrate = CommandLine()->ParmValue("-tickrate", 0);
		if (tickrate > 10)
			return 1.0f / tickrate;
	}

	return g_pfnServerConfigGetTickInterval(config);
}

DLL_EXPORT void *CreateInterface(const char *pName, int *pReturnCode)
{
	if (g_pfnServerCreateInterface == NULL)
	{
		// Engine should stop joining VAC-secured servers with a modified gameinfo,
		// this is to be extra cautious.
		if (CommandLine()->HasParm(CUtlSymbolLarge_Hash(false, "-secretchinesemode", 18)))
		{
			Plat_FatalErrorFunc("ICommandLine::HasParm vtable index is out of date, unable to ensure -insecure.\n\nExiting for safety.");
		}

		if (!CommandLine()->HasParm(CUtlSymbolLarge_Hash(false, "-insecure", 9)))
		{
			Plat_FatalErrorFunc("Refusing to load cvar-unhide-s2 in secure mode.\n\nAdd -insecure to the game's launch options and restart the game.");
		}

		// Generate the path to the real server.dll
		CUtlString realServerPath(Plat_GetGameDirectory());
		realServerPath.Append("\\citadel\\bin\\win64\\server.dll");
		realServerPath.FixSlashes();

		HMODULE serverModule = LoadLibrary(realServerPath.GetForModify());
		g_pfnServerCreateInterface = (CreateInterfaceFn)GetProcAddress(serverModule, "CreateInterface");

		if (g_pfnServerCreateInterface == NULL)
		{
			Plat_FatalErrorFunc("Could not find CreateInterface entrypoint in server.dll: %d", GetLastError());
		}
	}

	auto original = g_pfnServerCreateInterface(pName, pReturnCode);

	// Intercept the first interface requested by the engine
	if (strcmp(pName, "Source2ServerConfig001") == 0)
	{
		g_pfnServerConfigConnect = PatchVtable(original, 0, Connect);
		g_pfnServerConfigGetTickInterval = PatchVtable(original, 13, GetTickInterval);
	}

	return original;
}
