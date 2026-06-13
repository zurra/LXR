// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXRSubsystem.h"
#include "DrawDebugHelpers.h"
#include "EngineUtils.h"
#include "LXR.h"
#include "LXROctreeVolume.h"
#include "LXRSourceComponent.h"
#include "LXRDetectionComponent.h"
#include "Math/GenericOctree.h"

static TAutoConsoleVariable<bool> CVarLXRDetectionDebugWidget(
	TEXT("lxr.debug.widget"),
	0,
	TEXT("Render the LXR Debug Widgets.\n")
	TEXT("  0: Nope\n")
	TEXT("  1: Yep\n"),
	ECVF_Scalability | ECVF_RenderThreadSafe);

DEFINE_LOG_CATEGORY(LogLXR);

void ULXRSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	if (GetWorld()->WorldType == EWorldType::Game || GetWorld()->WorldType == EWorldType::PIE)
	{
		GetWorld()->OnWorldBeginPlay.AddUObject(this, &ULXRSubsystem::CreateOctree);

		ConsoleHandle = IConsoleManager::Get().RegisterConsoleVariableSink_Handle(FConsoleCommandDelegate::CreateUObject(this, &ULXRSubsystem::OnDebugWidgetCvarChanged));

	}

	Super::Initialize(Collection);
}

void ULXRSubsystem::Deinitialize()
{
	Super::Deinitialize();
	if (LXROctreeObject.IsValid())
	{
		LXROctreeObject->Destroy();
		LXROctreeObject = nullptr;
	}
	IConsoleManager::Get().UnregisterConsoleVariableSink_Handle(ConsoleHandle);
}

void ULXRSubsystem::RegisterLight(ULXRSourceComponent* LightSource)
{
	if (LightSource->bSolo)
		bSoloFound = true;

	LightSources.AddUnique(LightSource);
	if (!LXROctreeObject.IsValid())
	{
		LXRActorsNotInOctreeYetBuffer.Add(LightSource->GetOwner());
	}
	else
	{
		AddLXRActorToTree(LightSource->GetOwner());
	}

	OnLightAdded.Broadcast(LightSource);
}

void ULXRSubsystem::OnDebugWidgetCvarChanged()
{
	bool b = CVarLXRDetectionDebugWidget.GetValueOnGameThread();

	if (b != bLXRDetectionDebugWidgetEnabled)
	{
		bLXRDetectionDebugWidgetEnabled = b;

		if (bLXRDetectionDebugWidgetEnabled)
		{
			if (!DetectionDebugWidget.IsValid())
			{
				TSubclassOf<UUserWidget> WidgetClass = LoadClass<UUserWidget>(NULL, TEXT("/LXR/Widget/WB_LxrCanvas.WB_LxrCanvas_C"));

				if (WidgetClass)
				{
					UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), WidgetClass);
					if (Widget)
					{
						DetectionDebugWidget = Widget;
					}
				}
			}
			if (DetectionDebugWidget.IsValid())
			{
				DetectionDebugWidget->AddToViewport();
			}
		}
		else
		{
			if (DetectionDebugWidget.IsValid())
			{
				DetectionDebugWidget->RemoveFromParent();
			}
		}
	}
}

void ULXRSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bSoloFound)
		GEngine->AddOnScreenDebugMessage(uint64(this) + 51, GetWorld()->DeltaTimeSeconds, FColor::Red, FString::Printf(TEXT("SOLO LIGHT DETECTED! \n ONLY SOLO LIGHTS WILL WORK WITH LXR")));
}

TStatId ULXRSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(ULXRSubsystem, STATGROUP_Tickables);
}

void ULXRSubsystem::UnregisterLight(ULXRSourceComponent* LightSource)
{
	if (LightSources.Contains(LightSource))
	{
		LightSources.Remove(LightSource);
		OnLightRemoved.Broadcast(LightSource);
	}

	if (LXROctreeObject.IsValid())
	{
		RemoveOctreeElement(LightSource->GetOwner());
	}
	else
	{
		LXRActorsNotInOctreeYetBuffer.Remove(LightSource->GetOwner());
	}
}

