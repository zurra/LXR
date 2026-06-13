// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#include "LXROctree.h"
#include "LXRSubsystem.h"

void FLXROctreeSemantics::SetElementId(const FLXROctreeElement& Element, FOctreeElementId2 ID)
{
	UWorld* World = NULL;
	UObject* Owner = Element.GetOwner();

	if (AActor* Actor = Cast<AActor>(Owner))
	{
		World = Actor->GetWorld();
	}

	if (World)
	{
		ULXRSubsystem* LXRSubSystem = World->GetSubsystem<ULXRSubsystem>();
		uint32 UID = Element.Data->TreeActor->GetUniqueID();
		LXRSubSystem->AssignIDToElement(Element.Data->TreeActor.Get(), ID);
	}
}

// void FLXROctreeSemantics::SetElementId(FOctree& OctreeOwner, const FLXROctreeElement& Element, FOctreeElementId2 Id)
// {
// 	static_cast<FLXROctree&>(OctreeOwner).SetElementIdImpl(Element.GetOwner()->GetUniqueID(), Id);
// }

FLXROctreeObject::FLXROctreeObject()
{
}

FLXROctreeObject::FLXROctreeObject(const FVector& Origin, float Radius)
{
	// Octree = TOctree2(Origin, Radius);
	Octree = MakeShareable(new FLXROctreeSemantics::FLXROctree(Origin, Radius));
}

FLXROctreeObject::~FLXROctreeObject()
{
}

void FLXROctreeObject::Destroy()
{
	if(Octree.IsValid())
		Octree->Destroy();
	
}

void FLXROctreeObject::AddLxrElement(FDTOLXRData& InData)
{
	Octree->AddElement(FLXROctreeElement(InData));
}

void FLXROctreeObject::RemoveElement(FOctreeElementId2 ElementID)
{
	if (!ElementID.IsValidId())
		return;

	Octree->RemoveElement(ElementID);
	// static_cast<TOctree2*>(Octree)->RemoveElement(ElementID);
}

bool FLXROctreeObject::AnyElementsWithinBounds(const FBoxCenterAndExtent& QueryBox) const
{
	bool Result = false;
	Octree->FindFirstElementWithBoundsTest(QueryBox, [&Result](const FLXROctreeElement& Element)
	{
		Result = true;
		return true;
	});
	return Result;
}

void FLXROctreeObject::FindElements(TArray<FLXROctreeElement>& OutElements, const FBoxCenterAndExtent& QueryBox) const
{
	Octree->FindElementsWithBoundsTest(QueryBox, [&OutElements](const FLXROctreeElement& Element)
	{
		OutElements.Add(Element);
	});
}

void FLXROctreeObject::FindElements(TArray<FLXROctreeElement>& OutElements, const FSphere& QuerySphere) const
{
	const FBoxCenterAndExtent BoxFromSphere = FBoxCenterAndExtent(QuerySphere.Center, FVector(QuerySphere.W));
	Octree->FindElementsWithBoundsTest(BoxFromSphere, [&OutElements,&QuerySphere](const FLXROctreeElement& Element)
	{
		if (QuerySphere.Intersects(Element.Bounds.GetSphere()))
			OutElements.Add(Element);
	});
}

// void FLXROctree::SetElementIdImpl(const uint32 OwnerUniqueId, FOctreeElementId2 Id)
// {
// 	ObjectToOctreeId.Add(OwnerUniqueId, Id);
// }
