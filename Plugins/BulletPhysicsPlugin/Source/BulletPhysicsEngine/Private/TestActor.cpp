// Fill out your copyright notice in the Description page of Project Settings.

#include "TestActor.h"

#include "SimpleBallPawn.h"
#include "Types/AttributeStorage.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"

#include "Types/AttributeStorage.h"


// Sets default values
ATestActor::ATestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Make this actor replicate
	bReplicates = true;
	bAlwaysRelevant = true;
	
	// For deterministic physics
	PhysicsAccumulator = 0.0f;
}

// Called when the game starts or when spawned
void ATestActor::BeginPlay()
{
	Super::BeginPlay();

	// Setup Bullet Physics
	BtCollisionConfig = new btDefaultCollisionConfiguration();
	BtCollisionDispatcher = new btCollisionDispatcher(BtCollisionConfig);
	BtBroadphase = new btDbvtBroadphase();
	mt = new btSequentialImpulseConstraintSolver;
	
	// Set fixed random seed for deterministic physics
	mt->setRandSeed(12345);
	BtConstraintSolver = mt;
	
	BtWorld = new btDiscreteDynamicsWorld(BtCollisionDispatcher, BtBroadphase, BtConstraintSolver, BtCollisionConfig);
	BtWorld->setGravity(btVector3(0, 0, -980.0)); // -9.8 m/s^2 in Unreal units
	
	// Disable deactivation for deterministic physics
	BtWorld->getDispatchInfo().m_allowedCcdPenetration = 0.0001f;
	BtWorld->getSolverInfo().m_numIterations = 10;
	BtWorld->getSolverInfo().m_solverMode = SOLVER_SIMD | SOLVER_USE_WARMSTARTING;
	
	// Initialize state tracking
	CurrentFrameNumber = 0;
}

// Called every frame
void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	// Fixed timestep physics simulation
	PhysicsAccumulator += DeltaTime;
	
	// Run as many fixed physics steps as needed to catch up
	while (PhysicsAccumulator >= FixedDeltaTime)
	{
		// Process inputs from all clients
		if (HasAuthority()) // Server-only
		{
			for (auto& Pair : ClientInputBuffers)
			{
				FBulletPlayerInput Input;
				// Process one input per physics step
				if (Pair.Value.Peek(Input))
				{
					// Only process if this is the correct frame or newer
					if (Input.FrameNumber <= CurrentFrameNumber)
					{
						Pair.Value.Dequeue(Input);
						ApplyInput(Input);
					}
				}
			}
		}
		
		// Step simulation
		StepPhysics(FixedDeltaTime, 0);
		PhysicsAccumulator -= FixedDeltaTime;
		CurrentFrameNumber++;
		
		// Record state for reconciliation
		RecordState();
		
		// Server sends state updates to clients
		if (HasAuthority() && (CurrentFrameNumber % 2 == 0)) // Send every other frame to save bandwidth
		{
			MC_SendStateToClients(GetSerializableState());
		}
	}
}

void ATestActor::RecordState()
{
	FBulletSimulationState State;
	State.FrameNumber = CurrentFrameNumber;
	
	// Record all dynamic objects
	for (int32 i = 0; i < BtRigidBodies.Num(); i++)
	{
		btRigidBody* Body = BtRigidBodies[i];
		if (Body)
		{
			FBulletObjectState ObjectState;
			ObjectState.ObjectID = i;
			ObjectState.Transform = BulletHelpers::ToUE(Body->getWorldTransform(), GetActorLocation());
			ObjectState.Velocity = BulletHelpers::ToUEDir(Body->getLinearVelocity(), false);
			ObjectState.AngularVelocity = BulletHelpers::ToUEDir(Body->getAngularVelocity(), false);
			ObjectState.Force = BulletHelpers::ToUEDir(Body->getTotalForce(), false);
			
			State.ObjectStates.Add(ObjectState);
		}
	}
	
	// Store in cache
	StateCache.AddState(State);
}

FBulletSimulationState ATestActor::GetCurrentState()
{
    FBulletSimulationState State;
    State.FrameNumber = this->CurrentFrameNumber;
    
    // Capture all rigid body states
    BtCriticalSection.Lock();
    for (int32 i = 0; i < this->BtRigidBodies.Num(); i++)
    {
        btRigidBody* Body = this->BtRigidBodies[i];
        if (Body)
        {
            FBulletObjectState ObjectState;
            ObjectState.ObjectID = i;
            ObjectState.Transform = BulletHelpers::ToUE(Body->getWorldTransform(), this->GetActorLocation());
            ObjectState.Velocity = BulletHelpers::ToUEDir(Body->getLinearVelocity(), false);
            ObjectState.AngularVelocity = BulletHelpers::ToUEDir(Body->getAngularVelocity(), false);
            ObjectState.Force = BulletHelpers::ToUEDir(Body->getTotalForce(), false);
            
            State.ObjectStates.Add(ObjectState);
        }
    }
    BtCriticalSection.Unlock();
    
    return State;
}