const TArray<TWeakObjectPtr<ULXRSourceComponent>>& ULXRSubsystem::GetAllLights() const
{
	return LightSources;
}


// OCTREE STUFF

void ULXRSubsystem::RegisterDetection(ULXRDetectionComponent* DetectionActor)
{
	DetectionComponents.AddUnique(DetectionActor);
	AddLXRActorToTree(DetectionActor->GetOwner());
}

void ULXRSubsystem::UnregisterDetection(ULXRDetectionComponent* DetectionActor)
{
	DetectionComponents.Remove(DetectionActor);
}

void ULXRSubsystem::CreateOctree()
{
	if (LXROctreeObject.IsValid())
		return;
	float OctreeRadius = 64000.f;
	FVector OctreeLocation = FVector::Zero();


	const TActorIterator<ALXROctreeVolume> It(GetWorld());
	if (It)
	{
		OctreeRadius = It->GetActorScale().GetMax();
		OctreeLocation = It->GetActorLocation();
		UE_LOG(LogLXR, Log, TEXT("Created Octree from LXROctree Volume"));
	}
	else
	{
		UE_LOG(LogLXR, Log, TEXT("Created Octree."));
	}
	LXROctreeObject = MakeShareable(new FLXROctreeObject(OctreeLocation, OctreeRadius));
	// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, FString::Printf(TEXT("Created Octree. Size %f"), OctreeRadius));

	for (TWeakObjectPtr<AActor> LightSource : LXRActorsNotInOctreeYetBuffer)
	{
		AddLXRActorToTree(LightSource.Get());
	}

	// TActorIterator<ALXROctreeVolume> It(GetWorld())
	// if(It)
	// {
	// }
	// for (TActorIterator<ALXROctreeVolume> It(GetWorld()); It; ++It)
	// {
	// 	LXROctree = MakeShareable(new FLXROctree(FVector(0, 0, 0), It->GetActorScale3D().GetMax() * 2));
	// 	return;
	// }

	// 	TArray<FLXROctreeElement> OctreeElements;
	//

	if constexpr (false)
	{
		TArray<FLXROctreeSemantics::FLXROctree::FNodeIndex> NodeIndexes;
		TArray<FBoxCenterAndExtent> NodesBounds;

		int elementCount = 0;


		GetOctree()->FindNodesWithPredicate([this, &NodesBounds](FLXROctreeSemantics::FLXROctree::FNodeIndex ParentNodeIndex, FLXROctreeSemantics::FLXROctree::FNodeIndex NodeIndex, const FBoxCenterAndExtent& NodeBounds)
		                                    {
			                                    NodesBounds.Add(NodeBounds);
			                                    return true;
		                                    }, [this,&elementCount, &NodeIndexes](FLXROctreeSemantics::FLXROctree::FNodeIndex ParentNodeIndex, FLXROctreeSemantics::FLXROctree::FNodeIndex NodeIndex,
		                                                                          const FBoxCenterAndExtent& NodeBounds)
		                                    {
			                                    NodeIndexes.Add(NodeIndex);
			                                    FVector maxExtent = NodeBounds.Extent;
			                                    FVector center = NodeBounds.Center;

			                                    DrawDebugBox(GetWorld(), center, maxExtent, FColor::Blue, false, 0.0f);
			                                    DrawDebugSphere(GetWorld(), center + maxExtent, 4.0f, 12, FColor::Green, false, 0.0f);
			                                    DrawDebugSphere(GetWorld(), center - maxExtent, 4.0f, 12, FColor::Red, false, 0.0f);

			                                    TArrayView<const FLXROctreeElement> elements = LXROctreeObject->Octree->GetElementsForNode(NodeIndex);

			                                    for (int i = 0; i < elements.Num(); i++)
			                                    {
				                                    // Draw debug boxes around elements
				                                    float max = elements[i].Bounds.BoxExtent.GetMax();
				                                    elementCount++;
			                                    }
		                                    });

		for (int i = 0; i < NodeIndexes.Num(); ++i)
		{
			TArrayView<const FLXROctreeElement> Elements = GetOctree()->GetElementsForNode(NodeIndexes[i]);
			bool HasElements = Elements.Num() > 0;
			DrawDebugBox(GetWorld(), NodesBounds[i].Center, NodesBounds[i].Extent, HasElements ? FColor::Green : FColor::Red, false, 10.0f, 0, HasElements ? 10 : 5);
			for (auto Element : Elements)
			{
				float Max = Element.Bounds.BoxExtent.GetMax();
				FVector MaxExtent = FVector(Max);
				FVector Center = Element.Data.Get().TreeActor->GetActorLocation();

				DrawDebugBox(GetWorld(), Center, MaxExtent, FColor().Yellow, false, 10.0f);
			}
		}

		LXROctreeObject->Octree->DumpStats();
	}
}


