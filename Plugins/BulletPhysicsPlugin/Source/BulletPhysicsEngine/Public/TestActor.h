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
#include "ControllableInterface.h"
#include "Net/UnrealNetwork.h"
#include "ControllableInterface.h"
#include "TestActor.generated.h"

USTRUCT(BlueprintType)
struct BULLETPHYSICSENGINE_API Ftris
{
	GENERATED_BODY()
	FVector a;
	FVector b;
	FVector c;
	FVector d;
};

UCLASS()
class BULLETPHYSICSENGINE_API ATestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ATestActor();

	// Input buffers for each client
	UPROPERTY()
	TMap<int32, FInputBuffer> ClientInputBuffers;
	
	// State cache for reconciliation
	UPROPERTY()
	FStateCache StateCache;
	
	UPROPERTY(Blueprintable, BlueprintReadWrite, Category = "Physics Networking")
	TMap<int32, int32> ServerIdToClientId; // Server to client ID mapping
	
	UPROPERTY(Blueprintable, BlueprintReadWrite, Category = "Physics Networking")
	TMap<int32, int32> ClientIdToServerId; // Client to server ID mapping (client-side)
	
	FCriticalSection MapCriticalSection;
	
	// Current simulation state
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	FBulletSimulationState CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int32 CurrentFrameNumber = 0;	// Global current frame number
	
	// Set up by default to run at 60 FPS physics
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	float FixedDeltaTime = 1.0f / 60.0f;
	
	// How many frames of buffer we want for client inputs - helps with jitter
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int32 TargetBufferSize = 3;
	
	// Flag to indicate we're reconciling
	UPROPERTY()
	bool bIsReconciling = false;
	
	// Last frame we reconciled at
	UPROPERTY()
	int32 LastReconcileFrame = -1;
	
	// Bullet physics components
	btCollisionConfiguration* BtCollisionConfig;
	btCollisionDispatcher* BtCollisionDispatcher;
	btBroadphaseInterface* BtBroadphase;
	btConstraintSolver* BtConstraintSolver;
	btDiscreteDynamicsWorld* BtWorld;
	BulletHelpers* BulletHelpers;
	BulletDebugDraw* btdebugdraw;
	btStaticPlaneShape* plane;
	btIDebugDraw* BtDebugDraw;
	
	TArray<btRigidBody*> BtRigidBodies;
	FCriticalSection BtCriticalSection;
	
	TArray<btCollisionObject*> BtStaticObjects;
	btCollisionObject* procbody;
	TArray<btBoxShape*> BtBoxCollisionShapes;
	TArray<btSphereShape*> BtSphereCollisionShapes;
	TArray<btCapsuleShape*> BtCapsuleCollisionShapes;
	btSequentialImpulseConstraintSolver* mt;
	
	struct ConvexHullShapeHolder
	{
		UBodySetup* BodySetup;
		int HullIndex;
		FVector Scale;
		btConvexHullShape* Shape;
	};
	TArray<ConvexHullShapeHolder> BtConvexHullCollisionShapes;
	
	struct CachedDynamicShapeData
	{
		FName ClassName;
		btCollisionShape* Shape;
		bool bIsCompound;
		btScalar Mass;
		btVector3 Inertia;
	};
	TArray<CachedDynamicShapeData> CachedDynamicShapes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Physics|Objects")
	TArray<AActor*> PhysicsStaticActors1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Physics|Objects")
	TArray<AActor*> DynamicActors;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Physics|Objects")
	FString text="ra";
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet Physics|Objects")
	float PhysicsStatic1Friction = 0.6;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet Physics|Objects")
	float PhysicsStatic1Restitution = 0.3;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bullet Physics|Objects")
	float randvar;

protected:
	virtual void BeginPlay() override;
	
	// Tick accumulator for fixed timestep
	float PhysicsAccumulator = 0.0f;

