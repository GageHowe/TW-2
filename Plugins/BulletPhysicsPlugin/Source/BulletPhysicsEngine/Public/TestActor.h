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

	// Array of FIFO Buffers of inputs coming into the server
	// Array index dictates the 
	TArray<TMpscQueue<FBulletPlayerInput>> InputBuffers;

	UPROPERTY(Blueprintable, BlueprintReadWrite, Category = "Physics Networking")
	TMap<int32, int32> ServerIdToClientId; // Client-side mapping of server IDs to local IDs
	
	// current simulation state
	// server sends this to client who sets their state to it and extrapolates from there
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	FBulletSimulationState CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Physics Networking")
	int32 CurrentFrameNumber = 0;	// Global current frame number
	const float FixedDeltaTime = 1.0f / 60.0f;
	
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
	btIDebugDraw* BtDebugDraw;	// Custom debug interface
	
	FCriticalSection BtCriticalSection; // one rigid body can be inserted at a time
	TArray<btRigidBody*> BtRigidBodies;	// Dynamic bodies; this is where IDs will matter
	
	TArray<btCollisionObject*> BtStaticObjects;	// Static colliders
	btCollisionObject* procbody;
	TArray<btBoxShape*> BtBoxCollisionShapes;
	TArray<btSphereShape*> BtSphereCollisionShapes;
	TArray<btCapsuleShape*> BtCapsuleCollisionShapes;
	btSequentialImpulseConstraintSolver* mt;
	struct ConvexHullShapeHolder	// Structure to hold re-usable ConvexHull shapes based on origin BodySetup / subindex / scale
	{
		UBodySetup* BodySetup;
		int HullIndex;
		FVector Scale;
		btConvexHullShape* Shape;
	};
	TArray<ConvexHullShapeHolder> BtConvexHullCollisionShapes;
	struct CachedDynamicShapeData	// These shapes are for *potentially* compound rigid body shapes
	{
		FName ClassName; // class name for cache
		btCollisionShape* Shape;
		bool bIsCompound; // if true, this is a compound shape and so must be deleted
		btScalar Mass;
		btVector3 Inertia; // because we like to precalc this
	};
	TArray<CachedDynamicShapeData> CachedDynamicShapes;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Bullet Physics|Objects") 	// This list can be edited in the level, linking to placed static actors
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
	virtual void BeginPlay() override;	// Called when the game starts or when spawned

public:	
	virtual void Tick(float DeltaTime) override;	// Called every frame

	void SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution);
	
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

	const ATestActor::CachedDynamicShapeData& GetCachedDynamicShapeData(AActor* Actor, float Mass);

	btRigidBody* AddRigidBody(AActor* Actor, const ATestActor::CachedDynamicShapeData& ShapeData, float Friction, float Restitution);
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

	// // Call this from blueprints to create a synced object
	// UFUNCTION(Server, Reliable, BlueprintCallable)
	// void SR_AddRigidBody(AActor* Body /* the pawn/actor */, float Friction, float Restitution, int ClientID,float mass);

	UFUNCTION(BlueprintCallable)
	FBulletSimulationState getState();
	
	// send the physics state to the players
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void MC_SendStateToClients(FBulletSimulationState serverState);

	// // one client sends its inputs to the server
	// UFUNCTION(Server, Reliable, BlueprintCallable)
	// void SR_SendInputToServer(FBulletPlayerInput);
	
	// used for syncing frames between server and clients
	UFUNCTION(BlueprintCallable)
	int GetPingInFrames();
	
};
