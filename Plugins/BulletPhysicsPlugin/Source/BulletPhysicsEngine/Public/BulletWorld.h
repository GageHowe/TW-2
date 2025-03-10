// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PhysicsEngine/BodySetup.h"
#include "Containers/Queue.h"
#include "ThirdParty/BulletPhysicsEngineLibrary/BulletMinimal.h"
#include "ThirdParty/BulletPhysicsEngineLibrary/src/bthelper.h"
#include "ThirdParty/BulletPhysicsEngineLibrary/src/motionstate.h"
#include "ThirdParty/BulletPhysicsEngineLibrary/src/BulletMain.h"
#include "ThirdParty/BulletPhysicsEngineLibrary/debug/btdebug.h"
#include "Components/ShapeComponent.h"
#include <functional>
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "BulletWorld.generated.h"

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

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
    	int32 FrameNumber; // Frame number for this input
};

USTRUCT(BlueprintType) // A FBulletObjectState is the instantaneous state of one object in a frame
struct FBulletObjectState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	int32 ObjectID; // Unique ID for the physics object

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FTransform Transform; // Transform of the physics object

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FVector Velocity;    // Linear velocity of the physics object

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	FVector AngularVelocity; // Angular velocity of the physics object

	UPROPERTY(BlueprintReadWrite, Category = "Physics Networking")
	int32 FrameNumber; // Frame number for this state
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

