#pragma once

#include "CADBundle.h"

#include <maxapi.h>
#include <iparamb2.h>
#include <ref.h>

// Shared ParamBlock2 lifecycle helpers for the four helper objects.
// The owning class keeps a single reference at slot 0; these helpers do not
// change the object's inheritance or reference layout.

// Creates the auto pblock and, when defaultSizeModeParamId >= 0, initializes
// that parameter to model space (1) for newly created objects. Old scenes that
// do not store the parameter still fall back to its p_default (screen mode),
// so legacy scenes keep their appearance.
void CADMakeAutoParamBlocks(ReferenceMaker* owner, ClassDesc2* desc,
                            int defaultSizeModeParamId = -1);

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

// Fills a dropdown with the fonts available through the BezFont manager and
// selects the value stored in pblock[paramId]. Missing current values are
// appended so an unavailable font still round-trips.
void CADPopulateFontCombo(HWND hDlg, int controlId, IParamBlock2* pblock,
                          int paramId);

// Writes the selected dropdown text into pblock[paramId] as one undoable edit
// and redraws the viewports.
void CADApplyFontCombo(HWND hDlg, int controlId, IParamBlock2* pblock,
                       int paramId, ReferenceMaker* owner, int stringId);

// Re-selects the dropdown entry that matches pblock[paramId]. Used after
// undo/redo or any programmatic pblock change, where ParamMap2 cannot update
// this custom (non-p_ui) combobox itself.
void CADSyncFontComboSelection(HWND hDlg, int controlId,
                               IParamBlock2* pblock, int paramId);

// Vertically centers the text inside every CustEdit control of a parameter
// dialog. Uses the SDK's ICustEdit::SetLeading, which offsets the text within
// the control, so the dialog layout does not change.
void CADCenterCustEditText(HWND hDlg);

// Two-item mode selector helpers (0 = screen constant, 1 = model space).
// CADApplySizeModeCombo converts every parameter listed in sizeParamIds when
// the mode changes so the on-screen size is preserved: screen mode stores
// pixels, model mode stores world units. objectAnchor is an object-space point
// used to measure the viewport scale; pass the origin to use the object pivot.
void CADPopulateSizeModeCombo(HWND hDlg, int controlId, int sizeMode);
void CADApplySizeModeCombo(HWND hDlg, int controlId, IParamBlock2* pblock,
                           int paramId, ReferenceMaker* owner,
                           const Point3& objectAnchor, const int* sizeParamIds,
                           int sizeParamCount);
void CADSyncSizeModeCombo(HWND hDlg, int controlId, int sizeMode);
