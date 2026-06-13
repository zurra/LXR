// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "CoreMinimal.h"
#include "LXRSourceComponent.h"
#include "GameFramework/Actor.h"
#include "Math/GenericOctree.h"

enum LXR_API ELXROctreeElementType
{
	Source,
	Detection,
	Both
};

struct FDTOLXRData
{
public:
	AActor* LXRActor;
	uint32 UID;
	ELXROctreeElementType OctreeElementType;

	FDTOLXRData(): LXRActor(), UID(), OctreeElementType()
	{
	}

	FDTOLXRData(AActor* InLXRSourceActor, uint32 InUID, ELXROctreeElementType InType) : LXRActor(InLXRSourceActor), UID(InUID), OctreeElementType(InType)
	{
	}
};

struct LXR_API FLXROctreeData : public TSharedFromThis<FLXROctreeData, ESPMode::ThreadSafe>
{
	const uint32 UID;
	const TWeakObjectPtr<AActor> TreeActor;
	TWeakObjectPtr<ULXRSourceComponent> SourceComponent;
	ELXROctreeElementType OctreeElementType;

	FLXROctreeData() : UID(), TreeActor(), OctreeElementType()
	{
	}

	FLXROctreeData(FDTOLXRData LxrData) : UID(LxrData.UID), TreeActor(LxrData.LXRActor), OctreeElementType(LxrData.OctreeElementType)
	{
		if (auto LxrSourceComponent = LxrData.LXRActor->GetComponentByClass<ULXRSourceComponent>())
		{
			SourceComponent = LxrSourceComponent;
		}
	}
};

struct FLXROctreeElement
{
public:
	TSharedRef<FLXROctreeData, ESPMode::ThreadSafe> Data;
	FBoxSphereBounds Bounds;

	FLXROctreeElement() : Data(new FLXROctreeData()), Bounds()
	{
	}

	FLXROctreeElement(FDTOLXRData& LxrData) : Data(new FLXROctreeData(LxrData)), Bounds(FSphere(LxrData.LXRActor->GetActorLocation(), 50))
	{
	}

	FORCEINLINE bool IsEmpty() const
	{
		const FBox BoundingBox = Bounds.GetBox();
		return BoundingBox.IsValid == 0 || BoundingBox.GetSize().IsNearlyZero();
	}

	FORCEINLINE UObject* GetOwner() const
	{
		return Data->TreeActor.Get();
	}
};

struct FLXROctreeSemantics
{
	enum { MaxElementsPerLeaf = 30 };

	enum { MinInclusiveElementsPerNode = 15 };

	enum { MaxNodeDepth = 12 };


	typedef TOctree2<FLXROctreeElement, FLXROctreeSemantics> FLXROctree;
	typedef TInlineAllocator<MaxElementsPerLeaf> ElementAllocator;

	FORCEINLINE static const FBoxSphereBounds& GetBoundingBox(const FLXROctreeElement& Element)
	{
		return Element.Bounds;
	}

	FORCEINLINE static bool AreElementsEqual(const FLXROctreeElement& A, const FLXROctreeElement& B)
	{
		return A.Data->TreeActor == B.Data->TreeActor;
	}

	static void SetElementId(const FLXROctreeElement& Element, FOctreeElementId2 ID);
	// static void SetElementId(FOctree& OctreeOwner, const FLXROctreeElement& Element, FOctreeElementId2 Id);
};

class FLXROctreeObject : public TSharedFromThis<FLXROctreeObject, ESPMode::ThreadSafe>
{
public:
	FLXROctreeObject();
	FLXROctreeObject(const FVector& Origin, float Radius);
	virtual ~FLXROctreeObject();
	void Destroy();

	void AddLxrElement(FDTOLXRData& InData);
	void RemoveElement(FOctreeElementId2 ElementID);
	bool AnyElementsWithinBounds(const FBoxCenterAndExtent& QueryBox) const;
	void FindElements(TArray<FLXROctreeElement>& OutElements, const FBoxCenterAndExtent& QueryBox) const;
	void FindElements(TArray<FLXROctreeElement>& OutElements, const FSphere& QuerySphere) const;


	TSharedPtr<FLXROctreeSemantics::FLXROctree, ESPMode::ThreadSafe> Octree;
};
