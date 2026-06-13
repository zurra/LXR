// Copyright (c) 2026 Hannes Väisänen
// Licensed under the LXR Community License.
// See LICENSE file for details.

#pragma once
#include "Tasks/GenerateSenseTargetsAsyncTask.h"

#include "LXRSubsystem.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "DrawDebugHelpers.h"
#include "LXRFunctionLibrary.h"


void FGenerateSenseTargetsAsyncTask::DoWork()
{
	Data->StartState();
	Data->TracePoints.Reset();
	GenerateSenseTargets();
	FilterGeneratedTargets();
	Data->TargetIterator = Data->TracePoints.Num() - 1;
	Data->NextState(); // process

}

inline float GetComponentTickInterval(const ULXRLightSenseComponent* SenseComponent)
{
	return SenseComponent->GetComponentTickInterval();
}

void FGenerateSenseTargetsAsyncTask::GenerateSenseTargets()
{
	TArray<FVector> CylinderEdges;
	FVector Start = Data->SenseLocation;
	FRotator Rotator = Data->SenseRotation;
	FVector RotatedDirection;

	float ConeAngle = Data->SenseComponent->SenseTraceTaskParams.ConeAngle;
	double SenseDistance = Data->SenseComponent->SenseTraceTaskParams.SenseDistance;
	float ConeTraces = Data->SenseComponent->SenseTraceTaskParams.ConeTraces;
	float DistancePerSegment = Data->SenseComponent->SenseTraceTaskParams.DistancePerSegment;
	float MinDistancePerTarget = Data->SenseComponent->SenseTraceTaskParams.MinDistancePerTarget;
	bool bDrawDebug = Data->SenseComponent->DebugOptions.bDebugTargets;
	RotatedDirection = UKismetMathLibrary::RotateAngleAxis(FVector::ForwardVector, ConeAngle, FVector::LeftVector);

	FVector A = RotatedDirection * SenseDistance;
	FVector B = FVector::ForwardVector * SenseDistance;
	FVector P = UKismetMathLibrary::ProjectVectorOnToVector(A, B);

	double CapsuleRadius = (P - A).Length();
	// Calculate the rotation matrix based on the direction
	UE::Math::TVector<double> Direction = Rotator.Vector();
	UE::Math::TMatrix<double> RotationMatrix = FRotationMatrix::MakeFromX(FVector::RightVector);

	// Calculate the axis vector of the cylinder
	FVector AxisVector = FVector::ForwardVector;
	for (int32 SideIndex = 0; SideIndex < ConeTraces; ++SideIndex)
	{
		// Calculate the angle for the current side
		double Theta = 360.0f * (static_cast<float>(SideIndex) / static_cast<float>(ConeTraces));

		// Convert angle to radians
		double ThetaRad = FMath::DegreesToRadians(Theta + GetWorld()->RealTimeSeconds * 150);
		// float ThetaRad = FMath::DegreesToRadians(Theta);

		// Calculate the position on the cylinder
		FVector PointOnCylinder = B + RotationMatrix.TransformVector(AxisVector * CapsuleRadius * FMath::Cos(ThetaRad))
			+ RotationMatrix.GetUnitAxis(EAxis::Z) * CapsuleRadius * FMath::Sin(ThetaRad);

		// Do something with the PointOnCylinder, such as spawning a mesh or drawing a debug point
		// ...
		CylinderEdges.AddUnique(PointOnCylinder);
		// if (bDrawDebug)
		// {
		// 	DrawDebugPoint(GetWorld(), PointOnCylinder, 10.0f, FColor::Red, false, 1.0f);
		// }
	}

	if (bDrawDebug)
	{
		TSharedPtr<FSenseTaskData> DebugDataCopy = MakeShared<FSenseTaskData>(*Data);
		ULXRFunctionLibrary::DebugOnGameThread([TaskData = DebugDataCopy, CylinderEdges, Start]
		{
			for (auto Edge : CylinderEdges)
			{
				FVector C = TaskData->SenseRotation.RotateVector(Edge.GetSafeNormal());
				DrawDebugLine(TaskData->SenseComponent->GetWorld(), Start, Start + C * TaskData->SenseComponent->SenseTraceTaskParams.SenseDistance, FColor::Cyan, false, 0.2f, 0,
				              2);
			}
		});
	}
	int Segments = FMath::RoundToInt(static_cast<float>(SenseDistance) / DistancePerSegment);
	int Totals = CylinderEdges.Num()*Segments;
	int Divisor = SenseDistance/Totals;
	int TraceInt = -1;
	for (FVector& EdgeEnd : CylinderEdges)
	{
		// if (bDrawDebug)
		// {
		// 	DrawDebugLine(GetWorld(), Start, Start + RotatedDirection * SenseDistance, FColor::Emerald, false, GetComponentTickInterval(Data->SenseComponent), 0, 1);
		// }
		for (int i = 1; i < Segments; ++i)
		{
			// if (FMath::RandBool())
			// 	continue;
			for (int Count = 0; Count <= 1; ++Count)
			{
				float Mpl = FMath::RandRange(150,250);
				float IteratorDistance = i * DistancePerSegment;
				FVector CylinderEdgeEnd = EdgeEnd.GetSafeNormal() * IteratorDistance;
				FVector Center = UKismetMathLibrary::ProjectVectorOnToVector(CylinderEdgeEnd, B);
				FVector DirectionToUse = EdgeEnd.GetSafeNormal();
				if (Count == 1)
				{
					TraceInt++;
					// Mpl = 250;
					IteratorDistance = i * DistancePerSegment;
					DirectionToUse = FMath::LerpStable(FVector::ForwardVector, EdgeEnd.GetSafeNormal(), 0.5f);
				}

				FVector DirectionToCenter = (Center - CylinderEdgeEnd);
				float DistanceToCenter = DirectionToCenter.Length();
				DirectionToCenter = DirectionToCenter.GetSafeNormal();

				float SinMod = (FMath::Sin(static_cast<float>(IteratorDistance * 5 + GetWorld()->GetRealTimeSeconds() * 2)) + 1) * 0.5;
				float CosMod = (FMath::Cos(GetWorld()->GetRealTimeSeconds()*1)+1)*0.5f;
				double DistanceToAdd = FMath::Wrap<float>(IteratorDistance + GetWorld()->GetRealTimeSeconds() *Mpl , 100.f, SenseDistance);

				
				// FVector TracePoint = DirectionToUse.GetSafeNormal() * FMath::Wrap<float>((Divisor*TraceInt), 100.f, SenseDistance);;
				FVector TracePoint = DirectionToUse.GetSafeNormal() * DistanceToAdd;
				FVector TracePoint2 = EdgeEnd.GetSafeNormal() * DistanceToAdd;
				// auto CurrentDistance = TracePoint.Size();
				auto CurrentDistancePercentFromSenseDistance = DistanceToAdd / SenseDistance;

				// bool UseTracePoint = UKismetMathLibrary::RandomBoolWithWeight(FMath::Max(0.1f, CurrentDistancePercentFromSenseDistance * 1));
				TracePoint = TracePoint + DirectionToCenter * (SinMod * (DistanceToCenter * CurrentDistancePercentFromSenseDistance));
				// FVector TracePoint2 = TracePoint+RotatedDirection*-CapsuleRadius;
				// TracePoint = TracePoint+(DirectionToUse).GetSafeNormal()*;
				// TracePoint = TracePoint+Start;
				TracePoint = Rotator.RotateVector(TracePoint) + Start;
				TracePoint2 = Rotator.RotateVector(TracePoint2) + Start;

				bool AddNewTracePoint = true;

				for (int j = 0; j < Data->TracePoints.Num(); ++j)
				{
					if ((Data->TracePoints[j] - TracePoint).Size() < MinDistancePerTarget)
					{
						AddNewTracePoint = false;
						break;
					}
				}

				if (AddNewTracePoint)
					Data->TracePoints.Add(TracePoint);

				Data->TracePoints.Add(TracePoint2);

				if (bDrawDebug)
				{
					TSharedPtr<FSenseTaskData> DebugDataCopy = MakeShared<FSenseTaskData>(*Data);
					ULXRFunctionLibrary::DebugOnGameThread([TaskData = DebugDataCopy, CylinderEdges, Start, TracePoint, AddNewTracePoint, Count, TracePoint2]
					{
						DrawDebugCrosshairs(TaskData->SenseComponent->GetWorld(), TracePoint,FRotator::ZeroRotator, 10.f, FColor::Green, false, 0.25);

						DrawDebugPoint(TaskData->SenseComponent->GetWorld(), TracePoint, 5.f, AddNewTracePoint ? (Count == 0 ? FColor::Green : FColor::Purple) : FColor::Red, false,
						               0.2f);
						DrawDebugPoint(TaskData->SenseComponent->GetWorld(), TracePoint2, 5.f, FColor::Green, false, 0.2f);
					});
				}
			}
		}
	}
}

void FGenerateSenseTargetsAsyncTask::FilterGeneratedTargets()
{
	const AActor* Owner = Data->SenseComponent->GetOwner();
	auto Params = FCollisionQueryParams(Owner->GetFName(), SCENE_QUERY_STAT_ONLY(KismetTraceUtils), false);
	Params.AddIgnoredActor(Owner);

	TArray<FVector> RemoveTargets;
	for (const FVector& End : Data->TracePoints)
	{
		if (GetWorld()->LineTraceTestByChannel(Owner->GetActorLocation(), End, Data->SenseComponent->TraceChannel, Params))
		{
			RemoveTargets.Add(End);
			// DrawDebugBox(GetWorld(), End, FVector(5), FColor::Red, false, 0.1f, 0, 1);
		}
		// else
		// {
		// 	DrawDebugBox(GetWorld(), End, FVector(10), FColor::Green, false, 0.1f, 0, 1);
		// }
	}
	for (FVector& RemoveTarget : RemoveTargets)
	{
		Data->TracePoints.RemoveSwap(RemoveTarget);
	}
}
