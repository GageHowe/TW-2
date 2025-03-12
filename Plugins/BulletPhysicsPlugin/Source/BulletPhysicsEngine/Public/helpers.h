#pragma once

#include "CoreMinimal.h"
#include <functional>
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
struct FBulletPlayerInput
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector MovementInput = {0,0,0}; // 0-1 on all axes, in local space (x,y,z)

	UPROPERTY(BlueprintReadWrite)
	bool BoostInput = false; // 0-1

	UPROPERTY(BlueprintReadWrite)
	AActor* Player = nullptr;
	// is this even needed?

	UPROPERTY(BlueprintReadWrite)
	int32 FrameNumber = 0; // Frame number for this input; is this necessary?
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
	FVector Velocity;

	UPROPERTY(BlueprintReadWrite)
	FVector AngularVelocity;

	UPROPERTY(BlueprintReadWrite)
	int32 FrameNumber;
};

USTRUCT(BlueprintType) // A FBulletSimulationState is an array of all object states
struct FBulletSimulationState
{
	GENERATED_BODY()
	TArray<FBulletObjectState> ObjectStates;
	int32 FrameNumber;
};

/**
 * Converts a TMap to separate key and value arrays for RPC transmission
 * 
 * @param InMap - The map to convert
 * @param OutKeys - Array to store the keys
 * @param OutValues - Array to store the values
 */
template<typename KeyType, typename ValueType>
	static void MapToArrays(const TMap<KeyType, ValueType>& InMap, TArray<KeyType>& OutKeys, TArray<ValueType>& OutValues)
{
	const int32 ElementCount = InMap.Num();
	OutKeys.Empty(ElementCount);
	OutValues.Empty(ElementCount);
        
	for (const TPair<KeyType, ValueType>& Pair : InMap)
	{
		OutKeys.Add(Pair.Key);
		OutValues.Add(Pair.Value);
	}
}
    
/**
 * Converts separate key and value arrays back to a TMap after RPC transmission
 * 
 * @param InKeys - Array of keys
 * @param InValues - Array of values
 * @return TMap created from the arrays
 */
template<typename KeyType, typename ValueType>
static TMap<KeyType, ValueType> ArraysToMap(const TArray<KeyType>& InKeys, const TArray<ValueType>& InValues)
{
	TMap<KeyType, ValueType> Result;
        
	const int32 KeyCount = InKeys.Num();
	const int32 ValueCount = InValues.Num();
	const int32 ElementCount = FMath::Min(KeyCount, ValueCount);
        
	Result.Reserve(ElementCount);
        
	for (int32 i = 0; i < ElementCount; ++i)
	{
		Result.Add(InKeys[i], InValues[i]);
	}
        
	return Result;
}