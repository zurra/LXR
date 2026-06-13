// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.


#include "LXRFlicker.h"
#include "LXR.h"
#include "LXRSourceComponent.h"
#include "Components/LightComponent.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "MaterialTypes.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/KismetMathLibrary.h"

ULXRFlicker::ULXRFlicker()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAutoActivate = true;
}


void ULXRFlicker::BeginPlay()
{
	Super::BeginPlay();

	//Lazy "late begin play"
	if (!bInitializeManually)
	{
		FTimerHandle Temp;
		GetWorld()->GetTimerManager().SetTimer(Temp, FTimerDelegate::CreateLambda([&]
		{
			if (RandomizeSeed)
				Seed = FMath::RandRange(0, 999999);

			InitializeFlicker();
		}), 0.1f, false);
	}
}

#if WITH_EDITOR
void ULXRFlicker::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	InitializeFlicker();
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ThisClass, Color))
	{
		ChangeColors();
	}
}
void ULXRFlicker::Activate(bool bReset)
{
	// if (bReset)
	// {
	// 	Reset();
	// 	InitializeFlicker();
	// }
	Super::Activate();
}
#endif

void ULXRFlicker::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (!bActive) return;
	CurrentNoiseValue = CalculateNoise();
	ChangeLightIntensities();
	if (bChangeLocation)
	{
		if (bChangeLocationSameForAllComponents)
		{
			MoveComponents();
		}
		else
		{
			for (const auto& Component : ChangeLocationComponents)
			{
				MoveComponent(*Component);
			}
		}
	}
	if (bUseEmissiveMaterial)
	{
		ModifyMaterials();
	}
	ChangeColors();
}

float ULXRFlicker::CalculateNoise() const
{
	const double Time = GetWorld()->GetTime().GetRealTimeSeconds();
	const double Scale = Time * Speed;
	const float NoiseValue = (FMath::PerlinNoise1D(Scale + Seed) + 1) * 0.5f;

	float FinalValue = NoiseValue;
	if (Posterization > 0)
	{
		FinalValue = FMath::Floor(NoiseValue * Posterization) / Posterization;
	}
	return FMath::GetMappedRangeValueClamped(TRange<float>(0.0f, 1.0f), OutValueRange, FinalValue);
}

void ULXRFlicker::Reset()
{
	bIsInitialized = false;
	EmissivePrimitivesAndMaterials.Reset();
	InitialIntensities.Reset();
	InitialLocations.Reset();
	NewLocations.Reset();
}

void ULXRFlicker::MoveComponents()
{
	const TWeakObjectPtr<USceneComponent> Component = ChangeLocationComponents[0];

	FChangeLocationData* LocationData = NewLocations.Find(Component);
	if (LocationData == nullptr)
	{
		UE_LOG(LogLXR, Error, TEXT("LocationData is null!"));
		return;
	}
	FVector* NewLocation = &LocationData->NewLocation;
	FVector* CurrentLocation = &LocationData->CurrentLocation;

	if (FVector::PointsAreNear(*CurrentLocation, *NewLocation, 10))
	{
		const FVector Center = InitialLocations[ChangeLocationComponents[0]];
		*NewLocation = UKismetMathLibrary::RandomPointInBoundingBox(Center, ChangeLocationRadius);
	}

	const float CurveMpl = 1;
	const float MaxDistance = LocationData->Distance;
	const float CurrentDistance = FVector::Distance(*CurrentLocation, *NewLocation);;
	RunTimeChangeLocationCurve.GetRichCurveConst()->Eval(FMath::Clamp(CurrentDistance / MaxDistance, 0, 1));

	const FVector DirectionToMove = (*NewLocation - *CurrentLocation).GetSafeNormal();
	*CurrentLocation = *CurrentLocation + DirectionToMove * ChangeLocationSpeed;
	const FVector FinalLocation = *CurrentLocation + (*NewLocation - *CurrentLocation).GetSafeNormal() * ChangeLocationSpeed * CurveMpl;

	for (const TWeakObjectPtr<USceneComponent> element : ChangeLocationComponents)
	{
		element->SetRelativeLocation(FinalLocation);
	}
}