bool ULXRSubsystem::RemoveIDToElementMapping(const AActor* SourceObject)
{
	unsigned* UID = LXRObjectToUID.Find(SourceObject);
	if (!UID) return false;
	FOctreeElementId2* ElementID = UIDToElementID.Find(*UID);

	if (ElementID)
	{
		LXRObjectToUID.Remove(SourceObject);
		UIDToElementID.Remove(*UID);
		UIDToLXRActor.Remove(*UID);
		return true;
	}
	return false;
}

bool ULXRSubsystem::RemoveIDToElementMapping(const unsigned UID)
{
	FOctreeElementId2* ElementID = UIDToElementID.Find(UID);

	if (ElementID)
	{
		TWeakObjectPtr<const AActor>* SourceActor = UIDToLXRActor.Find(UID);
		if (!SourceActor->IsValid())
		{
			for (auto Pair : LXRObjectToUID)
			{
				if (SourceActor->Get() != NULL)
					break;

				if (Pair.Value == UID)
					SourceActor = &Pair.Key;
			}
		}

		if (IsValid(SourceActor->Get()))
			LXRObjectToUID.Remove(*SourceActor);


		UIDToElementID.Remove(UID);
		UIDToLXRActor.Remove(UID);
		return true;
	}
	return false;
}


void ULXRSubsystem::FindLXRPoints(TArray<FLXROctreeElement>& OutElements, const FBoxCenterAndExtent& QueryBox) const
{
	FRWScopeLock TreeDataLock(LXRTreeLockObject, FRWScopeLockType::SLT_ReadOnly);
	LXROctreeObject->FindElements(OutElements, QueryBox);
}

void ULXRSubsystem::FindLXRPoints(TArray<FLXROctreeElement>& OutElements, const FSphere& QuerySphere) const
{
	FRWScopeLock TreeDataLock(LXRTreeLockObject, FRWScopeLockType::SLT_ReadOnly);
	LXROctreeObject->FindElements(OutElements, QuerySphere);
}

void ULXRSubsystem::AddLXRPoints(const TArray<FDTOLXRData>& LXRDTOs)
{
	FRWScopeLock TreeDataLock(LXRTreeLockObject, FRWScopeLockType::SLT_Write);
	for (FDTOLXRData DTO : LXRDTOs)
	{
		LXROctreeObject->AddLxrElement(DTO);
	}
}

void ULXRSubsystem::AddLXRActorToTree(AActor* Actor)
{
	FRWScopeLock TreeDataLock(LXRTreeLockObject, FRWScopeLockType::SLT_Write);
	if (LXROctreeObject.IsValid())
	{
		ELXROctreeElementType ElementType = Actor->GetComponentByClass(ULXRSourceComponent::StaticClass()) == NULL ? Detection : Actor->GetComponentByClass(ULXRDetectionComponent::StaticClass()) == NULL ? Source : Both;
		FDTOLXRData Data = FDTOLXRData(Actor, Actor->GetUniqueID(), ElementType);
		LXROctreeObject->AddLxrElement(Data);
		if (ElementType == Source || ElementType == Both)
		{
			InformNearbyDetectionActorsToUpdateOctreeLights(Actor);
		}
	}
}

