﻿#pragma once

#include "CoreMinimal.h"
#include <functional>

#include "HLSLTypeAliases.h"
#include "Containers/Queue.h"
#include "GameFramework/Actor.h"
#include "helpers.generated.h"

USTRUCT(BlueprintType)
struct Ftris
{
	GENERATED_BODY()
	FVector a;
	FVector b;
	FVector c;
	FVector d;
};

USTRUCT(BlueprintType, Blueprintable)
struct FTWPlayerInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite) // move this to int8s or bools for bandwidth
	FVector MovementInput = {0,0,0}; // 0-1 on all axes, in local space

	// this is like "intended rotation" for a free camera. this should never be overriden by the server.
	UPROPERTY(BlueprintReadWrite)
	FRotator ControlRotation = {0,0,0};

	// these are rotation diffs eg. for piloting a spaceship
	UPROPERTY()
	float TurnRight = 0;
	UPROPERTY()
	float TurnUp = 0;
	UPROPERTY()
	float RollRight = 0;

	UPROPERTY(BlueprintReadWrite)
	bool BoostInput = false; // 0-1

	UPROPERTY(BlueprintReadWrite)
	AActor* Player = nullptr;
};

USTRUCT(BlueprintType) // A FBulletObjectState is the instantaneous state of one object in a frame
struct FBulletObjectState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite)
	AActor* Actor = nullptr;

	UPROPERTY(BlueprintReadWrite)
	FTransform Transform;

	UPROPERTY(BlueprintReadWrite)
	FVector Velocity = {0,0,0};

	UPROPERTY(BlueprintReadWrite)
	FVector AngularVelocity;

	// used for getting state error
	// Update your operator- to be const-correct
	FBulletObjectState operator-(const FBulletObjectState& other) const
	{
		FBulletObjectState result;
		result.Actor = Actor;
		result.Transform.SetLocation(Transform.GetLocation() - other.Transform.GetLocation());
		result.Transform.SetRotation(Transform.GetRotation() - other.Transform.GetRotation());
		result.Velocity = Velocity - other.Velocity;
		result.AngularVelocity = AngularVelocity - other.AngularVelocity;

		return result;
	}
	
	// FBulletObjectState operator+(const FBulletObjectState& other)
	// {
	// 	FBulletObjectState result;
	// 	result.Actor = Actor;
	// 	result.Transform.SetLocation(Transform.GetLocation() + other.Transform.GetLocation());
	// 	result.Transform.SetRotation(Transform.GetRotation() + other.Transform.GetRotation());
	// 	result.Velocity = Velocity + other.Velocity;
	// 	result.AngularVelocity = AngularVelocity + other.AngularVelocity;
	//
	// 	return result;
	// }
};

USTRUCT(BlueprintType) // A FBulletSimulationState is an array of all object states
struct FBulletSimulationState
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<FBulletObjectState> ObjectStates = TArray<FBulletObjectState>();

	UPROPERTY()
	double CurrentTime = 0;
};

static FBulletObjectState InterpolateObjectStates(const FBulletObjectState& a, const FBulletObjectState& b, float alpha)
{
	FBulletObjectState result;
	result.Actor = a.Actor; // Preserve actor reference
	result.Transform.SetLocation(FMath::Lerp(a.Transform.GetLocation(), b.Transform.GetLocation(), alpha));
	result.Transform.SetRotation(FQuat::Slerp(a.Transform.GetRotation(), b.Transform.GetRotation(), alpha));
	result.Velocity = FMath::Lerp(a.Velocity, b.Velocity, alpha);
	result.AngularVelocity = FMath::Lerp(a.AngularVelocity, b.AngularVelocity, alpha);
	return result;
}

inline FBulletSimulationState InterpolateSimState(FBulletSimulationState& a, FBulletSimulationState& b, float alpha)
{
	FBulletSimulationState result;
    
	for (FBulletObjectState& stateA : a.ObjectStates)
	{
		FBulletObjectState* stateB = b.ObjectStates.FindByPredicate([&stateA](const FBulletObjectState& state) 
		{ 
			return state.Actor == stateA.Actor; 
		});
        
		if (stateB)
		{
			FBulletObjectState interpolatedState = InterpolateObjectStates(stateA, *stateB, alpha);
			result.ObjectStates.Add(interpolatedState);
		}
		else { result.ObjectStates.Add(stateA); }
	}
    
	return result;
}

// Convert TMap to a pair of arrays
template<typename KeyType, typename ValueType>
TPair<TArray<KeyType>, TArray<ValueType>> TMapToArrays(const TMap<KeyType, ValueType>& Map)
{
	TArray<KeyType> OutKeys;
	TArray<ValueType> OutValues;
    
	for (const TPair<KeyType, ValueType>& Pair : Map)
	{
		OutKeys.Add(Pair.Key);
		OutValues.Add(Pair.Value);
	}
    
	return TPair<TArray<KeyType>, TArray<ValueType>>(OutKeys, OutValues);
}

// Convert a pair of arrays back to TMap
template<typename KeyType, typename ValueType>
TMap<KeyType, ValueType> ArraysToTMap(const TArray<KeyType>& InKeys, const TArray<ValueType>& InValues)
{
	TMap<KeyType, ValueType> OutMap;
    
	const int32 Count = FMath::Min(InKeys.Num(), InValues.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		OutMap.Add(InKeys[i], InValues[i]);
	}
    
	return OutMap;
}

