// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Curves/CurveFloat.h"
#include "LXRFlicker.generated.h"

class ULightComponent;

USTRUCT()
struct FPrimitiveMaterial
{
	GENERATED_BODY()

	FPrimitiveMaterial(): Material(nullptr), MaterialIndex(0)
	{
	}

	UPROPERTY()
	TObjectPtr<UMaterialInterface>
	Material;

	int MaterialIndex;

	FPrimitiveMaterial(int Index, UMaterialInterface* MaterialInterface)
	{
		MaterialIndex = Index,
			Material = MaterialInterface;
	}

	bool operator==(FPrimitiveMaterial Other) const
	{
		return (Material == Other.Material) && (MaterialIndex == Other.MaterialIndex);
	}

	bool operator==(int IndexOfMaterial) const
	{
		return MaterialIndex == IndexOfMaterial;
	}
};

USTRUCT()
struct FPrimitiveMaterials
{
	GENERATED_BODY()

	FPrimitiveMaterials()
	{
	}

	UPROPERTY()
	TArray<FPrimitiveMaterial> PrimitiveMaterials;

	void AddMaterial(int IndexOfMaterial, UMaterialInterface* MaterialInterface)
	{
		FPrimitiveMaterial* DataPtr = PrimitiveMaterials.FindByPredicate([IndexOfMaterial](const FPrimitiveMaterial& Data)
		{
			return Data.MaterialIndex == IndexOfMaterial;
		});

		if (DataPtr != nullptr)
		{
			DataPtr->Material = MaterialInterface;
			DataPtr->MaterialIndex = IndexOfMaterial;
		}
		else
		{
			PrimitiveMaterials.Add(FPrimitiveMaterial(IndexOfMaterial, MaterialInterface));
		}
	}

	bool Contains(int IndexOfMaterial, const UMaterialInterface* MaterialInterface)
	{
		FPrimitiveMaterial* DataPtr = PrimitiveMaterials.FindByPredicate([IndexOfMaterial, MaterialInterface](const FPrimitiveMaterial& Data)
		{
			return Data.MaterialIndex == IndexOfMaterial && Data.Material == MaterialInterface;
		});

		if (DataPtr != nullptr)
		{
			return true;
		}
		return false;
	}

	bool Contains(int IndexOfMaterial)
	{
		FPrimitiveMaterial* DataPtr = PrimitiveMaterials.FindByPredicate([IndexOfMaterial](const FPrimitiveMaterial& Data)
		{
			return Data.MaterialIndex == IndexOfMaterial;
		});

		if (DataPtr != nullptr)
		{
			return true;
		}
		return false;
	}

	FPrimitiveMaterial* Find(int IndexOfMaterial, const UMaterialInterface* MaterialInterface)
	{
		FPrimitiveMaterial* DataPtr = PrimitiveMaterials.FindByPredicate([IndexOfMaterial, MaterialInterface](const FPrimitiveMaterial& Data)
		{
			return Data.MaterialIndex == IndexOfMaterial && Data.Material == MaterialInterface;
		});

		if (DataPtr != nullptr)
		{
			return DataPtr;
		}
		return nullptr;
	}

	FPrimitiveMaterial* Find(int IndexOfMaterial)
	{
		FPrimitiveMaterial* DataPtr = PrimitiveMaterials.FindByPredicate([IndexOfMaterial](const FPrimitiveMaterial& Data)
		{
			return Data.MaterialIndex == IndexOfMaterial;
		});

		if (DataPtr != nullptr)
		{
			return DataPtr;
		}
		return nullptr;
	}
};

USTRUCT()
struct FChangeLocationData
{
	GENERATED_BODY()

	FChangeLocationData()
	{
	};
	FChangeLocationData(TWeakObjectPtr<USceneComponent> InComponent) { Component = InComponent; }

	FChangeLocationData(const FVector& Location, float InDistance, USceneComponent* InComponent)
	{
		Component = InComponent;
		CurrentLocation = Location;
		NewLocation = Location;
		Distance = InDistance;
	}

	TWeakObjectPtr<USceneComponent> Component;
	FVector CurrentLocation;
	FVector NewLocation;

	float Distance;

	bool operator==(const FChangeLocationData& Other) const
	{
		return Component == Other.Component;
	}

	bool operator==(USceneComponent* ComponentToCheck) const
	{
		return Component.Get() == ComponentToCheck;
	}
};

FORCEINLINE uint32 GetTypeHash(const FChangeLocationData& LXR)
{
	const uint32 Hash = FCrc::MemCrc32(&LXR, sizeof(FChangeLocationData));
	return Hash;
}


FORCEINLINE uint32 GetTypeHash(const FPrimitiveMaterials& LXR)
{
	const uint32 Hash = FCrc::MemCrc32(&LXR, sizeof(FPrimitiveMaterials));
	return Hash;
}