void ULXRSubsystem::RemoveStaleLXRPoints(FBox Area)
{
	FRWScopeLock TreeDataLock(LXRTreeLockObject, FRWScopeLockType::SLT_Write);
	TArray<FLXROctreeElement> Elements;
	LXROctreeObject->FindElements(Elements, Area);
	for (FLXROctreeElement Element : Elements)
	{
		if (IsValid(Element.GetOwner()))
			continue;

		FOctreeElementId2 Id;
		GetElementID(Id, Element.Data->UID);
		if (Id.IsValidId())
		{
			LXROctreeObject->RemoveElement(Id);
		}

		RemoveIDToElementMapping(Element.Data->UID);
	}
	LXROctreeObject->Octree->ShrinkElements();
}

void ULXRSubsystem::RemoveStaleLXRPoints(FVector Origin, FVector Extent)
{
	RemoveStaleLXRPoints(FBoxCenterAndExtent(Origin, Extent).GetBox());
}

void ULXRSubsystem::RemoveOctreeElement(const AActor* SourceObject)
{
	FRWScopeLock TreeDataLock(LXRTreeLockObject, FRWScopeLockType::SLT_Write);
	if (IsValid(SourceObject))
	{
		auto UID = LXRObjectToUID.Find(SourceObject);
		if (UID == NULL) return;
		auto ElementID = UIDToElementID.Find(*UID);
		if (ElementID && ElementID->IsValidId())
		{
			LXROctreeObject->RemoveElement(*ElementID);
			RemoveIDToElementMapping(SourceObject);
			LXROctreeObject->Octree->ShrinkElements();
		}
	}
}

void ULXRSubsystem::RemoveOctreeElement(const unsigned UID)
{
	FRWScopeLock TreeDataLock(LXRTreeLockObject, FRWScopeLockType::SLT_Write);
	const FOctreeElementId2* Element = UIDToElementID.Find(UID);
	if (Element && Element->IsValidId())
	{
		LXROctreeObject->RemoveElement(*Element);
		RemoveIDToElementMapping(UID);
		LXROctreeObject->Octree->ShrinkElements();
	}
}

void ULXRSubsystem::AssignIDToElement(const AActor* SourceObject, FOctreeElementId2 ID)
{
	uint32 UID = SourceObject->GetUniqueID();
	LXRObjectToUID.Add(SourceObject, UID);
	UIDToLXRActor.Add(UID, SourceObject);
	UIDToElementID.Add(UID, ID);
}

void ULXRSubsystem::UpdateElement(const unsigned UID)
{
	FOctreeElementId2 Id;
	GetElementID(Id, UID);
	if (Id.IsValidId())
	{
		FLXROctreeElement Element = LXROctreeObject->Octree->GetElementById(Id);
		Element.Bounds = FSphere(Element.Data->TreeActor->GetActorLocation(), 1);

		InformNearbyDetectionActorsToUpdateOctreeLights(Element.Data->TreeActor.Get());

		UE_LOG(LogLXR, Verbose, TEXT("Updated Octree Element :  %s"), *Element.Data->TreeActor->GetName());
	}
}

TTuple<FVector*, FRotator*>* ULXRSubsystem::GetPlayerCameraLocationAndRotation()
{
	float CurrentTime = GetWorld()->TimeSeconds;
	if (CurrentTime > (PlayerCameraUpdateTimer + 1))
	{
		PlayerCameraUpdateTimer = GetWorld()->TimeSeconds;
		const APlayerController* FirstPC = GetWorld()->GetFirstPlayerController();
		if (!FirstPC || !FirstPC->PlayerCameraManager) return &PlayerCameraLocationAndRotation;
		const APlayerCameraManager* PlayerCameraManager = FirstPC->PlayerCameraManager;
		*PlayerCameraLocationAndRotation.Key = PlayerCameraManager->GetCameraLocation();
		*PlayerCameraLocationAndRotation.Value = PlayerCameraManager->GetCameraRotation();
	}

	return &PlayerCameraLocationAndRotation;
}

