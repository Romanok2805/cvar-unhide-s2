#include <cstdint>
#include "steam/steamtypes.h"
#include "appframework/IAppSystem.h"
#include "filesystem.h"
#include "icvar.h"
#include "interface.h"

CreateInterfaceFn g_pfnServerCreateInterface = NULL;

typedef bool(*AppSystemConnectFn)(IAppSystem *appSystem, CreateInterfaceFn factory);
static AppSystemConnectFn g_pfnServerConfigConnect = NULL;

class IScriptVM;
class IScriptManager;

enum ScriptLanguage_t
{
	SL_NONE,
	SL_LUA,

	SL_DEFAULT = SL_LUA
};

typedef IScriptVM*(*ScriptManagerCreateVMFn)(IScriptManager *manager, ScriptLanguage_t language);
static ScriptManagerCreateVMFn g_pfnScriptManagerCreateVM = NULL;

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

static IScriptVM *CreateVM(IScriptManager *manager, ScriptLanguage_t language)
{
	return g_pfnScriptManagerCreateVM(manager, SL_LUA);
}

static bool Connect(IAppSystem* appSystem, CreateInterfaceFn factory)
{
	const auto result = g_pfnServerConfigConnect(appSystem, factory);

	ConnectInterfaces(&factory, 1);

	ConVar_Register(FCVAR_RELEASE | FCVAR_CLIENT_CAN_EXECUTE | FCVAR_GAMEDLL);

	g_pfnScriptManagerCreateVM = PatchVtable(g_pScriptManager, 11, CreateVM);

	return result;
}

#undef CreateInterface

DLL_EXPORT void *CreateInterface(const char *pName, int *pReturnCode)
{
	if (g_pfnServerCreateInterface == NULL)
	{
		// Engine should stop joining VAC-secured servers with a modified gameinfo,
		// this is to be extra cautious.
		auto insecure = CommandLine()->HasParm("-insecure");
		if (!insecure)
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
	}

	return original;
}