FORCEINLINE uint32 GetTypeHash(const FPrimitiveMaterial& LXR)
{
	const uint32 Hash = FCrc::MemCrc32(&LXR, sizeof(FPrimitiveMaterial));
	return Hash;
}

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class LXR_API ULXRFlicker : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	ULXRFlicker();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void Activate(bool bReset = false) override;
#endif

public:
	
	UPROPERTY(EditAnywhere, Category="LXR|Flicker")
	bool  bInitializeManually = false;
	
	UFUNCTION(BlueprintCallable, Category = "LXR|Flicker|Reset", meta=(CallInEditor = "true"))
	void Reset();

	//Is the flicker active
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="LXR|Flicker")
	bool bActive = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="LXR|Flicker")
	bool RandomizeSeed = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "RandomizeSeed == false"))
	float Seed = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker")
	float Speed = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker")
	float IntensityMultiplier = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker")
	FLinearColor Color = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker")
	float Posterization = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker")
	FFloatRange OutValueRange = FFloatRange(0.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker")
	bool bChangeLocation = false;

	//List of Components which are affected by the Change Location, only parent of childs is needed.
	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(UseComponentPicker, EditCondition = "bChangeLocation == true"))
	TArray<FComponentReference> ComponentsWhichShouldChangeLocation;

	//Curve to use as interpolation for Change Location.
	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(EditCondition = "bChangeLocation == true"))
	TObjectPtr<UCurveFloat>
	ChangeLocationCurve;

	//Runtime Curve to use as interpolation for Change Location. Enabled if 
	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(EditCondition = "bChangeLocation == true && ChangeLocationCurve == nullptr" ))
	FRuntimeFloatCurve RunTimeChangeLocationCurve;

	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "bChangeLocation == true"))
	bool bChangeLocationSameForAllComponents = false;

	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "bChangeLocation == true"))
	FVector ChangeLocationRadius = FVector(100);
	
	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "bChangeLocation == true"))
	float ChangeLocationSpeed = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="LXR|Flicker")
	bool bUseEmissiveMaterial = false;

	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "bUseEmissiveMaterial == true"))
	FName EmissivePrimitiveTag = "Emissive";

	//List of primitives which has correct material for the emissive change.
	//If empty, EmissivePrimitiveTag will be used primitive components will be used.
	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "bUseEmissiveMaterial == true && EmissivePrimitiveTag.IsEmpty()"))
	TArray<TObjectPtr<UPrimitiveComponent>> EmissiveMaterialPrimitives;

	//Name of emissive material parameter.
	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "bUseEmissiveMaterial == true"))
	FName EmissiveParameterName = "Emissive";

	UPROPERTY(EditAnywhere, Category="LXR|Flicker", meta=(HideEditConditionToggle, EditCondition = "bUseEmissiveMaterial == true && EmissivePrimitiveTag.IsEmpty()"))
	FFloatRange OutValueRangeEmissive = FFloatRange(0.0f, 1.0f);

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(BlueprintReadOnly, Category="LXR|Flicker")
	float CurrentNoiseValue;

	UFUNCTION(BlueprintCallable, Category="LXR|Flicker")
	void SetFlickerState(bool NewState)
	{
		bFlickerState = NewState;
		SetComponentTickEnabled(bFlickerState);
	};

	UFUNCTION(BlueprintCallable, Category="LXR|Flicker")
	bool GetFlickerState() { return bFlickerState; };

	UFUNCTION(BlueprintCallable, Category="LXR|Flicker")
	void InitializeFlicker();

private:

	void MoveComponents();
	void MoveComponent(USceneComponent& Component);
	void ModifyMaterials();
	void ChangeLightIntensities();
	void ChangeColors();
	void FetchMyLights();
	void SetupChangeLocationComponents();
	float CalculateNoise() const;

	UPROPERTY()
	TObjectPtr<class ULXRSourceComponent> SourceComponent;

	UPROPERTY()
	TArray<TObjectPtr<ULightComponent>> MyLights;
	UPROPERTY()
	TMap<TWeakObjectPtr<ULightComponent>, float> InitialIntensities;
	UPROPERTY()
	TMap<TWeakObjectPtr<USceneComponent>, FVector> InitialLocations;

	TSet<FChangeLocationData> NewLocations;
	UPROPERTY()
	TMap<TWeakObjectPtr<USceneComponent>, FVector> CurrentLocations;
	UPROPERTY()
	TMap<TWeakObjectPtr<USceneComponent>, float> ChangeLocationPercents;


	TArray<TWeakObjectPtr<USceneComponent>> ChangeLocationComponents;

	// UPROPERTY()
	// TMap<UPrimitiveComponent*, TArray<UMaterialInstanceDynamic*>> EmissivePrimitives;

	UPROPERTY()
	TMap<TObjectPtr<UPrimitiveComponent>, FPrimitiveMaterials> EmissivePrimitivesAndMaterials;


	bool bFlickerState = true;
	bool bIsInitialized = false;

	float LastFramDistancePercent;
};