FBulletSimulationState ATestActor::GetSerializableState()
{
    // Create a smaller, network-optimized version of the state
    FBulletSimulationState State = GetCurrentState();
    
    // Filter out inactive objects to save bandwidth
    State.ObjectStates.RemoveAll([](const FBulletObjectState& ObjectState) {
        // Only include objects that moved significantly
        return ObjectState.Velocity.SizeSquared() < 0.1f && 
               ObjectState.AngularVelocity.SizeSquared() < 0.1f;
    });
    
    return State;
}

void ATestActor::ProcessClientInput(const FBulletPlayerInput& Input)
{
    // Only process inputs for this client's controlled objects
    if (!HasAuthority()) // Client-side prediction
    {
        // Only process if we own this object
        // Use the client-to-server ID mapping
        MapCriticalSection.Lock();
        int32* ServerID = ClientIdToServerId.Find(Input.ObjectID);
        MapCriticalSection.Unlock();
        
        if (ServerID != nullptr)
        {
            // Apply the input immediately
            FBulletPlayerInput AdjustedInput = Input;
            AdjustedInput.ObjectID = *ServerID;
            ApplyInput(AdjustedInput);
        }
    }
    else // Server authority
    {
        // For server, we use the input as-is
        ApplyInput(Input);
    }
}

void ATestActor::ApplyInput(const FBulletPlayerInput& Input)
{
	// Check if the object ID is valid
	if (!BtRigidBodies.IsValidIndex(Input.ObjectID) || !BtRigidBodies[Input.ObjectID])
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyInput: Invalid object ID %d (BtRigidBodies size: %d)"), 
			Input.ObjectID, BtRigidBodies.Num());
		return;
	}
    
	btRigidBody* Body = BtRigidBodies[Input.ObjectID];
	AActor* ControlledActor = static_cast<AActor*>(Body->getUserPointer());
    
	// Handle ball-specific input
	ASimpleBallPawn* Ball = Cast<ASimpleBallPawn>(ControlledActor);
	if (Ball)
	{
		// Apply movement with appropriate force
		if (!Input.MovementInput.IsNearlyZero())
		{
			FVector WorldForce = Input.MovementInput * Ball->RollForce;
			Body->applyCentralForce(BulletHelpers::ToBtDir(WorldForce, false));
		}
        
		// Apply jump
		if (Input.Ability && Ball->bIsOnGround)
		{
			Body->applyCentralImpulse(btVector3(0, 0, Ball->JumpForce));
		}
	}
	else
	{
		// Default handling for other pawn types
		if (!Input.MovementInput.IsNearlyZero())
		{
			// Convert local input to world space
			FTransform BodyTransform = BulletHelpers::ToUE(Body->getWorldTransform(), GetActorLocation());
			FVector WorldForce = BodyTransform.TransformVector(Input.MovementInput * 1000.0f);
            
			Body->applyCentralForce(BulletHelpers::ToBtDir(WorldForce, false));
		}
        
		// Apply ability input
		if (Input.Ability)
		{
			Body->applyCentralImpulse(btVector3(0, 0, 500.0f));
		}
	}
    
	// If actor implements the controllable interface, pass input to it
	if (ControlledActor && ControlledActor->GetClass()->ImplementsInterface(UControllableInterface::StaticClass()))
	{
		IControllableInterface::Execute_OnPlayerInput(ControlledActor, Input);
	}
}

void ATestActor::EnqueueInput(const FBulletPlayerInput& Input, int32 ClientID)
{
	// Get or create input buffer for this client
	FInputBuffer& Buffer = ClientInputBuffers.FindOrAdd(ClientID);
    
	// Log what we're enqueueing
	UE_LOG(LogTemp, Warning, TEXT("SERVER: Enqueueing input for client %d: movement=(%f,%f), frame=%d"), 
		  ClientID, Input.MovementInput.X, Input.MovementInput.Y, Input.FrameNumber);
    
	// Add input to buffer
	Buffer.Enqueue(Input);
}

void ATestActor::Server_SendInput_Implementation(const TArray<FBulletPlayerInput>& Inputs)
{
    // Process all inputs from a client
    for (const FBulletPlayerInput& Input : Inputs)
    {
        EnqueueInput(Input, Input.ObjectID);
    }
}

void ATestActor::MC_SendStateToClients_Implementation(FBulletSimulationState ServerState)
{
    // On server, just record that we sent this state
    if (HasAuthority())
    {
        return;
    }
    
    // On client, handle state reconciliation
    ReconcileState(ServerState);
}