FVector ULXRSubsystem::UpdateElement(AActor* SourceObject)
{
	if (!IsValid(SourceObject))
		return {};

	unsigned UID = SourceObject->GetUniqueID();
	FOctreeElementId2 Id;
	GetElementID(Id, UID);
	if (!Id.IsValidId()) return {};

	FLXROctreeElement Element = LXROctreeObject->Octree->GetElementById(Id);
	Element.Bounds = FSphere(SourceObject->GetActorLocation(), 1);
	// RemoveOctreeElement(SourceObject);
	// AddLXRActorToTree(SourceObject);

	InformNearbyDetectionActorsToUpdateOctreeLights(SourceObject);

	UE_LOG(LogLXR, Verbose, TEXT("Updated Octree Element :  %s"), *SourceObject->GetName());
	return SourceObject->GetActorLocation();


	// TArray<FVector> LXRElementLocations;
	// LXRObjectToID.MultiFind(SourceObject, LXRElementLocations, false);
	// for (const FVector LxrElementLocation : LXRElementLocations)
	// {
	// 	FOctreeElementId2 ElementID;
	// 	GetElementID(ElementID, LxrElementLocation);
	// 	LXROctree->RemoveElement(ElementID);
	// 	RemoveIDToElementMapping(LxrElementLocation);
	// 	LXRObjectToID.Remove(SourceObject);
	// }
	//
	// LXROctree->ShrinkElements();
}

bool ULXRSubsystem::GetElementID(FOctreeElementId2& OutElementID, const AActor* SourceObject) const
{
	const unsigned* UID = LXRObjectToUID.Find(SourceObject);
	if (!UID) return false;
	return GetElementID(OutElementID, *UID);
}

bool ULXRSubsystem::GetElementID(FOctreeElementId2& OutElementID, const unsigned UID) const
{
	const FOctreeElementId2* Element = UIDToElementID.Find(UID);
	if (!Element || !Element->IsValidId())
		return false;

	OutElementID = *Element;
	return true;
}

// ULXRMethodObject* ULXRSubsystem::GetOrAddMethodObject(const UClass* MethodObjectClass)
// {
// 	if (!InUseMethodObjectMap.Contains(MethodObjectClass))
// 	{
// 		ULXRMethodObject* MethodObject = NewObject<ULXRMethodObject>(this, MethodObjectClass);
// 		InUseMethodObjectMap.Add(MethodObjectClass, MethodObject);
// 	}
// 	return InUseMethodObjectMap[MethodObjectClass];
// }

