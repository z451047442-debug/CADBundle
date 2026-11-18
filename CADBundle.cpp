//**************************************************************************/
// Copyright (c) 1998-2020 Autodesk, Inc.
// All rights reserved.
// 
// Use of this software is subject to the terms of the Autodesk license 
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.
//**************************************************************************/
// DESCRIPTION: Plugin Wizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "CADBundle.h"

#define CADDimension_CLASS_ID Class_ID(0x34883ae1, 0x5594bda4)

#define PBLOCK_REF 0

class CADDimension : public HelperObject
{
public:
	// Parameter block
	IParamBlock2* pblock; // ref 0

	// Loading/Saving
	IOResult Load(ILoad* /*iload*/) { return IO_OK; }
	IOResult Save(ISave* /*isave*/) { return IO_OK; }

	// From Animatable
	Class_ID ClassID() { return CADDimension_CLASS_ID; }
	SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
	void GetClassName(TSTR& s, bool localized) const override { s = localized ? GetString(IDS_CLASS_NAME) : _T("CADDimension"); }

	RefTargetHandle Clone(RemapDir& remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }

	int NumParamBlocks() { return 1; } // return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int /*i*/) { return pblock; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() override;

	// From Object
	ObjectState Eval(TimeValue t) override;

	// Constructor/Destructor
	CADDimension() {}
	virtual ~CADDimension() {}
};


class CADDimensionClassDesc : public ClassDesc2 
{
public:
	int           IsPublic() override                               { return TRUE; }
	void*         Create(BOOL /*loading = FALSE*/) override         { return new CADDimension(); }
	const TCHAR*  ClassName() override                              { return GetString(IDS_CLASS_NAME); }
	const TCHAR*  NonLocalizedClassName() override                  { return _T("CADDimension"); }
	SClass_ID     SuperClassID() override                           { return HELPER_CLASS_ID; }
	Class_ID      ClassID() override                                { return CADDimension_CLASS_ID; }
	const TCHAR*  Category() override                               { return GetString(IDS_CATEGORY); }

	const TCHAR*  InternalName() override                           { return _T("CADDimension"); } // Returns fixed parsable name (scripter-visible name)
	HINSTANCE     HInstance() override                              { return hInstance; } // Returns owning module handle


};

ClassDesc2* GetCADDimensionDesc()
{
	static CADDimensionClassDesc CADDimensionDesc;
	return &CADDimensionDesc; 
}




RefTargetHandle CADDimension::Clone(RemapDir& /*remap*/)
{
#pragma message(TODO("To be implemented"))
	return nullptr;
}

RefResult CADDimension::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle /*hTarget*/, PartID& /*partID*/, RefMessage /*message*/, BOOL /*propagate*/)
{
#pragma message(TODO("To be implemented"))
	return REF_SUCCEED;
}

// From BaseObject
CreateMouseCallBack* CADDimension::GetCreateMouseCallBack()
{
#pragma message(TODO("To be implemented"))
	return nullptr;
}

// From Object
ObjectState CADDimension::Eval(TimeValue /*t*/)
{
#pragma message(TODO("To be implemented"))
	return ObjectState(this);
}
//**************************************************************************/
// Copyright (c) 1998-2020 Autodesk, Inc.
// All rights reserved.
// 
// Use of this software is subject to the terms of the Autodesk license 
// agreement provided at the time of installation or download, or which 
// otherwise accompanies this software in either electronic or hard copy form.
//**************************************************************************/
// DESCRIPTION: Plugin Wizard generated plugin
// AUTHOR: 
//***************************************************************************/

#include "CADBundle.h"

#define CADDimension_CLASS_ID Class_ID(0xa517250d, 0x8cd4f7c3)

#define PBLOCK_REF 0

class CADDimension : public HelperObject
{
public:
	// Parameter block
	IParamBlock2* pblock; // ref 0

	// Loading/Saving
	IOResult Load(ILoad* /*iload*/) { return IO_OK; }
	IOResult Save(ISave* /*isave*/) { return IO_OK; }

	// From Animatable
	Class_ID ClassID() { return CADDimension_CLASS_ID; }
	SClass_ID SuperClassID() { return HELPER_CLASS_ID; }
	void GetClassName(TSTR& s, bool localized) const override { s = localized ? GetString(IDS_CLASS_NAME) : _T("CADDimension"); }

	RefTargetHandle Clone(RemapDir& remap);
	RefResult NotifyRefChanged(const Interval& changeInt, RefTargetHandle hTarget, PartID& partID, RefMessage message, BOOL propagate);

	int NumSubs() { return 1; }

	int NumParamBlocks() { return 1; } // return number of ParamBlocks in this instance
	IParamBlock2* GetParamBlock(int /*i*/) { return pblock; } // return i'th ParamBlock
	IParamBlock2* GetParamBlockByID(BlockID id) { return (pblock->ID() == id) ? pblock : NULL; } // return id'd ParamBlock

	void DeleteThis() { delete this; }

	// From BaseObject
	CreateMouseCallBack* GetCreateMouseCallBack() override;

	// From Object
	ObjectState Eval(TimeValue t) override;

	// Constructor/Destructor
	CADDimension() {}
	virtual ~CADDimension() {}
};


class CADDimensionClassDesc : public ClassDesc2 
{
public:
	int           IsPublic() override                               { return TRUE; }
	void*         Create(BOOL /*loading = FALSE*/) override         { return new CADDimension(); }
	const TCHAR*  ClassName() override                              { return GetString(IDS_CLASS_NAME); }
	const TCHAR*  NonLocalizedClassName() override                  { return _T("CADDimension"); }
	SClass_ID     SuperClassID() override                           { return HELPER_CLASS_ID; }
	Class_ID      ClassID() override                                { return CADDimension_CLASS_ID; }
	const TCHAR*  Category() override                               { return GetString(IDS_CATEGORY); }

	const TCHAR*  InternalName() override                           { return _T("CADDimension"); } // Returns fixed parsable name (scripter-visible name)
	HINSTANCE     HInstance() override                              { return hInstance; } // Returns owning module handle


};

ClassDesc2* GetCADDimensionDesc()
{
	static CADDimensionClassDesc CADDimensionDesc;
	return &CADDimensionDesc; 
}




RefTargetHandle CADDimension::Clone(RemapDir& /*remap*/)
{
#pragma message(TODO("To be implemented"))
	return nullptr;
}

RefResult CADDimension::NotifyRefChanged(const Interval& /*changeInt*/, RefTargetHandle /*hTarget*/, PartID& /*partID*/, RefMessage /*message*/, BOOL /*propagate*/)
{
#pragma message(TODO("To be implemented"))
	return REF_SUCCEED;
}

// From BaseObject
CreateMouseCallBack* CADDimension::GetCreateMouseCallBack()
{
#pragma message(TODO("To be implemented"))
	return nullptr;
}

// From Object
ObjectState CADDimension::Eval(TimeValue /*t*/)
{
#pragma message(TODO("To be implemented"))
	return ObjectState(this);
}
