#pragma once

#include "CoreMinimal.h"
#include "structs.generated.h"

// Number of redundant inputs to send (helps with packet loss)
#define MAX_STATE_CACHE 120 // Number of frames worth of state to cache (2 seconds at 60fps)
#define REDUNDANT_INPUTS 10 // Number of previous inputs to send redundantly

USTRUCT(BlueprintType)
struct BULLETPHYSICSENGINE_API FBulletPlayerInput
{
    GENERATED_BODY()

    // Input frame number (for matching with physics state)
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 FrameNumber = 0;
    
    // ID of the object this input is for
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 ObjectID = 0;
    
    // Duplicate count for compression (when multiple identical inputs are sent)
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    uint8 DuplicateCount = 1;
    
    // Movement input (normalized direction)
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FVector MovementInput = FVector::ZeroVector;
    
    // Button inputs
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    uint8 Ability : 1;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    uint8 Crouch : 1;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    uint8 RightClick : 1;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    uint8 LeftClick : 1;
    
    // Equality operator for compression
    bool operator==(const FBulletPlayerInput& Other) const
    {
        return MovementInput.Equals(Other.MovementInput, 0.01f) &&
               Ability == Other.Ability &&
               Crouch == Other.Crouch &&
               RightClick == Other.RightClick && 
               LeftClick == Other.LeftClick &&
               ObjectID == Other.ObjectID;
    }
};

USTRUCT(BlueprintType) 
struct BULLETPHYSICSENGINE_API FBulletObjectState
{
    GENERATED_BODY()
    
    // ID of this object (maps to BtRigidBodies index)
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 ObjectID = 0;
    
    // Physics state
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FTransform Transform = FTransform::Identity;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FVector Velocity = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FVector AngularVelocity = FVector::ZeroVector;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FVector Force = FVector::ZeroVector;
    
    // Check if this state differs significantly from another
    bool RequiresCorrection(const FBulletObjectState& Other) const
    {
        // Check position difference
        if (!Transform.GetLocation().Equals(Other.Transform.GetLocation(), 5.0f))
            return true;
            
        // Check rotation difference (using dot product)
        if (FMath::Abs(1.0f - Transform.GetRotation().Dot(Other.Transform.GetRotation())) > 0.05f)
            return true;
            
        // Check velocity difference
        if (!Velocity.Equals(Other.Velocity, 50.0f))
            return true;
            
        // Check angular velocity
        if (!AngularVelocity.Equals(Other.AngularVelocity, 5.0f))
            return true;
            
        return false;
    }
};

USTRUCT(BlueprintType)
struct BULLETPHYSICSENGINE_API FBulletSimulationState
{
    GENERATED_BODY()
    
    // Current frame number
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 FrameNumber = 0;
    
    // States of all objects in the simulation
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    TArray<FBulletObjectState> ObjectStates;
    
    // Helper method to get a specific object's state
    FBulletObjectState* GetObjectState(int32 ID)
    {
        for (int32 i = 0; i < ObjectStates.Num(); i++)
        {
            if (ObjectStates[i].ObjectID == ID)
            {
                return &ObjectStates[i];
            }
        }
        return nullptr;
    }
};

// Class for buffering inputs
USTRUCT()
struct BULLETPHYSICSENGINE_API FInputBuffer
{
    GENERATED_BODY()
    
    // The buffered inputs
    TArray<FBulletPlayerInput> Buffer;
    
    // Adds an input to the buffer
    void Enqueue(const FBulletPlayerInput& Input)
    {
        Buffer.Add(Input);
    }
    
    // Gets the next input without removing it
    bool Peek(FBulletPlayerInput& OutInput)
    {
        if (Buffer.Num() == 0)
        {
            return false;
        }
        
        OutInput = Buffer[0];
        return true;
    }
    
    // Gets and removes the next input
    bool Dequeue(FBulletPlayerInput& OutInput)
    {
        if (Buffer.Num() == 0)
        {
            return false;
        }
        
        OutInput = Buffer[0];
        Buffer.RemoveAt(0);
        return true;
    }
    
    // Gets the number of buffered inputs
    int32 GetCount()
    {
        return Buffer.Num();
    }
    
    // Gets all inputs without removing them, with compression
    void CompressAndGetInputs(TArray<FBulletPlayerInput>& OutInputs, int32 MaxCount)
    {
        OutInputs.Reset();
        
        if (Buffer.Num() == 0)
        {
            return;
        }
        
        // Take only up to MaxCount inputs
        int32 Count = FMath::Min(MaxCount, Buffer.Num());
        
        // Just copy without compression for now
        for (int32 i = 0; i < Count; i++)
        {
            OutInputs.Add(Buffer[i]);
        }
    }
};

// Class for caching simulation states
USTRUCT()
struct BULLETPHYSICSENGINE_API FStateCache
{
    GENERATED_BODY()
    
    // Cached states indexed by frame number
    TMap<int32, FBulletSimulationState> StateMap;
    
    // Add a state to the cache
    void AddState(const FBulletSimulationState& State)
    {
        // Add or replace existing state
        StateMap.Add(State.FrameNumber, State);
        
        // Clean up old states (keep only MAX_STATE_CACHE most recent frames)
        if (StateMap.Num() > MAX_STATE_CACHE)
        {
            int32 OldestFrame = MAX_INT32;
            
            // Find oldest frame
            for (auto& Pair : StateMap)
            {
                if (Pair.Key < OldestFrame)
                {
                    OldestFrame = Pair.Key;
                }
            }
            
            // Remove oldest
            StateMap.Remove(OldestFrame);
        }
    }
    
    // Get a state by frame number
    FBulletSimulationState* GetState(int32 FrameNumber)
    {
        return StateMap.Find(FrameNumber);
    }
    
    // Clear the cache
    void Clear()
    {
        StateMap.Empty();
    }
};