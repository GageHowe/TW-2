#pragma once

#include "structs.generated.h"

// Constants
#define PHYSICS_TICK_RATE 60
#define FIXED_DELTA_TIME (1.0f / PHYSICS_TICK_RATE)
#define INPUT_BUFFER_SIZE 128
#define STATE_CACHE_SIZE 128
#define REDUNDANT_INPUTS 3 // Send each input multiple times to handle packet loss

USTRUCT(BlueprintType)
struct BULLETPHYSICSENGINE_API FBulletPlayerInput
{
    GENERATED_BODY()

    // Basic movement inputs
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
    FVector MovementInput; // Local space (x,y,z)
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
    bool Ability;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
    bool Crouch;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
    bool RightClick;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
    bool LeftClick;
    
    // Frame number and object ID for prediction
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 FrameNumber = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 ObjectID = -1;
    
    // The number of times this input is duplicated (for compression)
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    uint8 DuplicateCount = 0;
    
    // Equality operator for comparing inputs (used for input compression)
    bool operator==(const FBulletPlayerInput& Other) const
    {
        return MovementInput.Equals(Other.MovementInput, 0.01f) && 
               Ability == Other.Ability && 
               Crouch == Other.Crouch && 
               RightClick == Other.RightClick && 
               LeftClick == Other.LeftClick &&
               ObjectID == Other.ObjectID;
    }
    
    bool operator!=(const FBulletPlayerInput& Other) const
    {
        return !(*this == Other);
    }
};

USTRUCT(BlueprintType) // Object state for one frame
struct BULLETPHYSICSENGINE_API FBulletObjectState
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 ObjectID = -1;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FTransform Transform;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FVector Velocity;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FVector AngularVelocity;

    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    FVector Force;
    
    // Calculate how much this state differs from another state
    float GetStateDifference(const FBulletObjectState& OtherState) const
    {
        float PositionDiff = FVector::Distance(Transform.GetLocation(), OtherState.Transform.GetLocation());
        float RotationDiff = Transform.GetRotation().AngularDistance(OtherState.Transform.GetRotation());
        float VelocityDiff = FVector::Distance(Velocity, OtherState.Velocity);
        float AngularVelocityDiff = FVector::Distance(AngularVelocity, OtherState.AngularVelocity);
        
        return PositionDiff + RotationDiff + VelocityDiff * 0.2f + AngularVelocityDiff * 0.1f;
    }
    
    bool RequiresCorrection(const FBulletObjectState& ServerState) const
    {
        const float PositionThreshold = 0.5f; // 0.5 units
        const float RotationThreshold = 0.05f; // About 3 degrees
        const float VelocityThreshold = 1.0f; // 1 unit/s
        
        float PositionDiff = FVector::Distance(Transform.GetLocation(), ServerState.Transform.GetLocation());
        float RotationDiff = Transform.GetRotation().AngularDistance(ServerState.Transform.GetRotation());
        float VelocityDiff = FVector::Distance(Velocity, ServerState.Velocity);
        
        return PositionDiff > PositionThreshold || 
               RotationDiff > RotationThreshold || 
               VelocityDiff > VelocityThreshold;
    }
};

USTRUCT(BlueprintType) // A simulation state is an array of all object states
struct BULLETPHYSICSENGINE_API FBulletSimulationState
{
    GENERATED_BODY()
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    TArray<FBulletObjectState> ObjectStates;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    int32 FrameNumber = 0;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    TArray<FBulletPlayerInput> PendingInputs;
    
    UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    TArray<int32> ActiveObjectIDs;
    
    // Get state for a specific object
    FBulletObjectState* GetObjectState(int32 ObjectID)
    {
        for (FBulletObjectState& State : ObjectStates)
        {
            if (State.ObjectID == ObjectID)
            {
                return &State;
            }
        }
        return nullptr;
    }
    
    // Add or update an object state
    void UpdateObjectState(const FBulletObjectState& ObjectState)
    {
        for (int32 i = 0; i < ObjectStates.Num(); i++)
        {
            if (ObjectStates[i].ObjectID == ObjectState.ObjectID)
            {
                ObjectStates[i] = ObjectState;
                return;
            }
        }
        
        // Not found, add new
        ObjectStates.Add(ObjectState);
    }
    
    // Compatibility with original code
    void insert(const FBulletObjectState& obj, int num)
    {
        FBulletObjectState NewObj = obj;
        NewObj.ObjectID = num;
        ObjectStates.Add(NewObj);
    }
};



// Cache of historical states for reconciliation
USTRUCT(BlueprintType)
struct BULLETPHYSICSENGINE_API FStateCache
{
    GENERATED_BODY()
    
    // Array of simulation states (circular buffer)
    UPROPERTY()
    TArray<FBulletSimulationState> States;
    
    // Constructor initializes the buffer size
    FStateCache()
    {
        States.SetNum(STATE_CACHE_SIZE);
    }
    
    // Add a new state to the cache
    void AddState(const FBulletSimulationState& State)
    {
        States[State.FrameNumber % STATE_CACHE_SIZE] = State;
    }
    
    // Get a state by frame number
    FBulletSimulationState* GetState(int32 FrameNumber)
    {
        // Check if frame is within valid range
        if (FrameNumber < 0 || FrameNumber >= STATE_CACHE_SIZE)
        {
            return nullptr;
        }
        
        FBulletSimulationState& State = States[FrameNumber % STATE_CACHE_SIZE];
        if (State.FrameNumber != FrameNumber)
        {
            return nullptr; // No valid state stored
        }
        
        return &State;
    }
    
    // Get an object state by frame and object ID
    FBulletObjectState* GetObjectState(int32 FrameNumber, int32 ObjectID)
    {
        FBulletSimulationState* SimState = GetState(FrameNumber);
        if (!SimState)
        {
            return nullptr;
        }
        
        return SimState->GetObjectState(ObjectID);
    }
};