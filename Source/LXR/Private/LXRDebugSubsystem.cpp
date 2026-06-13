// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXRDebugSubsystem.h"

#include "DisplayDebugHelpers.h"
#include "LXR.h"
#include "UObject/ConstructorHelpers.h"
#include "LXRDetectionComponent.h"
#include "LXRLightSenseComponent.h"
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "Widget/ULXRDebugFeatureContainer.h"


void FLXRDebugFeatures::UpdateComponentDebugSettings(ELXRDebugComponent ComponentToUpdate, const FQueryLXRDebugOptions& NewDebugOptions) const
{
	if (DebugActor == nullptr) return;
	switch (ComponentToUpdate)
	{
	case ELXRDebugComponent::Detection:
		{
			ULXRDetectionComponent* DetectionComponent = DebugActor->FindComponentByClass<ULXRDetectionComponent>();
			if (DetectionComponent)
				DetectionComponent->DebugOptions = NewDebugOptions;
		}
		break;
	case ELXRDebugComponent::Sense:
		if (ULXRLightSenseComponent* SenseComponent = DebugActor->FindComponentByClass<ULXRLightSenseComponent>())
			SenseComponent->DebugOptions = NewDebugOptions;
		break;
	}
}

void FLXRDebugFeatures::UpdateDebugFeatures(const FLXRDebugFeatures& Other)
{
	bEnableDirect = Other.bEnableDirect;
	bEnableSense = Other.bEnableSense;
	bEnableCapture = Other.bEnableCapture;
	bEnableSockets = Other.bEnableSockets;
	if (OnDebugFeaturesChanged.IsBound())
		OnDebugFeaturesChanged.Execute();
}


ULXRDebugSubsystem::ULXRDebugSubsystem(const FObjectInitializer&)
{
	static ConstructorHelpers::FClassFinder<UULXRDebugEntry> DebugEntryClassFinder(
		TEXT("/LXR/Widget/WB_LxrDebugEntry"));
	if (DebugEntryClassFinder.Succeeded())
	{
		UE_LOG(LogLXR, Log, TEXT("Found  %s"), *DebugEntryClassFinder.Class->GetName());
		DebugEntryWidgetClass = DebugEntryClassFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UULXRDebugCanvasWidget> DebugCanvasClassFinder(
		TEXT("/LXR/Widget/WB_LxrCanvas"));
	if (DebugCanvasClassFinder.Succeeded())
	{
		UE_LOG(LogLXR, Log, TEXT("Found  %s"), *DebugCanvasClassFinder.Class->GetName());
		DebugCanvasWidgetClass = DebugCanvasClassFinder.Class;
	}
}

FLinearColor ULXRDebugSubsystem::GetDebugColor()
{
	const int32 CurrentIndex = DebugActorCounter++;
	float Hue = FMath::Fmod((CurrentIndex * 0.61803398875f), 1.0f); // Golden ratio to avoid clustering
	float Saturation = 0.8f; // Keep saturation high for visibility
	float Value = 0.9f; // Bright colors

	return FLinearColor::MakeFromHSV8(Hue * 255, Saturation * 255, Value * 255);
}

void ULXRDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	if (GetWorld()->GetNetMode() != ENetMode::NM_DedicatedServer)
	{
		AHUD::OnShowDebugInfo.AddUObject(this, &ULXRDebugSubsystem::DisplayDebug);
	}
}

void ULXRDebugSubsystem::Deinitialize()
{
	AHUD::OnShowDebugInfo.RemoveAll(this);
	Super::Deinitialize();
}

void ULXRDebugSubsystem::AddDebugActor(AActor* InActor, const FLXRDebugFeatures& InDebugFeatures, bool IsLocalControllerActor)
{
	if (InActor == nullptr) return;

	if (FindEntryByActor(InActor) != nullptr) return;

	if (CanvasWidget == nullptr && IsValid(DebugCanvasWidgetClass))
	{
		CanvasWidget = CreateWidget<UULXRDebugCanvasWidget>(
			UGameplayStatics::GetPlayerController(GetWorld(), 0), DebugCanvasWidgetClass);

		if (CanvasWidget)
			CanvasWidget->AddToViewport();
	}

	if (CanvasWidget == nullptr) return;

	ULXRDebugEntryObject* Object = NewObject<ULXRDebugEntryObject>(this, ULXRDebugEntryObject::StaticClass());

	Object->Initialize(InActor, InDebugFeatures);
	DebugEntries.Add(Object);

	//This is good enough but obviously not correct for split screen..
	auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (!IsLocalControllerActor)
	{
		if (PC != nullptr)
		{
			APawn* PCPawn = PC->GetPawn();
			if (PCPawn)
			{
				AActor* PawnAsActor = Cast<AActor>(PCPawn);
				IsLocalControllerActor = PawnAsActor == InActor;
			}
		}
	}
	CanvasWidget->AddDebugObject(Object, IsLocalControllerActor);
}

void ULXRDebugSubsystem::UpdateActorDebugFeatures(AActor* InActor, const FLXRDebugFeatures& InDebugFeatures)
{
	ULXRDebugEntryObject* Entry = FindEntryByActor(InActor);
	if (!Entry) return;
	Entry->MyDebugFeatures->UpdateDebugFeatures(InDebugFeatures);
}

ULXRDebugEntryObject* ULXRDebugSubsystem::FindEntryByActor(const AActor* InActor)
{
	for (auto& Element : DebugEntries)
	{
		if (Element.Get()->GetMyDebugActor() == InActor)
			return Element.Get();
	}
	return nullptr;
}

void ULXRDebugSubsystem::RemoveDebugActor(AActor* InActor, bool IsLocalControllerActor)
{
	if (InActor == nullptr) return;
	if (CanvasWidget == nullptr) return;

	auto PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (!IsLocalControllerActor)
	{
		if (PC != nullptr)
		{
			APawn* PCPawn = PC->GetPawn();
			if (PCPawn)
			{
				AActor* PawnAsActor = Cast<AActor>(PCPawn);
				IsLocalControllerActor = PawnAsActor == InActor;
			}
		}
	}

	CanvasWidget->RemoveDebugObject(InActor, IsLocalControllerActor);

	TObjectPtr<ULXRDebugEntryObject> EntryToRemove = FindEntryByActor(InActor);
	if (EntryToRemove != nullptr)
	{
		DebugEntries.Remove(EntryToRemove);
	}

	
}

void ULXRDebugSubsystem::DisplayDebug(AHUD* HUD, UCanvas* Canvas, const FDebugDisplayInfo& DisplayInfo, float& YL, float& YPos)
{
	FDisplayDebugManager& DisplayDebugManager = Canvas->DisplayDebugManager;
	DisplayDebugManager.SetDrawColor(FColor(255, 255, 0));
	ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer) return;
	if (DisplayInfo.IsDisplayOn(FName("lxr")))
	{
		APlayerController* PlayerController = LocalPlayer->PlayerController;
		APawn* PCPawn = PlayerController->GetPawn();
		if (PCPawn)
		{
			if (ULXRDetectionComponent* DetectionComponent = PCPawn->GetComponentByClass<ULXRDetectionComponent>())
			{
				DetectionComponent->DisplayDebug(HUD, Canvas, DisplayInfo, YL, YPos);
			}
		}
	}
}