void ATestActor::ReconcileState(const FBulletSimulationState& ServerState)
{
    // Skip reconciliation if we're already reconciling
    if (bIsReconciling || ServerState.FrameNumber <= LastReconcileFrame)
    {
        return;
    }
    
    // Find our cached state for this frame
    FBulletSimulationState* CachedState = StateCache.GetState(ServerState.FrameNumber);
    if (!CachedState)
    {
        // State is too old, just apply directly
        for (const FBulletObjectState& ServerObjectState : ServerState.ObjectStates)
        {
            if (BtRigidBodies.IsValidIndex(ServerObjectState.ObjectID) && BtRigidBodies[ServerObjectState.ObjectID])
            {
                btRigidBody* Body = BtRigidBodies[ServerObjectState.ObjectID];
                
                Body->setWorldTransform(BulletHelpers::ToBt(ServerObjectState.Transform, GetActorLocation()));
                Body->setLinearVelocity(BulletHelpers::ToBtDir(ServerObjectState.Velocity, false));
                Body->setAngularVelocity(BulletHelpers::ToBtDir(ServerObjectState.AngularVelocity, false));
            }
        }
        return;
    }
    
    // Check for significant differences that require reconciliation
    bool NeedsReconciliation = false;
    for (const FBulletObjectState& ServerObjectState : ServerState.ObjectStates)
    {
        FBulletObjectState* CachedObjectState = CachedState->GetObjectState(ServerObjectState.ObjectID);
        if (CachedObjectState && CachedObjectState->RequiresCorrection(ServerObjectState))
        {
            NeedsReconciliation = true;
            break;
        }
    }
    
    if (!NeedsReconciliation)
    {
        return;
    }
    
    UE_LOG(LogTemp, Warning, TEXT("Reconciling physics from frame %d to %d"), 
        ServerState.FrameNumber, CurrentFrameNumber);
    
    // Begin reconciliation process
    bIsReconciling = true;
    
    // Save current state for restoration
    FBulletSimulationState CurrentClientState = GetCurrentState();
    
    // 1. Set state to match server's authoritative state
    for (const FBulletObjectState& ServerObjectState : ServerState.ObjectStates)
    {
        if (BtRigidBodies.IsValidIndex(ServerObjectState.ObjectID) && BtRigidBodies[ServerObjectState.ObjectID])
        {
            btRigidBody* Body = BtRigidBodies[ServerObjectState.ObjectID];
            
            Body->setWorldTransform(BulletHelpers::ToBt(ServerObjectState.Transform, GetActorLocation()));
            Body->setLinearVelocity(BulletHelpers::ToBtDir(ServerObjectState.Velocity, false));
            Body->setAngularVelocity(BulletHelpers::ToBtDir(ServerObjectState.AngularVelocity, false));
        }
    }
    
    // 2. Replay inputs from this frame up to current
    int32 ReplayStartFrame = ServerState.FrameNumber + 1;
    for (int32 Frame = ReplayStartFrame; Frame <= CurrentFrameNumber; Frame++)
    {
        // Apply all cached inputs for this frame
        for (auto& Pair : ClientInputBuffers)
        {
            FBulletPlayerInput Input;
            if (Pair.Value.Peek(Input) && Input.FrameNumber == Frame)
            {
                ApplyInput(Input);
                Pair.Value.Dequeue(Input);
            }
        }
        
        // Step physics
        StepPhysics(FixedDeltaTime, 0);
        
        // Record reconciled state
        RecordState();
    }
    
    // Reconciliation complete
    bIsReconciling = false;
    LastReconcileFrame = ServerState.FrameNumber;
}

TArray<FBulletPlayerInput> ATestActor::CompressInputs(const TArray<FBulletPlayerInput>& Inputs)
{
    TArray<FBulletPlayerInput> CompressedInputs;
    
    if (Inputs.Num() == 0)
    {
        return CompressedInputs;
    }
    
    FBulletPlayerInput CurrentInput = Inputs[0];
    CurrentInput.DuplicateCount = 1;
    
    for (int32 i = 1; i < Inputs.Num(); i++)
    {
        const FBulletPlayerInput& Input = Inputs[i];
        
        // If input is the same, increment duplicate count
        if (Input == CurrentInput && Input.FrameNumber == CurrentInput.FrameNumber + CurrentInput.DuplicateCount)
        {
            CurrentInput.DuplicateCount++;
        }
        else
        {
            // Add current input and start new one
            CompressedInputs.Add(CurrentInput);
            CurrentInput = Input;
            CurrentInput.DuplicateCount = 1;
        }
    }
    
    // Add the last processed input
    CompressedInputs.Add(CurrentInput);
    
    return CompressedInputs;
}

TArray<FBulletPlayerInput> ATestActor::DecompressInputs(const TArray<FBulletPlayerInput>& CompressedInputs)
{
    TArray<FBulletPlayerInput> DecompressedInputs;
    
    for (const FBulletPlayerInput& Input : CompressedInputs)
    {
        // Add the original input
        FBulletPlayerInput BaseInput = Input;
        BaseInput.DuplicateCount = 1;
        
        // Decompress by repeating for the duplicate count
        for (uint8 i = 0; i < Input.DuplicateCount; i++)
        {
            FBulletPlayerInput DuplicateInput = BaseInput;
            DuplicateInput.FrameNumber = Input.FrameNumber + i;
            DecompressedInputs.Add(DuplicateInput);
        }
    }
    
    return DecompressedInputs;
}

