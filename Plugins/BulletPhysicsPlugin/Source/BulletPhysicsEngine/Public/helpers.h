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

USTRUCT(BlueprintType)
struct FBulletPlayerInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	FVector MovementInput; // 0-1 on all axes, in local space (x,y,z)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	bool BoostInput; // 0-1

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int64 ObjectID; // get this from FNetworkGUID.ObjectID

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	int32 FrameNumber; // Frame number for this input; is this necessary?
};

USTRUCT(BlueprintType) // A FBulletObjectState is the instantaneous state of one object in a frame
struct FBulletObjectState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	int32 ObjectID; // is this really necessary?

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FTransform Transform;

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FVector Velocity;

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FVector AngularVelocity;

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
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