public:	
	virtual void Tick(float DeltaTime) override;

	// Bullet physics setup methods
	void SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution);
	
	UFUNCTION(BlueprintCallable)
	void AddStaticBody(AActor* player, float Friction, float Restitution, int &ID);
	
	UFUNCTION(BlueprintCallable)
	void AddProcBody(AActor* Body, float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID);
	
	UFUNCTION(BlueprintCallable)
	void UpdateProcBody(AActor* Body, float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID, int PrevID);
	
	UFUNCTION(BlueprintCallable)
	void AddRigidBody(AActor* Body, float Friction, float Restitution, int& ID, float mass);
	
	UFUNCTION(BlueprintCallable)
	void UpdatePlayertransform(AActor* player, int ID);
	
	UFUNCTION(BlueprintCallable)
	void AddImpulse(int ID, FVector Impulse, FVector Location);
	
	typedef const std::function<void(btCollisionShape* /*SingleShape*/, const FTransform& /*RelativeXform*/)>& PhysicsGeometryCallback;
	
	UFUNCTION(BlueprintCallable)
	void AddForce(int ID, FVector Force, FVector Location);
	
	UFUNCTION(BlueprintCallable)
	void AddCentralForce(int ID, FVector Force);
	
	UFUNCTION(BlueprintCallable)
	void AddTorque(int ID, FVector Torque);
	
	UFUNCTION(BlueprintCallable)
	void AddTorqueImpulse(int ID, FVector Torque);
	
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
	const ATestActor::CachedDynamicShapeData& GetCachedDynamicShapeData(AActor* Actor, float Mass);
	btRigidBody* AddRigidBody(AActor* Actor, const ATestActor::CachedDynamicShapeData& ShapeData, float Friction, float Restitution);
	btRigidBody* AddRigidBody(AActor* Actor, btCollisionShape* CollisionShape, btVector3 Inertia, float Mass, float Friction, float Restitution);
	
	UFUNCTION(BlueprintCallable)
	void StepPhysics(float DeltaSeconds, int substeps);
	
	UFUNCTION(BlueprintCallable)
	void SetPhysicsState(int ID, FTransform transforms, FVector Velocity, FVector AngularVelocity, FVector& Force);
	
	UFUNCTION(BlueprintCallable)
	void GetPhysicsState(int ID, FTransform& transforms, FVector& Velocity, FVector& AngularVelocity, FVector& Force);
	
	UFUNCTION(BlueprintCallable)
	void GetVelocityAtLocation(int ID, FVector Location, FVector& Velocity);
	
	UFUNCTION(BlueprintCallable)
	void ResetSim();

	// NETWORKING FUNCTIONS
	
	// Record current state for reconciliation
	UFUNCTION(BlueprintCallable)
	void RecordState();
	
	// Get current simulation state
	UFUNCTION(BlueprintCallable)
	FBulletSimulationState GetCurrentState();
	
	// Send state to clients
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void MC_SendStateToClients(FBulletSimulationState ServerState);
	
	// Step physics for multiple frames
	UFUNCTION(BlueprintCallable)
	void StepPhysicsXFrames(int32 FrameCount)
	{
		for (int32 i = 0; i < FrameCount; i++)
		{
			StepPhysics(FixedDeltaTime, 0);
		}
	}
	
	// Process client input
	UFUNCTION(BlueprintCallable)
	void ProcessClientInput(const FBulletPlayerInput& Input);
	
	// Apply input to object
	UFUNCTION(BlueprintCallable)
	void ApplyInput(const FBulletPlayerInput& Input);
	
	// Client sends input to server
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void Server_SendInput(const TArray<FBulletPlayerInput>& Inputs);
	
	// Server reconciles client state
	UFUNCTION(BlueprintCallable)
	void ReconcileState(const FBulletSimulationState& ServerState);
	
	// Enqueue client input
	UFUNCTION(BlueprintCallable)
	void EnqueueInput(const FBulletPlayerInput& Input, int32 ClientID);
	
	// Get estimated ping in frames
	UFUNCTION(BlueprintCallable)
	int32 GetPingInFrames();
	
	// Compress inputs before sending to reduce bandwidth
	UFUNCTION(BlueprintCallable)
	TArray<FBulletPlayerInput> CompressInputs(const TArray<FBulletPlayerInput>& Inputs);
	
	// Decompress inputs received from network
	UFUNCTION(BlueprintCallable)
	TArray<FBulletPlayerInput> DecompressInputs(const TArray<FBulletPlayerInput>& CompressedInputs);
	
	// Get snapshot of buffered inputs (for debugging)
	UFUNCTION(BlueprintCallable, BlueprintPure)
	TArray<FBulletPlayerInput> GetBufferedInputs(int32 ClientID, int32 MaxCount = 10);
	
	// Get serializable simulation state
	UFUNCTION(BlueprintCallable, BlueprintPure)
	FBulletSimulationState GetSerializableState();
};