TArray<FBulletPlayerInput> ATestActor::GetBufferedInputs(int32 ClientID, int32 MaxCount)
{
    TArray<FBulletPlayerInput> Result;
    
    FInputBuffer* Buffer = ClientInputBuffers.Find(ClientID);
    if (Buffer)
    {
        Buffer->CompressAndGetInputs(Result, MaxCount);
    }
    
    return Result;
}

int32 ATestActor::GetPingInFrames()
{
    // Estimate ping based on a default value since PlayerState->GetPingInMilliseconds() is not readily available
    // In a real implementation, you could use a network subsystem to get actual ping values
    
    if (GetWorld() && GetWorld()->GetGameState())
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC && PC->PlayerState)
        {
            // Most UE projects have a GetPing() method that returns ping in ms
            // Since we can't access it directly, we'll estimate
            
            // Assuming 60Hz physics and a typical ping of 100ms
            return FMath::CeilToInt(100.0f / (FixedDeltaTime * 1000.0f));
        }
    }
    
    // Default to 5 frames (about 83ms at 60fps)
    return 5;
}

void ATestActor::AddImpulse(int ID, FVector Impulse, FVector Location)
{
	BtRigidBodies[ID]->applyImpulse(BulletHelpers::ToBtDir(Impulse, false), BulletHelpers::ToBtPos(Location, GetActorLocation()));
}

// BEGIN NEW METHODS ####################################################
void ATestActor::AddForce(int ID, FVector Force, FVector Location)
{
	BtRigidBodies[ID]->applyForce(BulletHelpers::ToBtDir(Force, false), BulletHelpers::ToBtPos(Location, GetActorLocation()));
}
void ATestActor::AddCentralForce(int ID, FVector Force)
{
	BtRigidBodies[ID]->applyCentralForce(BulletHelpers::ToBtDir(Force, false));
}
void ATestActor::AddTorque(int ID, FVector Torque)
{
	BtRigidBodies[ID]->applyTorque(BulletHelpers::ToBtDir(Torque, false));
}
void ATestActor::AddTorqueImpulse(int ID, FVector Torque)
{
	BtRigidBodies[ID]->applyTorqueImpulse(BulletHelpers::ToBtDir(Torque, false));
}
void ATestActor::GetVelocityAtLocation(int ID, FVector Location, FVector&Velocity)
{
	if (BtRigidBodies[ID]) {
		Velocity = BulletHelpers::ToUEPos(BtRigidBodies[ID]->getVelocityInLocalPoint(BulletHelpers::ToBtPos(Location, GetActorLocation())), FVector(0));
	}
}

// NETWORKING
//
// FBulletSimulationState ATestActor::getState()
// {
// 	FBulletSimulationState state;
// 	state.FrameNumber = CurrentFrameNumber;
//
// 	for (int i = 0; i < BtRigidBodies.Num(); i++)
// 	{
// 		btRigidBody* body = BtRigidBodies[i];
// 		FBulletObjectState os;
// 		
// 		os.Transform = BulletHelpers::ToUE(body->getWorldTransform(), GetActorLocation());
// 		os.Velocity = BulletHelpers::ToUEDir(body->getLinearVelocity(), false);
// 		os.AngularVelocity = BulletHelpers::ToUEDir(body->getAngularVelocity(), false);
// 		os.Force = BulletHelpers::ToUEDir(body->getTotalForce(), false);
// 		
// 		state.insert(os, i);
// 	}
// 	return state;
// }

//
// void ATestActor::EnqueueInput(FBulletPlayerInput Input, int32 ID)
// {
// 	InputBuffers[ID].Enqueue(Input);
// 	GEngine->AddOnScreenDebugMessage(-1,
// 		5.0f,
// 		FColor::Red,
// 		TEXT("Added input")
// 	);
// }
//
// void ATestActor::ConsumeInput(int32 ID)
// {
// 	// if (!InputBuffers.Contains(ID)) { return; }
//
// 	if (!InputBuffers.IsValidIndex(ID)) { return; }
//
// 	GEngine->AddOnScreenDebugMessage(-1,
// 		5.0f,
// 		FColor::Red,
// 		TEXT("Valid!")
// 	);
// 	
// 	TOptional<FBulletPlayerInput> optional = InputBuffers[ID].Dequeue();
// 	if (optional.IsSet())
// 	{
// 		// Get the actual input value
// 		FBulletPlayerInput input = optional.GetValue();
//         
// 		btRigidBody* body = BtRigidBodies[ID];
// 		void* userPtr = body->getUserPointer();
// 		AActor* actor = static_cast<AActor*>(userPtr);
// 		
// 		// Check if the actor implements the interface
// 		if (actor && actor->GetClass()->ImplementsInterface(UControllableInterface::StaticClass()))
// 		{
// 			IControllableInterface::Execute_OnPlayerInput(actor, input);
// 		}
// 	} else
// 	{
// 		// do last input
// 		// how? is it cached somewhere?
// 	}
// }

