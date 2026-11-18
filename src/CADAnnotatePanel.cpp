#include "CADAnnotatePanel.h"

class CADAnnotatePanel : public UtilityObj
{
public:
    void BeginEditParams(Interface*, IUtil*) override {}
    void EndEditParams(Interface*, IUtil*) override {}
    void DeleteThis() override { delete this; }
};

class CADAnnotatePanelClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return TRUE; }
    void* Create(BOOL) override { return new CADAnnotatePanel(); }
    const TCHAR* ClassName() override { return GetString(IDS_UTILITY_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADAnnotatePanel"); }
    SClass_ID SuperClassID() override { return UTILITY_CLASS_ID; }
    Class_ID ClassID() override { return kCADAnnotatePanelClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADAnnotatePanel"); }
    HINSTANCE HInstance() override { return hInstance; }
};

ClassDesc2* GetCADAnnotatePanelDesc()
{
    static CADAnnotatePanelClassDesc desc;
    return &desc;
}
