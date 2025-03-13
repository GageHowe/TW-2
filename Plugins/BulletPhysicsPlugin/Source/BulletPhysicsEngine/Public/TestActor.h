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
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameState.h"
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
	TMap<btRigidBody*, AActor*> BodyToActor;
	TMap<AActor*, btRigidBody*> ActorToBody;
	
	// server's list of input Cbuffers for each pawn
	TMap<AActor*, TUniquePtr<TMpscQueue<FBulletPlayerInput>>> InputBuffers;
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void SendInputToServer(AActor* actor, FBulletPlayerInput input);

	

	TCircularBuffer<FBulletSimulationState> StateHistoryBuffer = TCircularBuffer<FBulletSimulationState>(128);
	uint32 CurrentHistoryIndex = 0;
	uint32 HistoryCount = 0;
	
	// Simple helper methods
	void AddHistoryEntry(const FBulletSimulationState& Entry)
	{
		StateHistoryBuffer[CurrentHistoryIndex] = Entry;
		CurrentHistoryIndex = StateHistoryBuffer.GetNextIndex(CurrentHistoryIndex);
		HistoryCount = FMath::Min(HistoryCount + 1, (uint32)StateHistoryBuffer.Capacity());
	}

	const FBulletSimulationState* FindHistoryEntryByFrame(int32 FrameNumber) const
	{
		if (HistoryCount == 0) return nullptr;
    
		uint32 Index = CurrentHistoryIndex;
		for (uint32 i = 0; i < HistoryCount; ++i)
		{
			Index = StateHistoryBuffer.GetPreviousIndex(Index);
			if (StateHistoryBuffer[Index].FrameNumber == FrameNumber)
				return &StateHistoryBuffer[Index];
		}
		return nullptr;
	}
	const FBulletSimulationState* GetLatestHistoryEntry() const
	{
		if (HistoryCount == 0)
			return nullptr;
		return &StateHistoryBuffer[StateHistoryBuffer.GetPreviousIndex(CurrentHistoryIndex)];
	}

	UFUNCTION(BlueprintCallable)
	int SumMyInputBuffers() const
	{
		int TotalEntries = 0;
		for (const auto& Pair : InputBuffers)
		{
			// TotalEntries += Pair.Value.Capacity();
			TotalEntries += 0;
		}
		return TotalEntries;
	}

	UFUNCTION(BlueprintCallable)
	FBulletSimulationState GetCurrentState() const
	{
		FBulletSimulationState thisState;
		thisState.FrameNumber = CurrentFrameNumber;
		// construct and add all object states
		for (const auto& tuple : BodyToActor)
		{
			FBulletObjectState os;
			btRigidBody* body = tuple.Key;

			os.Transform = BulletHelpers::ToUE(body->getWorldTransform(), GetActorLocation());
			os.Velocity = BulletHelpers::ToUEDir(body->getLinearVelocity(), true);
			os.AngularVelocity = BulletHelpers::ToUEDir(body->getAngularVelocity(), true);
	
			thisState.ObjectStates.Add(os);
		}
		return thisState;
	}

	// find a way to send map of actor*s to inputs
	UFUNCTION(BlueprintCallable, NetMulticast, Unreliable)
	// void MC_SendStateToClients(FBulletSimulationState state, TMap<AActor*, FBulletPlayerInput> LastActorInputs);
	// void MC_SendStateToClients(FBulletSimulationState state, TArray<TPair<AActor*, FBulletPlayerInput>> inputs);
	void MC_SendStateToClients(FBulletSimulationState State, const TArray<AActor*>& InputActors, const TArray<FBulletPlayerInput>& PlayerInputs);

	// sets the state of the physics world, called by
	// BasicPhysicsPawn.resim()
	// only used on the client
	void SetState(FBulletSimulationState state)
	{
		if (!HasAuthority())
		{
			// set to state
			return;
		}
	}
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

	// use this to completely remove the body and references to it
	// E.g. when destroying an actor
	void DestroyRigidBody(btRigidBody* rigidbody)
	{

		BodyToActor.Remove(rigidbody);
		ActorToBody.Remove(*BodyToActor.Find(rigidbody));
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
	// new function, no ufunction macro because btRigidBody can't be in BP
	btRigidBody* AddRigidBodyAndReturn(AActor* Body, float Friction, float Restitution, float mass);
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