//
// void ATestActor::ConsumeAllInputs()
// {
// 	
// }


// END NEW METHODS

void ATestActor::SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution)
{
	for (AActor* Actor : Actors)
	{
		ExtractPhysicsGeometry(Actor,
			[Actor, this, Friction, Restitution](btCollisionShape* Shape, const FTransform& RelTransform)
			{
				// Every sub-collider in the actor is passed to this callback function
				// We're baking this in world space, so apply actor transform to relative
				const FTransform FinalXform = RelTransform * Actor->GetActorTransform();
				AddStaticCollision(Shape, FinalXform, Friction, Restitution, Actor);
			});
	}
}
void ATestActor::AddStaticBody(AActor* Body, float Friction, float Restitution,int &ID)
{
		ExtractPhysicsGeometry(Body,[Body, this, Friction, Restitution](btCollisionShape* Shape, const FTransform& RelTransform)
		{
			// Every sub-collider in the actor is passed to this callback function
			// We're baking this in world space, so apply actor transform to relative
			const FTransform FinalXform = RelTransform * Body->GetActorTransform();
			AddStaticCollision(Shape, FinalXform, Friction, Restitution, Body);
		});
	ID = BtWorld->getNumCollisionObjects() - 1;
}
void ATestActor::AddProcBody(AActor* Body,  float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID)
{
	btCollisionShape* Shape = GetTriangleMeshShape(a,b,c,d);
		const FTransform FinalXform = Body->GetActorTransform();
	 procbody=	AddStaticCollision(Shape, FinalXform, Friction, Restitution, Body);
	ID = BtWorld->getNumCollisionObjects() - 1;
}
void ATestActor::UpdateProcBody(AActor* Body, float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID, int PrevID)
{
	BtWorld->removeCollisionObject(procbody);
	procbody = nullptr;

	btCollisionShape* Shape = GetTriangleMeshShape(a, b, c, d);
	const FTransform FinalXform = Body->GetActorTransform();
	procbody = AddStaticCollision(Shape, FinalXform, Friction, Restitution, Body);
	
	ID = BtWorld->getNumCollisionObjects() - 1;
}
void ATestActor::AddRigidBody(AActor* Body /* the pawn/actor */, float Friction, float Restitution, int& ID,float mass)
{
	BtCriticalSection.Lock();
	auto rbody = AddRigidBody(Body, GetCachedDynamicShapeData(Body, mass), Friction, Restitution);
	ID = BtRigidBodies.Num() - 1;
	// AddBodyToMap(ID, rbody);
	BtCriticalSection.Unlock();
}
void ATestActor::UpdatePlayertransform(AActor* player, int ID)
{
		BtWorld->getCollisionObjectArray()[ID]->setWorldTransform(BulletHelpers::ToBt(player->GetActorTransform(), GetActorLocation()));
}
void ATestActor::ExtractPhysicsGeometry(AActor* Actor, PhysicsGeometryCallback CB)
{
	TInlineComponentArray<UActorComponent*, 20> Components;
	// Used to easily get a component's transform relative to actor, not parent component
	const FTransform InvActorTransform = Actor->GetActorTransform().Inverse();

	// Collisions from meshes
	
	Actor->GetComponents(UStaticMeshComponent::StaticClass(), Components);
	for (auto&& Comp : Components)
	{
		ExtractPhysicsGeometry(Cast<UStaticMeshComponent>(Comp), InvActorTransform, CB);
	}
	
	// Collisions from separate collision components
	Actor->GetComponents(UShapeComponent::StaticClass(), Components);
	for (auto&& Comp : Components)
	{
		ExtractPhysicsGeometry(Cast<UShapeComponent>(Comp), InvActorTransform, CB);
	}
}
btCollisionObject* ATestActor::AddStaticCollision(btCollisionShape* Shape, const FTransform& Transform, float Friction,
float Restitution, AActor* Actor)
{
	btTransform Xform = BulletHelpers::ToBt(Transform, GetActorLocation());
	btCollisionObject* Obj = new btCollisionObject();
	Obj->setCollisionShape(Shape);
	Obj->setWorldTransform(Xform);
	Obj->setFriction(Friction);
	Obj->setRestitution(Restitution);
	Obj->setUserPointer(Actor);
	Obj->setActivationState(DISABLE_DEACTIVATION);
	BtWorld->addCollisionObject(Obj);
	UE_LOG(LogTemp, Warning, TEXT("Static geom added"));
	BtStaticObjects.Add(Obj);
	
	return Obj;
}
void ATestActor::ExtractPhysicsGeometry(UStaticMeshComponent* SMC, const FTransform& InvActorXform, PhysicsGeometryCallback CB)
{
	UStaticMesh* Mesh = SMC->GetStaticMesh();
	if (!Mesh)
		return;

	// We want the complete transform from actor to this component, not just relative to parent
	FTransform CompFullRelXForm = SMC->GetComponentTransform() * InvActorXform;
	ExtractPhysicsGeometry(CompFullRelXForm, Mesh->GetBodySetup(), CB);

	// Not supporting complex collision shapes right now
	// If we did, note that Mesh->ComplexCollisionMesh is WITH_EDITORONLY_DATA so not available at runtime
	// See StaticMeshRender.cpp, FStaticMeshSceneProxy::GetDynamicMeshElements
	// Line 1417+, bDrawComplexCollision
	// Looks like we have to access LODForCollision, RenderData->LODResources
	// So they use a mesh LOD for collision for complex shapes, never drawn usually?

}
void ATestActor::ExtractPhysicsGeometry(UShapeComponent* Sc, const FTransform& InvActorXform, PhysicsGeometryCallback CB)
{
	// We want the complete transform from actor to this component, not just relative to parent
	FTransform CompFullRelXForm = Sc->GetComponentTransform() * InvActorXform;
	ExtractPhysicsGeometry(CompFullRelXForm, Sc->ShapeBodySetup, CB);
}
void ATestActor::ExtractPhysicsGeometry(const FTransform& XformSoFar, UBodySetup* BodySetup, PhysicsGeometryCallback CB)
{
	FVector Scale = XformSoFar.GetScale3D();
	btCollisionShape* Shape = nullptr;

	// Iterate over the simple collision shapes
	for (auto&& Box : BodySetup->AggGeom.BoxElems)
	{
		// We'll re-use based on just the LxWxH, including actor scale
		// Rotation and centre will be baked in world space
		FVector Dimensions = FVector(Box.X, Box.Y, Box.Z) * Scale;
		Shape = GetBoxCollisionShape(Dimensions);
		FTransform ShapeXform(Box.Rotation, Box.Center);
		// Shape transform adds to any relative transform already here
		FTransform XForm = ShapeXform * XformSoFar;
		CB(Shape, XForm);
	}
	for (auto&& Sphere : BodySetup->AggGeom.SphereElems)
	{
		// Only support uniform scale so use X
		Shape = GetSphereCollisionShape(Sphere.Radius * Scale.X);
		FTransform ShapeXform(FRotator::ZeroRotator, Sphere.Center);
		// Shape transform adds to any relative transform already here
		FTransform XForm = ShapeXform * XformSoFar;
		CB(Shape, XForm);
	}
	// Sphyl == Capsule (??)
	for (auto&& Capsule : BodySetup->AggGeom.SphylElems)
	{
		// X scales radius, Z scales height
		Shape = GetCapsuleCollisionShape(Capsule.Radius * Scale.X, Capsule.Length * Scale.Z);
		// Capsules are in Z in UE, in Y in Bullet, so roll -90
		FRotator Rot(0, 0, -90);
		// Also apply any local rotation
		Rot += Capsule.Rotation;
		FTransform ShapeXform(Rot, Capsule.Center);
		// Shape transform adds to any relative transform already here
		FTransform XForm = ShapeXform * XformSoFar;
		CB(Shape, XForm);
	}
	for (int i = 0; i < BodySetup->AggGeom.ConvexElems.Num(); ++i)
	{
		Shape = GetConvexHullCollisionShape(BodySetup, i, Scale);
		CB(Shape, XformSoFar);
	}

}
btCollisionShape* ATestActor::GetBoxCollisionShape(const FVector& Dimensions)
{
	// Simple brute force lookup for now, probably doesn't need anything more clever
	btVector3 HalfSize = BulletHelpers::ToBtSize(Dimensions * 0.5);
	for (auto&& S : BtBoxCollisionShapes)
	{
		btVector3 Sz = S->getHalfExtentsWithMargin();
		if (FMath::IsNearlyEqual(Sz.x(), HalfSize.x()) &&
			FMath::IsNearlyEqual(Sz.y(), HalfSize.y()) &&
			FMath::IsNearlyEqual(Sz.z(), HalfSize.z()))
		{
			return S;
		}
	}
	// Not found, create
	auto S = new btBoxShape(HalfSize);
	// Get rid of margins, just cause issues for me
	S->setMargin(0);
	BtBoxCollisionShapes.Add(S);

	return S;

}
btCollisionShape* ATestActor::GetSphereCollisionShape(float Radius)
{
	// Simple brute force lookup for now, probably doesn't need anything more clever
	btScalar Rad = BulletHelpers::ToBtSize(Radius);
	for (auto&& S : BtSphereCollisionShapes)
	{
		// Bullet subtracts a margin from its internal shape, so add back to compare
		if (FMath::IsNearlyEqual(S->getRadius(), Rad))
		{
			return S;
		}
	}
	// Not found, create
	auto S = new btSphereShape(Rad);
	// Get rid of margins, just cause issues for me
	S->setMargin(0);
	BtSphereCollisionShapes.Add(S);
	return S;
}
btCollisionShape* ATestActor::GetCapsuleCollisionShape(float Radius, float Height)
{
	// Simple brute force lookup for now, probably doesn't need anything more clever
	btScalar R = BulletHelpers::ToBtSize(Radius);
	btScalar H = BulletHelpers::ToBtSize(Height);
	btScalar HalfH = H * 0.5f;

	for (auto&& S : BtCapsuleCollisionShapes)
	{
		// Bullet subtracts a margin from its internal shape, so add back to compare
		if (FMath::IsNearlyEqual(S->getRadius(), R) &&
			FMath::IsNearlyEqual(S->getHalfHeight(), HalfH))
		{
			return S;
		}
	}

	// Not found, create
	auto S = new btCapsuleShape(R, H);
	BtCapsuleCollisionShapes.Add(S);

	return S;

}
btCollisionShape* ATestActor::GetTriangleMeshShape(TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d)
{
	btTriangleMesh* triangleMesh = new btTriangleMesh();

	for (int i =0;i<a.Num();i++)
	{
		triangleMesh->addTriangle(BulletHelpers::ToBtPos(a[i], FVector::ZeroVector), BulletHelpers::ToBtPos(b[i], FVector::ZeroVector), BulletHelpers::ToBtPos(c[i], FVector::ZeroVector));
		triangleMesh->addTriangle(BulletHelpers::ToBtPos(a[i], FVector::ZeroVector), BulletHelpers::ToBtPos(c[i], FVector::ZeroVector), BulletHelpers::ToBtPos(d[i], FVector::ZeroVector));

	}
	btBvhTriangleMeshShape* Trimesh= new btBvhTriangleMeshShape(triangleMesh,true);
	return Trimesh;
}
btCollisionShape* ATestActor::GetConvexHullCollisionShape(UBodySetup* BodySetup, int ConvexIndex, const FVector& Scale)
{
	for (auto&& S : BtConvexHullCollisionShapes)
	{ 
		if (S.BodySetup == BodySetup && S.HullIndex == ConvexIndex && S.Scale.Equals(Scale))
		{
			return S.Shape;
		}
	}
	const FKConvexElem& Elem = BodySetup->AggGeom.ConvexElems[ConvexIndex];
	auto C = new btConvexHullShape();
	for (auto&& P : Elem.VertexData)
	{
		C->addPoint(BulletHelpers::ToBtPos(P, FVector::ZeroVector));
	}
	// Very important! Otherwise, there's a gap between 
	C->setMargin(0);
	// Apparently this is good to call?
	C->initializePolyhedralFeatures();

	BtConvexHullCollisionShapes.Add({
		BodySetup,
		ConvexIndex,
		Scale,
		C
		});

	return C;
}
const ATestActor::CachedDynamicShapeData& ATestActor::GetCachedDynamicShapeData(AActor* Actor, float Mass)
{
	// We re-use compound shapes based on (leaf) BP class
	const FName ClassName = Actor->GetClass()->GetFName();
	// Because we want to support compound colliders, we need to extract all colliders first before
	// constructing the final body.
	TArray<btCollisionShape*, TInlineAllocator<20>> Shapes;
	TArray<FTransform, TInlineAllocator<20>> ShapeRelXforms;
	ExtractPhysicsGeometry(Actor,
		[&Shapes, &ShapeRelXforms](btCollisionShape* Shape, const FTransform& RelTransform)
		{
			Shapes.Add(Shape);
			ShapeRelXforms.Add(RelTransform);
		});
	CachedDynamicShapeData ShapeData;
	ShapeData.ClassName = ClassName;
	// Single shape with no transform is simplest
	if (ShapeRelXforms.Num() == 1 &&
		ShapeRelXforms[0].EqualsNoScale(FTransform::Identity))
	{
		ShapeData.Shape = Shapes[0];
		// just to make sure we don't think we have to clean it up; simple shapes are already stored
		ShapeData.bIsCompound = false;
	}
	else
	{
		// Compound or offset single shape; we will cache these by blueprint type
		btCompoundShape* CS = new btCompoundShape();
		for (int i = 0; i < Shapes.Num(); ++i)
		{
			// We don't use the actor origin when converting transform in this case since object space
			// Note that btCompoundShape doesn't free child shapes, which is fine since they're tracked separately
			CS->addChildShape(BulletHelpers::ToBt(ShapeRelXforms[i], FVector::ZeroVector), Shapes[i]);
		}
		ShapeData.Shape = CS;
		ShapeData.bIsCompound = true;
	}

	// Calculate Inertia
	ShapeData.Mass = Mass;
	ShapeData.Shape->calculateLocalInertia(Mass, ShapeData.Inertia);

	// Cache for future use
	CachedDynamicShapes.Add(ShapeData);

	return CachedDynamicShapes.Last();

}
btRigidBody* ATestActor::AddRigidBody(AActor* Actor, const ATestActor::CachedDynamicShapeData& ShapeData, float Friction, float Restitution)
{
	return AddRigidBody(Actor, ShapeData.Shape, ShapeData.Inertia, ShapeData.Mass, Friction, Restitution);
}
btRigidBody* ATestActor::AddRigidBody(AActor* Actor, btCollisionShape* CollisionShape, btVector3 Inertia, float Mass, float Friction, float Restitution)
{
	// GEngine->AddOnScreenDebugMessage(        -1,          // Key: Unique identifier for the message, -1 to display multiple times
	// 	5.0f,        // Duration: Time in seconds the message stays on screen
	// 	FColor::Red, // Color: Text color, e.g., FColor::Red, FColor::Green
	// 	TEXT("A body was created") // Message: The string to display
	// );
	auto Origin = GetActorLocation();
	auto MotionState = new BulletCustomMotionState(Actor, Origin);
	const btRigidBody::btRigidBodyConstructionInfo rbInfo(Mass*10, MotionState, CollisionShape, Inertia*10);
	btRigidBody* Body = new btRigidBody(rbInfo);
	Body->setUserPointer(Actor);
	Body->setActivationState(DISABLE_DEACTIVATION); // changed from ACTIVE_TAG, change back after the freezing is resolved - Gage
	Body->setDeactivationTime(0);

	BtWorld->addRigidBody(Body);
	BtRigidBodies.Add(Body);

	return Body;
	
}
void ATestActor::StepPhysics(float DeltaSeconds, int substeps)
{
	BtWorld->stepSimulation(DeltaSeconds, substeps, 1. / 60);
}
void ATestActor::SetPhysicsState(int ID, FTransform transforms, FVector Velocity, FVector AngularVelocity, FVector& Force)
{
	if (BtRigidBodies[ID]) {
		BtRigidBodies[ID]->setWorldTransform(BulletHelpers::ToBt(transforms, GetActorLocation()));
		BtRigidBodies[ID]->setLinearVelocity(BulletHelpers::ToBtPos(Velocity, GetActorLocation()));
		BtRigidBodies[ID]->setAngularVelocity(BulletHelpers::ToBtPos(AngularVelocity, FVector(0)));
		//BtRigidBodies[ID]->apply(BulletHelpers::ToBtPos(Velocity, GetActorLocation()));
	}
}