void ULXRFlicker::MoveComponent(USceneComponent& Component)
{
	FChangeLocationData* LocationData = NewLocations.Find(TWeakObjectPtr<USceneComponent>(&Component));
	if (LocationData == nullptr)
	{
		UE_LOG(LogLXR, Error, TEXT("LocationData is null!"));
		return;
	}

	FVector* NewLocation = &LocationData->NewLocation;
	FVector* CurrentLocation = &LocationData->CurrentLocation;

	if (FVector::PointsAreNear(*CurrentLocation, *NewLocation, 1))
	{
		const FVector Center = InitialLocations[ChangeLocationComponents[0]];
		*NewLocation = UKismetMathLibrary::RandomPointInBoundingBox(Center, ChangeLocationRadius);
		LocationData->Distance = FVector::Distance(*CurrentLocation, *NewLocation);;
	}

	float CurveMpl = 1;
	const float MaxDistance = LocationData->Distance;
	const float CurrentDistance = FVector::Distance(*CurrentLocation, *NewLocation);
	const float DistancePercent = FMath::Abs((FMath::Clamp(CurrentDistance / MaxDistance, 0, 1))-1);
	float distanceDelta = DistancePercent-LastFramDistancePercent; 

	CurveMpl = RunTimeChangeLocationCurve.GetRichCurveConst()->Eval(DistancePercent);

	// const FVector FinalLocation = *CurrentLocation + (*NewLocation - *CurrentLocation).GetSafeNormal() * ChangeLocationSpeed * CurveMpl;
	 const FVector FinalLocation = FMath::Lerp(*CurrentLocation,*NewLocation,DistancePercent);
	const FVector DirectionToMove = (*NewLocation - *CurrentLocation).GetSafeNormal();
	*CurrentLocation = *CurrentLocation + DirectionToMove * ChangeLocationSpeed;

	// if (!RandomizeSeed)
	// {
	// 	DrawDebugDirectionalArrow(GetWorld(),*CurrentLocation,*NewLocation,25.f,FColor::Green);
	// 	GEngine->AddOnScreenDebugMessage(uint64(this), 0.2, FColor::Yellow, FString::Printf(TEXT("DistancePercent %f : Delta : %f"), DistancePercent,distanceDelta));
	// 	GEngine->AddOnScreenDebugMessage(uint64(this) + 1, 0.2, FColor::Green, FString::Printf(TEXT("CurveMpl %f"), CurveMpl));
	// 	LastFramDistancePercent = DistancePercent;
	// }
	 
	Component.SetRelativeLocation(FinalLocation);
}


void ULXRFlicker::ModifyMaterials()
{
	const float Min = OutValueRangeEmissive.GetLowerBoundValue();
	const float Max = OutValueRangeEmissive.GetUpperBoundValue();

	const float EmissiveValue = FMath::Lerp<float>(Min, Max, CurrentNoiseValue);

	for (const auto& Element : EmissivePrimitivesAndMaterials)
	{
		for (const auto& PrimitiveMaterial : Element.Value.PrimitiveMaterials)
		{
			Cast<UMaterialInstanceDynamic>(PrimitiveMaterial.Material)->SetScalarParameterValue(EmissiveParameterName, EmissiveValue);
		}
	}
}

void ULXRFlicker::ChangeLightIntensities()
{
	for (const auto& Element : InitialIntensities)
	{
		const TWeakObjectPtr<ULightComponent> LightComponent = Element.Key;
		if (LightComponent.IsValid())
		{
			if (LightComponent->GetOwner() != nullptr)
			{
				LightComponent->SetIntensity(Element.Value * CurrentNoiseValue * IntensityMultiplier);
				
			}
		}
	}
}

void ULXRFlicker::ChangeColors()
{
	if (MyLights.Num() > 0)
	{
		for (const auto& Element : MyLights)
		{
			if (IsValid(Element))
				Element->SetLightColor(Color);
		}
	}
	if (EmissivePrimitivesAndMaterials.Num() > 0)
	{
		for (const auto& Element : EmissivePrimitivesAndMaterials)
		{
			for (const auto PrimitiveMaterial : Element.Value.PrimitiveMaterials)
			{
				Cast<UMaterialInstanceDynamic>(PrimitiveMaterial.Material)->SetVectorParameterValue(FName("Color"), Color);
			}
		}
	}
}

void ULXRFlicker::FetchMyLights()
{
	SourceComponent = Cast<ULXRSourceComponent>(GetOwner()->GetComponentByClass(ULXRSourceComponent::StaticClass()));

	if (IsValid(SourceComponent))
	{
		MyLights = SourceComponent->GetMyLightComponents();
	}
	else
	{
		GetOwner()->GetComponents(MyLights);
	}

	if (MyLights.Num() > 0)
	{
		for (ULightComponent* LightComponent : MyLights)
		{
			if (!IsValid(LightComponent)) continue;

			float LightIntensity = LightComponent->Intensity;
			InitialIntensities.Add(LightComponent, LightIntensity);
		}
	}
}

