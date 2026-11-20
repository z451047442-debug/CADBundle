#include "CADParamBlockHost.h"

void CADMakeAutoParamBlocks(ReferenceMaker* owner, ClassDesc2* desc)
{
    if (owner && desc)
    {
        desc->MakeAutoParamBlocks(owner);
    }
}

ReferenceTarget* CADClonePBlockRef(IParamBlock2* pblock, RemapDir& remap)
{
    return pblock ? remap.CloneRef(pblock) : nullptr;
}

void CADSetPBlockRef(IParamBlock2*& pblock, RefTargetHandle target)
{
    pblock = target ? static_cast<IParamBlock2*>(target) : nullptr;
}

RefResult CADNotifyPBlockChange(ReferenceMaker* owner, IParamBlock2* pblock,
                                RefTargetHandle target, PartID& partID,
                                RefMessage message, BOOL propagate,
                                Interface* ip, bool* invalidateCache)
{
    (void)partID;
    (void)propagate;

    if (pblock && target == pblock && message == REFMSG_CHANGE)
    {
        if (invalidateCache)
        {
            *invalidateCache = true;
        }
        if (owner)
        {
            owner->NotifyDependents(FOREVER, PART_ALL, REFMSG_CHANGE);
        }
        if (ip)
        {
            ip->RedrawViews(ip->GetTime());
        }
    }

    return REF_SUCCEED;
}

void CADBeginAutoUI(ClassDesc2* desc, IObjParam* ip, ReferenceMaker* owner,
                    ULONG flags, Animatable* prev)
{
    if (desc)
    {
        desc->BeginEditParams(ip, owner, flags, prev);
    }
}

void CADEndAutoUI(ClassDesc2* desc, IObjParam* ip, ReferenceMaker* owner,
                  ULONG flags, Animatable* prev)
{
    if (desc)
    {
        desc->EndEditParams(ip, owner, flags, prev);
    }
}
