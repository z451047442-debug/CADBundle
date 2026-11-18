#include "CADBundle.h"

extern ClassDesc2* GetCADAnnotatePanelDesc();
extern ClassDesc2* GetCADDimensionDesc();
extern ClassDesc2* GetCADTextDesc();
extern ClassDesc2* GetCADLeaderDesc();
extern ClassDesc2* GetCADTableDesc();

HINSTANCE hInstance = nullptr;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, ULONG fdwReason, LPVOID)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        MaxSDK::Util::UseLanguagePackLocale();
        hInstance = hinstDLL;
        DisableThreadLibraryCalls(hInstance);
    }
    return TRUE;
}

extern "C" const TCHAR* LibDescription()
{
    return GetString(IDS_LIBDESCRIPTION);
}

extern "C" int LibNumberClasses()
{
    return 5;
}

extern "C" ClassDesc* LibClassDesc(int i)
{
    switch (i)
    {
    case 0: return GetCADAnnotatePanelDesc();
    case 1: return GetCADDimensionDesc();
    case 2: return GetCADTextDesc();
    case 3: return GetCADLeaderDesc();
    case 4: return GetCADTableDesc();
    default: return nullptr;
    }
}

extern "C" ULONG LibVersion()
{
    return VERSION_3DSMAX;
}

extern "C" int LibInitialize(void)
{
    return TRUE;
}

extern "C" int LibShutdown(void)
{
    return TRUE;
}

extern "C" ULONG CanAutoDefer()
{
    return 0;
}

TCHAR* GetString(int id)
{
    static TCHAR buf[512];
    if (hInstance)
    {
        return LoadString(hInstance, id, buf, _countof(buf)) ? buf : nullptr;
    }
    return nullptr;
}