void ULXRFlicker::SetupChangeLocationComponents()
{
	auto GetComponentsFromFComponentRefs([&](TArray<FComponentReference>& Refs)
	{
		TArray<USceneComponent*> SceneComponents;
		for (auto& Element : Refs)
		{
			SceneComponents.Add(Cast<USceneComponent>(Element.GetComponent(GetOwner())));
		}
		return SceneComponents;
	});

	auto GetLightComponentsAsUSceneComponents([&](TArray<TObjectPtr<ULightComponent>>& ComponentsToChange)
	{
		TArray<USceneComponent*> SceneComponents;
		for (const auto& Element : ComponentsToChange)
		{
			SceneComponents.Add(Cast<USceneComponent>(Element));
		}
		return SceneComponents;
	});

	TArray<USceneComponent*> ArrayToUse = ComponentsWhichShouldChangeLocation.Num() == 0
		                                      ? GetLightComponentsAsUSceneComponents(MyLights)
		                                      : GetComponentsFromFComponentRefs(ComponentsWhichShouldChangeLocation);

	for (USceneComponent* SceneComponent : ArrayToUse)
	{
		ChangeLocationComponents.Add(SceneComponent);
		InitialLocations.Add(SceneComponent, SceneComponent->GetRelativeLocation());
		FChangeLocationData Data = {SceneComponent->GetRelativeLocation(), 0, SceneComponent};
		NewLocations.Add(Data);
	}
}


void ULXRFlicker::InitializeFlicker()
{
	if (!IsValid(GetOwner())) return;

	if (!bIsInitialized)
	{
		FetchMyLights();
	}

	if (ChangeLocationCurve == nullptr)
	{
		auto Keys = RunTimeChangeLocationCurve.EditorCurveData.Keys;
		bool ResetRuntimeCurve = Keys.Num() == 0;

		if (Keys.Num() == 2)
		{
			for (int i = 0; i < Keys.Num(); ++i)
			{
				if (Keys[1].Value != i && Keys[0].Time != i)
				{
					ResetRuntimeCurve = true;
				}
			}
		}
		if (ResetRuntimeCurve)
		{
			RunTimeChangeLocationCurve.EditorCurveData.Reset();
			RunTimeChangeLocationCurve.EditorCurveData.AddKey(0, 1);
			RunTimeChangeLocationCurve.EditorCurveData.AddKey(1, 1);
			for (auto& Key : RunTimeChangeLocationCurve.EditorCurveData.Keys)
			{
				Key.InterpMode = ERichCurveInterpMode::RCIM_Linear;
			}
		}
	}
	else
	{
		const FRichCurve RichCurve = ChangeLocationCurve->FloatCurve;
		RunTimeChangeLocationCurve.EditorCurveData = RichCurve;
	}

	SetupChangeLocationComponents();

	if (bUseEmissiveMaterial)
	{
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		if (EmissiveMaterialPrimitives.Num() == 0)
		{
			GetOwner()->GetComponents(PrimitiveComponents);
		}
		else
		{
			PrimitiveComponents = EmissiveMaterialPrimitives;
		}


		for (UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
		{
			if (PrimitiveComponent->ComponentHasTag(EmissivePrimitiveTag) || EmissiveMaterialPrimitives.Num() > 0)
			{
				TArray<UMaterialInterface*> Materials;
				const FHashedMaterialParameterInfo ParameterInfo(EmissiveParameterName);

				PrimitiveComponent->GetUsedMaterials(Materials, false);
				int IndexOfMaterial = -1;
				for (UMaterialInterface* PrimitiveMaterial : Materials)
				{
					IndexOfMaterial++;
					bool Continue = false;
					if (EmissivePrimitivesAndMaterials.Contains(PrimitiveComponent))
					{
						const auto DataPtr = EmissivePrimitivesAndMaterials[PrimitiveComponent].Find(IndexOfMaterial);
						if (DataPtr != nullptr)
						{
							Continue = true;
							const auto DMI = DataPtr->Material;
							PrimitiveComponent->SetMaterial(IndexOfMaterial, DMI);
						}
					}

					if (Continue)
						continue;

					float Out;
					if (PrimitiveMaterial->GetScalarParameterValue(ParameterInfo, Out))
					{
						UMaterialInterface* DefaultMaterial = PrimitiveMaterial;

						if (const UMaterialInstanceDynamic* DynamicMaterialInstance = Cast<UMaterialInstanceDynamic>(PrimitiveMaterial))
						{
							DefaultMaterial = DynamicMaterialInstance->Parent;
						}

						const auto DMI = UKismetMaterialLibrary::CreateDynamicMaterialInstance(PrimitiveComponent, DefaultMaterial);
						PrimitiveComponent->SetMaterial(IndexOfMaterial, DMI);

						FPrimitiveMaterials* PrimitiveMaterials = nullptr;
						if (!EmissivePrimitivesAndMaterials.Contains(PrimitiveComponent))
							PrimitiveMaterials = &EmissivePrimitivesAndMaterials.Add(PrimitiveComponent);
						else
						{
							PrimitiveMaterials = &EmissivePrimitivesAndMaterials[PrimitiveComponent];
						}

						if (PrimitiveMaterials != nullptr)
						{
							PrimitiveMaterials->AddMaterial(IndexOfMaterial, DMI);
						}
					}
				}
			}
		}
	}
	bIsInitialized = true;
	ChangeColors();
}