UCLASS()
class BULLETPHYSICSENGINE_API ABulletWorld : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABulletWorld();

	// FIFO Buffer of inputs coming into the server
	TMpscQueue<FBulletPlayerInput> InputBuffer;
	
	// History buffer for client-side prediction
	// UPROPERTY()
	TArray<FBulletSimulationState> History;

	UFUNCTION(BlueprintCallable)
	int getNumInHistory()
	{
		return History.Num();
	}

	// current simulation state
	// server sends this to client who sets their state to it and extrapolates from there
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	FBulletSimulationState CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int32 CurrentFrameNumber = 0;	// Global current frame number
	const float FixedDeltaTime = 1.0f / 60.0f;
	
	// UFUNCTION(BlueprintCallable) 	// Add input to the server's input buffer
	// void AddInputToBuffer(const FPlayerInput& Input);
	//
	// UFUNCTION(BlueprintCallable) 	// Process inputs from the buffer at a fixed rate
	// void ProcessInputBuffer(float DeltaTime);
	//
	// UFUNCTION(BlueprintCallable)	// Predict the physics state on the client
	// void PredictPhysicsState(const FBulletPlayerInput& Input);
	//
	// UFUNCTION(BlueprintCallable)	// Reconcile the client's state with the server's state
	// void ReconcilePhysicsState(const FBulletPhysicsState& ServerState);
	//
	// UFUNCTION(BlueprintCallable)	// Rewind and re-simulate physics if necessary
	// void RewindAndResimulate(int32 FrameNumber);
	//
	// UFUNCTION(BlueprintCallable)	// Serialize the physics state for networking
	// FBulletPhysicsState SerializePhysicsState() const;
	//
	// UFUNCTION(BlueprintCallable) 	// Deserialize the physics state from the server
	// void DeserializePhysicsState(const FBulletPhysicsState& State);
	//
	// UFUNCTION(BlueprintCallable)	// Send periodic updates from the server to clients
	// void SendPhysicsUpdate();
	
	// Bullet section
	// Global objects
	btCollisionConfiguration* BtCollisionConfig;
	btCollisionDispatcher* BtCollisionDispatcher;
	btBroadphaseInterface* BtBroadphase;
	btConstraintSolver* BtConstraintSolver;
	btDiscreteDynamicsWorld* BtWorld;
	BulletHelpers* BulletHelpers;
	BulletDebugDraw* btdebugdraw;
	btStaticPlaneShape* plane;
	// Custom debug interface
	btIDebugDraw* BtDebugDraw;
	// Dynamic bodies
	TArray<btRigidBody*> BtRigidBodies;
	// Static colliders
	TArray<btCollisionObject*> BtStaticObjects;
	btCollisionObject* procbody;
	// Re-usable collision shapes
	TArray<btBoxShape*> BtBoxCollisionShapes;
	TArray<btSphereShape*> BtSphereCollisionShapes;
	TArray<btCapsuleShape*> BtCapsuleCollisionShapes;
	btSequentialImpulseConstraintSolver* mt;
	// Structure to hold re-usable ConvexHull shapes based on origin BodySetup / subindex / scale
	struct ConvexHullShapeHolder
	{
		UBodySetup* BodySetup;
		int HullIndex;
		FVector Scale;
		btConvexHullShape* Shape;
	};
	TArray<ConvexHullShapeHolder> BtConvexHullCollisionShapes;
	// These shapes are for *potentially* compound rigid body shapes
	struct CachedDynamicShapeData
	{
		FName ClassName; // class name for cache
		btCollisionShape* Shape;
		bool bIsCompound; // if true, this is a compound shape and so must be deleted
		btScalar Mass;
		btVector3 Inertia; // because we like to precalc this
	};
	TArray<CachedDynamicShapeData> CachedDynamicShapes;

	// This list can be edited in the level, linking to placed static actors
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Physics|Objects")
		TArray<AActor*> PhysicsStaticActors1;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Physics|Objects")
		TArray<AActor*> DynamicActors;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Physics|Objects")
		FString text="ra";
	// These properties can only be edited in the Blueprint
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet Physics|Objects")
		float PhysicsStatic1Friction = 0.6;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet Physics|Objects")
		float PhysicsStatic1Restitution = 0.3;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet Physics|Objects")
		float randvar;
	// I chose not to use spinning / rolling friction in the end since it had issues!

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution);

	UFUNCTION(BlueprintCallable)
	void activate();
	UFUNCTION(BlueprintCallable)
	void AddStaticBody(AActor* player, float Friction, float Restitution,int &ID);
	UFUNCTION(BlueprintCallable)
	void AddProcBody(AActor* Body,  float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID);
	UFUNCTION(BlueprintCallable)
	void UpdateProcBody(AActor* Body, float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID, int PrevID);
	UFUNCTION(BlueprintCallable)
	void AddRigidBody(AActor* Body, float Friction, float Restitution, int& ID,float mass);
	UFUNCTION(BlueprintCallable)
	void UpdatePlayertransform(AActor* player, int ID);
	UFUNCTION(BlueprintCallable)
	void AddImpulse( int ID, FVector Impulse, FVector Location);
	typedef const std::function<void(btCollisionShape* /*SingleShape*/, const FTransform& /*RelativeXform*/)>& PhysicsGeometryCallback;
	UFUNCTION(BlueprintCallable)
	void AddForce(int ID, FVector Force, FVector Location);
	UFUNCTION(BlueprintCallable)
	void AddCentralForce(int ID, FVector Force);
	UFUNCTION(BlueprintCallable)
	void AddTorque(int ID, FVector Torque);
	UFUNCTION(BlueprintCallable)
	void AddTorqueImpulse(int ID, FVector Torque);
	typedef const std::function<void(btCollisionShape* /*SingleShape*/, const FTransform& /*RelativeXform*/)>& PhysicsGeometryCallback;
	void ExtractPhysicsGeometry(AActor* Actor, PhysicsGeometryCallback CB);

	btCollisionObject* AddStaticCollision(btCollisionShape* Shape, const FTransform& Transform, float Friction, float Restitution, AActor* Actor);


	void ExtractPhysicsGeometry(UStaticMeshComponent* SMC, const FTransform& InvActorXform, PhysicsGeometryCallback CB);

	void ExtractPhysicsGeometry(UShapeComponent* Sc, const FTransform& InvActorXform, PhysicsGeometryCallback CB);

	void ExtractPhysicsGeometry(const FTransform& XformSoFar, UBodySetup* BodySetup, PhysicsGeometryCallback CB);

	btCollisionShape* GetBoxCollisionShape(const FVector& Dimensions);

	btCollisionShape* GetSphereCollisionShape(float Radius);

	btCollisionShape* GetCapsuleCollisionShape(float Radius, float Height);

	btCollisionShape* GetTriangleMeshShape(TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d);
	

	btCollisionShape* GetConvexHullCollisionShape(UBodySetup* BodySetup, int ConvexIndex, const FVector& Scale);

	const ABulletWorld::CachedDynamicShapeData& GetCachedDynamicShapeData(AActor* Actor, float Mass);

	btRigidBody* AddRigidBody(AActor* Actor, const ABulletWorld::CachedDynamicShapeData& ShapeData, float Friction, float Restitution);

	btRigidBody* AddRigidBody(AActor* Actor, btCollisionShape* CollisionShape, btVector3 Inertia, float Mass, float Friction, float Restitution);
	UFUNCTION(BlueprintCallable)
	void StepPhysics(float DeltaSeconds, int substeps);
	UFUNCTION(BlueprintCallable)
	void SetPhysicsState(int ID, FTransform transforms, FVector Velocity, FVector AngularVelocity,FVector& Force);
	UFUNCTION(BlueprintCallable)
	void GetPhysicsState(int ID, FTransform& transforms, FVector& Velocity, FVector& AngularVelocity, FVector& Force);
	UFUNCTION(BlueprintCallable)
	void GetVelocityAtLocation(int ID, FVector Location, FVector& Velocity);
	UFUNCTION(BlueprintCallable)
	void ResetSim();

	// networking code
	
	UFUNCTION(Server, Unreliable, BlueprintCallable)
	void Server_SendInputsToServer(FBulletPlayerInput input);
	
	TMap<int32, btRigidBody*> PhysicsObjects;

	UFUNCTION(BlueprintCallable)
	bool StatesMatch(const FBulletSimulationState& A, const FBulletSimulationState& B) const;
	
	UFUNCTION(BlueprintCallable)
	void SaveCurrentState();
	
	UFUNCTION(BlueprintCallable)
	void ConsumeInput();
	
	// send state to all clients
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void MC_SendPhysicsStateToClient(FBulletSimulationState state);
	
	UFUNCTION(BlueprintCallable)
	void StepPhysicsXFrames(int frames);

	UFUNCTION(BlueprintCallable)
	void ApplyObjectState(FBulletObjectState& ObjectState, const FBulletObjectState& DesiredState);

	// client corrects to the state of some frame on the server
	UFUNCTION(BlueprintCallable)
	void ClientCorrection(FBulletSimulationState state);


	// HELPER FUNCTIONS FOR MEMORY AND OBJECT MANAGEMENT
	
	void AddBodyToMap(int32 ObjectID, btRigidBody* Body)
	{
		PhysicsObjects.Add(ObjectID, Body);
	}

	void RemoveBodyFromMap(int32 ObjectID)
	{
		if (btRigidBody* Body = PhysicsObjects.FindRef(ObjectID))
		{
			// Clean up the Bullet rigid body
			delete Body->getMotionState();
			delete Body;
		}
		PhysicsObjects.Remove(ObjectID);
	}

	btRigidBody* GetBodyFromMap(int32 ObjectID) const
	{
		return PhysicsObjects.FindRef(ObjectID);
	}
	
	virtual void BeginDestroy() override
	{
		Super::BeginDestroy();

		// Clean up all Bullet rigid bodies
		for (auto& Pair : PhysicsObjects)
		{
			if (btRigidBody* Body = Pair.Value)
			{
				delete Body->getMotionState();
				delete Body;
			}
		}
		PhysicsObjects.Empty();
	}
};
