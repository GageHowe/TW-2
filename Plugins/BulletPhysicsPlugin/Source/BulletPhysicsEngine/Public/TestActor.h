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
#include <queue>
#include "BlueprintEditor.h"
#include "HairStrandsInterface.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "helpers.h"
#include "TestActor.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ATestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATestActor();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBulletSimulationState CurrentState;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 CurrentFrameNumber = 0;	// Global current frame number
	const float FixedDeltaTime = 1.0f / 60.0f;

	// bidirectional map
	TMap<btRigidBody*, FNetworkGUID> BodyToGUID;
	TMap<FNetworkGUID, btRigidBody*> GUIDToBody;

	TMpscQueue<FBulletPlayerInput> PlayerInputQueue; // for server use
	
	


	
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

protected:
	virtual void BeginPlay() override;
public:	
	virtual void Tick(float DeltaTime) override;

	// THE FOLLOWING FUNCTIONS ARE FOR SYNCING ACTORS WITH UNIQUE IDs. FNetworkGUID is a unique, replicated ID for UObjects.
	AActor* GetActorFromNetGUID(FNetworkGUID guid)
	{
		UNetDriver* driver = GetWorld()->GetNetDriver();
		FNetGUIDCache* cache = driver->GuidCache.Get();
		if (driver && cache)
		{
			UObject *obj = cache->GetObjectFromNetGUID(guid, false);
			AActor* actor = Cast<AActor>(obj);
		}
		if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("GetActorFromNetGUID returned null"));
		return nullptr;
	}
	FNetworkGUID GetNetGUIDFromActor(AActor* actor) const
	{
		if (actor && GetWorld()->GetNetDriver())
		{ // this happens when playing as standalone
			FNetworkGUID NetGUID = GetWorld()->GetNetDriver()->GuidCache->GetOrAssignNetGUID(actor);
			return NetGUID;
		} else
		{ // this happens when playing as standalone
			if(GEngine) GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, TEXT("GetNetGUIDFromActor returned zeroed"));
			return FNetworkGUID();
		}
	}

	// use this to completely remove the body and references to it
	// Eg. when destroying an actor
	void DestroyRigidBody(btRigidBody* rigidbody)
	{
		GUIDToBody.Remove(*BodyToGUID.Find(rigidbody));
		BodyToGUID.Remove(rigidbody);
		BtWorld->removeRigidBody(rigidbody);
	}

	// THESE FUNCTIONS ARE PART OF THE API AND LARGELY SHOULDN'T BE TOUCHED
	void SetupStaticGeometryPhysics(TArray<AActor*> Actors, float Friction, float Restitution);
	UFUNCTION(BlueprintCallable)
	void AddStaticBody(AActor* player, float Friction, float Restitution,int &ID);
	UFUNCTION(BlueprintCallable)
	void AddProcBody(AActor* Body,  float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID);
	UFUNCTION(BlueprintCallable)
	void UpdateProcBody(AActor* Body, float Friction, TArray<FVector> a, TArray<FVector> b, TArray<FVector> c, TArray<FVector> d, float Restitution, int& ID, int PrevID);
	UFUNCTION(BlueprintCallable)
	void AddRigidBody(AActor* Body, float Friction, float Restitution,float mass);
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
};
