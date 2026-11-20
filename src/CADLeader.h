#pragma once

#include "CADBundle.h"
#include "CADBezFontText.h"

ClassDesc2* GetCADLeaderDesc();

class CADLeader : public HelperObject
{
public:
    CADLeader();
    ~CADLeader() override = default;

    Class_ID ClassID() override { return kCADLeaderClassID; }
    SClass_ID SuperClassID() override { return HELPER_CLASS_ID; }
    void GetClassName(TSTR& s, bool localized) const override
    {
        s = localized ? GetString(IDS_LEADER_CLASS) : _T("CADLeader");
    }
    const MCHAR* GetObjectName(bool localized) const override
    {
        return localized ? GetString(IDS_LEADER_CLASS) : _T("CADLeader");
    }
    void InitNodeName(MSTR& s) override { s = _T("CAD引线"); }
    void DeleteThis() override { delete this; }

    int NumRefs() override { return 1; }
    RefTargetHandle GetReference(int index) override
    {
        return index == 0 ? mPBlock : nullptr;
    }
    void SetReference(int index, RefTargetHandle target) override;
    RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget,
                               PartID& partID, RefMessage message,
                               BOOL propagate) override;

    int NumSubs() override { return 0; }
    Animatable* SubAnim(int) override { return nullptr; }
    MSTR SubAnimName(int, bool) override { return MSTR(); }

    int NumParamBlocks() override { return 1; }
    IParamBlock2* GetParamBlock(int index) override
    {
        return index == 0 ? mPBlock : nullptr;
    }
    IParamBlock2* GetParamBlockByID(BlockID id) override;

    void GetLocalBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
    void GetWorldBoundBox(TimeValue t, INode* inode, ViewExp* vpt, Box3& box) override;
    BOOL HasViewDependentBoundingBox() override { return TRUE; }
    int Display(TimeValue t, INode* inode, ViewExp* vpt, int flags) override;
    int HitTest(TimeValue t, INode* inode, int type, int crossing, int flags,
                IPoint2* p, ViewExp* vpt) override;
    CreateMouseCallBack* GetCreateMouseCallBack() override;

    void BeginEditParams(IObjParam* ip, ULONG flags, Animatable* prev) override;
    void EndEditParams(IObjParam* ip, ULONG flags, Animatable* next) override;
    IOResult Save(ISave* isave) override;
    IOResult Load(ILoad* iload) override;
    RefTargetHandle Clone(RemapDir& remap) override;
    ObjectState Eval(TimeValue) override { return ObjectState(this); }

    void SetCreationPoint(int index, const Point3& localPoint);

private:
    bool GetParams(TimeValue t, Point3& start, Point3& elbow, Point3& textPos,
                   MSTR& note, float& arrowSize, float& textHeight,
                   Color& lineColor, Color& textColor, MSTR& fontName);

    IParamBlock2* mPBlock = nullptr;
    CADBezFontCache mFontCache;
};
