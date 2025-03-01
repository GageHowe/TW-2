#pragma once

#include "structs.generated.h"

USTRUCT(BlueprintType)
struct BULLETPHYSICSENGINE_API FBulletPlayerInput
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	FVector MovementInput; // 0-1 on all axes, in local space (x,y,z)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	bool Ability;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	bool Crouch;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	bool RightClick;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	bool LeftClick;
	
	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	int32 FrameNumber; // Frame number for this input
};

USTRUCT(BlueprintType) // A FBulletObjectState is the instantaneous state of one object in a frame
struct BULLETPHYSICSENGINE_API FBulletObjectState
{
	GENERATED_BODY()

	// depricated, simply insert objectstate to appropriate index of serverstate
	// UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	// int32 ID;
	
	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FTransform Transform;

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FVector Velocity;

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FVector AngularVelocity;

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FVector Force;
	
};

USTRUCT(BlueprintType) // A FBulletSimulationState is an array of all object states
struct BULLETPHYSICSENGINE_API FBulletSimulationState
{
	GENERATED_BODY()
	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	TArray<FBulletObjectState> ObjectStates;
	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	int32 FrameNumber;

	void insert(FBulletObjectState obj, int num)
	{
		ObjectStates.Add(obj);
	}
};