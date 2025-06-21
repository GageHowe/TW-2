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
class ABasicPhysicsEntity;
class ABasicPhysicsPawn; // forward declaration
#include "BlueprintEditor.h"
#include "HairStrandsInterface.h"
#include "Engine/PackageMapClient.h"
#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"
#include "helpers.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameState.h"
#include "TWRingBuffer.h"
#include "TestActor.generated.h"

UCLASS()
class BULLETPHYSICSENGINE_API ATestActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ATestActor();

	// for interpolation/correction
	UPROPERTY()
	FBulletObjectState LocalPlayerError;
	UPROPERTY()
	bool bHasLocalPlayerError = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FBulletSimulationState CurrentState;

	// Server-only state buffer
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TWRingBuffer<FBulletSimulationState> StateHistory;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ABasicPhysicsPawn* LocalPawn; // this is set on PossessedBy

	// bool tock = false; // for syncing every other tick (30/s)

	// Frames
	// UPROPERTY(EditAnywhere, BlueprintReadWrite)
	// int32 CurrentFrameNumber = 0;	// Global current frame number
	const float FixedDeltaTime = 1.0f / 60.0f;

	// bidirectional map to manage an actor's rigidbody
	TMap<btRigidBody*, AActor*> BodyToActor;
	TMap<AActor*, btRigidBody*> ActorToBody;

	// the error clients must correct over an interpolation period
	// should go down to zero, which means no error
	TMap<AActor*, FBulletObjectState> InterpDeltas;
	
	// server's list of input Cbuffers for each pawn
	// TMap<AActor*, TSharedPtr<TMpscQueue<FTWPlayerInput>>> InputBuffers;
	TMap<AActor*, TWRingBuffer<FTWPlayerInput>*> InputBuffers;
	
	TWRingBuffer<FTWPlayerInput> LocalInputBuffer = TWRingBuffer<FTWPlayerInput>(64);

	FBulletObjectState* InterpolationError;
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void SendInputToServer(AActor* actor, FTWPlayerInput input);

	UFUNCTION(BlueprintCallable)
	FBulletSimulationState GetCurrentState() const
	{
		FBulletSimulationState thisState;
		// thisState.FrameNumber = CurrentFrameNumber;
		thisState.CurrentTime = GetWorld()->GetTimeSeconds();
		// construct and add all object states
		for (const auto& tuple : BodyToActor)
		{
			FBulletObjectState os;
			btRigidBody* body = tuple.Key;
			os.Actor = tuple.Value;
			os.Transform = BulletHelpers::ToUE(body->getWorldTransform(), GetActorLocation());
			os.Velocity = BulletHelpers::ToUEDir(body->getLinearVelocity(), true);
			os.AngularVelocity = BulletHelpers::ToUEDir(body->getAngularVelocity(), true);
	
			thisState.ObjectStates.Add(os);
		}
		return thisState;
	}
	
	void SetLocalState(FBulletSimulationState ServerState)
	{
		if (HasAuthority()) {
			GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, TEXT("Tried to run SetLocalState on server"));
			return;
		} else {

			for (const auto& objState : ServerState.ObjectStates)
			{
				if (ActorToBody.Contains(objState.Actor))
				{
					btRigidBody* body = ActorToBody[objState.Actor];

					body->clearForces();
					
					body->setWorldTransform(BulletHelpers::ToBt(objState.Transform, GetActorLocation()));
					body->setLinearVelocity(BulletHelpers::ToBtDir(objState.Velocity, true));
					body->setAngularVelocity(BulletHelpers::ToBtDir(objState.AngularVelocity, true));
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("SetLocalState: Actor not found in ActorToBody map"));
				}
			}
		}
	}

	// send state and actors' last input
	UFUNCTION(BlueprintCallable, NetMulticast, Unreliable)
	void MC_SendStateToClients(FBulletSimulationState ServerState, const TArray<AActor*>& InputActors, const TArray<FTWPlayerInput>& PlayerInputs);

	UFUNCTION(Server, Reliable)
	void shootThing(TSubclassOf<ABasicPhysicsEntity> projectileClass, FRotator direction, FVector inheritedVelocity, FVector location, AActor* owner2);
	
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
	void ApplyLocalPlayerErrorCorrection(float DeltaTime);
	virtual void AsyncPhysicsTickActor(float DeltaTime, float SimTime) override;

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


