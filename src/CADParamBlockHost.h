#pragma once

#include "CADBundle.h"

#include <maxapi.h>
#include <iparamb2.h>
#include <ref.h>

// Shared ParamBlock2 lifecycle helpers for the four helper objects.
// The owning class keeps a single reference at slot 0; these helpers do not
// change the object's inheritance or reference layout.

void CADMakeAutoParamBlocks(ReferenceMaker* owner, ClassDesc2* desc);

ReferenceTarget* CADClonePBlockRef(IParamBlock2* pblock, RemapDir& remap);

// Assigns the pblock reference at slot 0. The object contract guarantees that
// slot 0 is always an IParamBlock2 or null.
void CADSetPBlockRef(IParamBlock2*& pblock, RefTargetHandle target);

// Standard REFMSG_CHANGE handling: invalidates the object's derived cache,
// notifies dependents, and asks the core interface to redraw.
RefResult CADNotifyPBlockChange(ReferenceMaker* owner, IParamBlock2* pblock,
                                RefTargetHandle target, PartID& partID,
                                RefMessage message, BOOL propagate,
                                Interface* ip, bool* invalidateCache);

// P_AUTO_UI wrappers for ClassDesc2.
void CADBeginAutoUI(ClassDesc2* desc, IObjParam* ip, ReferenceMaker* owner,
                    ULONG flags, Animatable* prev);
void CADEndAutoUI(ClassDesc2* desc, IObjParam* ip, ReferenceMaker* owner,
                  ULONG flags, Animatable* prev);