void ATestActor::GetPhysicsState(int ID, FTransform& transforms, FVector& Velocity, FVector& AngularVelocity, FVector& Force)
{
	// Safety check for valid ID
	if (!BtRigidBodies.IsValidIndex(ID) || !BtRigidBodies[ID])
	{
		// Set default values instead of crashing
		transforms = FTransform::Identity;
		Velocity = FVector::ZeroVector;
		AngularVelocity = FVector::ZeroVector;
		Force = FVector::ZeroVector;
        
		UE_LOG(LogTemp, Warning, TEXT("GetPhysicsState: Invalid ID %d (BtRigidBodies size: %d)"), 
			ID, BtRigidBodies.Num());
		return;
	}
    
	// Debug output
	UE_LOG(LogTemp, Verbose, TEXT("Raw Bullet Velocity: %f,%f,%f"), 
		BtRigidBodies[ID]->getLinearVelocity().x(),
		BtRigidBodies[ID]->getLinearVelocity().y(),
		BtRigidBodies[ID]->getLinearVelocity().z());

	transforms = BulletHelpers::ToUE(BtRigidBodies[ID]->getWorldTransform(), GetActorLocation());
	Velocity = BulletHelpers::ToUEDir(BtRigidBodies[ID]->getLinearVelocity(), false);
	AngularVelocity = BulletHelpers::ToUEDir(BtRigidBodies[ID]->getAngularVelocity(), false);
	Force = BulletHelpers::ToUEDir(BtRigidBodies[ID]->getTotalForce(), false);
}


void ATestActor::ResetSim()
{
	
	for (int i = 0; i < BtRigidBodies.Num(); i++)
	{
		BtWorld->removeRigidBody(BtRigidBodies[i]);
		BtRigidBodies[i]->setActivationState(ACTIVE_TAG);
		BtRigidBodies[i]->setDeactivationTime(0);
		BtRigidBodies[i]->clearForces();
	}
	
	BtWorld = new btDiscreteDynamicsWorld(BtCollisionDispatcher, BtBroadphase, BtConstraintSolver, BtCollisionConfig);
	BtWorld->setGravity(btVector3(0, 0, -9.8));
	BtBroadphase->resetPool(BtCollisionDispatcher);
	BtConstraintSolver->reset();
	for (int i = 0; i < BtRigidBodies.Num(); i++)
	{
		BtWorld->addRigidBody(BtRigidBodies[i]);
	}
}
