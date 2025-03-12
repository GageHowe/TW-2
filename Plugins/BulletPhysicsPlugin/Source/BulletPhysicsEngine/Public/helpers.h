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
	int32 ObjectID; // is this really necessary?

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

	void insert(FBulletObjectState obj, int num)
	{
		ObjectStates.Add(obj);
	}
};

USTRUCT(BlueprintType)
struct FBulletBroadcastPacket
{
	GENERATED_BODY()
	FBulletSimulationState SimulationState;
	
};

