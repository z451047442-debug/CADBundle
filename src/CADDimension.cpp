#include "CADDimension.h"

class CADDimension : public HelperObject
{
public:
    Class_ID ClassID() override { return kCADDimensionClassID; }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    void GetClassName(TSTR& s, bool localized) const override { s = localized ? GetString(IDS_DIM_CLASS) : _T("CADDimension"); }
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

class CADDimensionClassDesc : public ClassDesc2
{
public:
    int IsPublic() override { return FALSE; }
    void* Create(BOOL) override { return new CADDimension(); }
    const TCHAR* ClassName() override { return GetString(IDS_DIM_CLASS); }
    const TCHAR* NonLocalizedClassName() override { return _T("CADDimension"); }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    Class_ID ClassID() override { return kCADDimensionClassID; }
    const TCHAR* Category() override { return GetString(IDS_CATEGORY); }
    const TCHAR* InternalName() override { return _T("CADDimension"); }
    HINSTANCE HInstance() override { return hInstance; }
};

ClassDesc2* GetCADDimensionDesc()
{
    static CADDimensionClassDesc desc;
    return &desc;
}
