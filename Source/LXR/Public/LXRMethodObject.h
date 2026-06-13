// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once

#include "CoreMinimal.h"
#include "ILXRSense.h"
#include "ILXRSilhouette.h"
#include "ILXRSource.h"
#include "UObject/Object.h"
#include "LXRMethodObject.generated.h"

class ULXRSourceComponent;
class ULXRLightSenseComponent;
class ULXRSilhouetteDetectionComponent;

UCLASS(BlueprintType, Blueprintable)
class LXR_API ULXRMethodObject : public UObject, public ILXRSource, public ILXRSense, public ILXRSilhouette
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="LXR|MethodObject")
	AActor* GetOwner() const;

	void SetOwner(AActor* InActor) { Owner = InActor; }

	UFUNCTION(BlueprintCallable, Category="LXR|MethodObject")
	ULXRSourceComponent* GetSourceComponent() const;

	void SetSourceComponent(ULXRSourceComponent* InSourceComponent) { SourceComponent = InSourceComponent; }

	UFUNCTION(BlueprintCallable, Category="LXR|MethodObject")
	ULXRLightSenseComponent* GetLightSenseComponent() const;

	void SetLightSenseComponent(ULXRLightSenseComponent* InLightSenseComponent) { SenseComponent = InLightSenseComponent; }

	UFUNCTION(BlueprintCallable, Category="LXR|MethodObject")
	ULXRSilhouetteDetectionComponent* GetSilhouetteComponent() const;

	void SetSilhouetteComponent(ULXRSilhouetteDetectionComponent* InSilhouetteComponent) { SilhouetteDetectionComponent = InSilhouetteComponent; }


	virtual UWorld* GetWorld() const override;

private:
	UPROPERTY()
	TObjectPtr<AActor> Owner;

	UPROPERTY()
	TObjectPtr<ULXRSourceComponent> SourceComponent;
	UPROPERTY()
	TObjectPtr<ULXRLightSenseComponent> SenseComponent;
	UPROPERTY()
	TObjectPtr<ULXRSilhouetteDetectionComponent> SilhouetteDetectionComponent;
};