void ULXRSubsystem::InformNearbyDetectionActorsToUpdateOctreeLights(const AActor* SourceActor)
{
	FBoxCenterAndExtent BoxCenterAndExtent;
	BoxCenterAndExtent.Center = SourceActor->GetActorLocation();
	BoxCenterAndExtent.Extent = FVector4(FVector(1000));

	if (GetWorld()->RealTimeSeconds < 2) return;

	// GEngine->AddOnScreenDebugMessage(90, 2.f, FColor::Magenta, FString::Printf(TEXT("FFAFAF %s"), *SourceActor->GetName()));
	// DrawDebugSphere(GetWorld(), SourceActor->GetActorLocation(), 25, FMath::Clamp<int32>(50 / 4.f, 8, 32), FColor::Red, false, 10.f);
	// DrawDebugBox(GetWorld(), BoxCenterAndExtent.Center, BoxCenterAndExtent.Extent, FColor::Red, false, 10.f);

	GetOctree()->FindElementsWithBoundsTest(BoxCenterAndExtent, [this, &SourceActor](const FLXROctreeElement& Element)
	{
		// DrawDebugSphere(GetWorld(), SourceActor->GetActorLocation(), 25, FMath::Clamp<int32>(50 / 4.f, 8, 32), FColor::Red, false, 10.f);
		if (Element.GetOwner() == SourceActor) return;
		if (Element.Data->OctreeElementType == Detection || Element.Data->OctreeElementType == Both)
		{
			AActor* LXRActor = Element.Data->TreeActor.Get();
			if (IsValid(LXRActor))
			{
				if (ULXRDetectionComponent* DetectionComponent = Cast<ULXRDetectionComponent>(LXRActor->GetComponentByClass(ULXRDetectionComponent::StaticClass())))
				{
					// DrawDebugDirectionalArrow(GetWorld(), SourceActor->GetActorLocation(), Element.Data->SourceObject.Get()->GetActorLocation(), 25.f, FColor::Cyan, false, 2.f, 0, 2);
					DetectionComponent->ForceUpdateOctreeLights();
				}
			}
		}
	});


	return;

	LXROctreeObject->Octree->FindNearbyElements(SourceActor->GetActorLocation(), [this, &SourceActor](const FLXROctreeElement& Element)
	{
		DrawDebugSphere(GetWorld(), SourceActor->GetActorLocation(), 25, FMath::Clamp<int32>(50 / 4.f, 8, 32), FColor::Red, false, 10.f);
		if (Element.GetOwner() == SourceActor) return;
		if (Element.Data->OctreeElementType == Detection || Element.Data->OctreeElementType == Both)
		{
			AActor* LXRActor = Element.Data->TreeActor.Get();
			if (IsValid(LXRActor))
			{
				if (ULXRDetectionComponent* DetectionComponent = Cast<ULXRDetectionComponent>(LXRActor->GetComponentByClass(ULXRDetectionComponent::StaticClass())))
				{
					DrawDebugDirectionalArrow(GetWorld(), SourceActor->GetActorLocation(), Element.Data->TreeActor.Get()->GetActorLocation(), 25.f, FColor::Cyan, false, 2.f, 0, 2);
					DetectionComponent->ForceUpdateOctreeLights();
				}
			}
		}
	});
	return;
}

void ULXRSubsystem::GetOctreeLights(FBoxCenterAndExtent& OctreeBoundsTestObject, TSet<TWeakObjectPtr<ULXRSourceComponent>>& Lights, bool bDebug) const
{
	GetOctree()->FindElementsWithBoundsTest(OctreeBoundsTestObject, [&](const FLXROctreeElement& Element)
	{
		if (Element.Data->OctreeElementType == Detection) return;

		if (Element.Data.Get().TreeActor.IsValid())
		{
			const TWeakObjectPtr<AActor> SourceActor = Element.Data.Get().TreeActor;
			ULXRSourceComponent* LightSourceComponent = Cast<ULXRSourceComponent>(SourceActor->GetComponentByClass(ULXRSourceComponent::StaticClass()));
			if ((bSoloFound && !LightSourceComponent->bSolo) || !LightSourceComponent->IsEnabled()) return;

			Lights.Add(LightSourceComponent);
		}
	});

	if (bDebug)
	{
		constexpr float DebugDrawTimeToUse = 1;
#if UE_BUILD_DEVELOPMENT
		{
			GEngine->AddOnScreenDebugMessage(95, DebugDrawTimeToUse, FColor::Magenta, FString::Printf(TEXT("OctreeLights: %d"), Lights.Num()));
		}
#endif

		for (auto& LightActor : Lights)
		{
			if (LightActor.IsValid())
			{
				DrawDebugDirectionalArrow(GetWorld(), OctreeBoundsTestObject.Center, LightActor->GetOwner()->GetActorLocation(), 150, FColor::Blue, false, DebugDrawTimeToUse, 0, 2);
			}
		}
	}
}
