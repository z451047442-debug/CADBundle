#include "CADText.h"

class CADText : public HelperObject
{
public:
    Class_ID ClassID() override { return kCADTextClassID; }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    void GetClassName(TSTR& s, bool localized) const override { s = localized ? GetString(IDS_TEXT_CLASS) : _T("CADText"); }
    void DeleteThis() override { delete this; }

    int NumRefs() override { return 0; }
    RefTargetHandle GetReference(int) override { return nullptr; }
    void SetReference(int, RefTargetHandle) override {}
    RefResult NotifyRefChanged(const Interval&, RefTargetHandle, PartID&, RefMessage, BOOL) override { return REF_SUCCEED; }

    void GetLocalBoundBox(TimeValue, INode*, ViewExp*, Box3& box) override { box.Init(); }
    void GetWorldBoundBox(TimeValue, INode*, ViewExp*, Box3& box) override { box.Init(); }
    int Display(TimeValue, INode*, ViewExp*, int) override { return 0; }
    int HitTest(TimeValue, INode*, int, int, int, IPoint2*, ViewExp*) override { return 0; }
    CreateMouseCallBack* GetCreateMouseCallBack() override { return nullptr; }

    void BeginEditParams(IObjParam*, ULONG, Animatable*) override {}
    void EndEditParams(IObjParam*, ULONG, Animatable*) override {}
    IOResult Save(ISave*) override { return IO_OK; }
    IOResult Load(ILoad*) override { return IO_OK; }
    RefTargetHandle Clone(RemapDir&) override { return nullptr; }
    ObjectState Eval(TimeValue) override { return ObjectState(this); }
};

class CADTextClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return FALSE; }
    void* Create(BOOL) override { return new CADText(); }
    const TCHAR* ClassName() override { return GetString(IDS_TEXT_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADText"); }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    Class_ID ClassID() override { return kCADTextClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADText"); }
    HINSTANCE HInstance() override { return hInstance; }
};

ClassDesc2* GetCADTextDesc()
{
    static CADTextClassDesc desc;
    return &desc;
}
