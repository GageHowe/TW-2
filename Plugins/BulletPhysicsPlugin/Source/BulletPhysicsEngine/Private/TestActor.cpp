// Fill out your copyright notice in the Description page of Project Settings.


#include "TestActor.h"

#include "BasicPhysicsPawn.h"
#include "TWPlayerController.h"
#include "LevelInstance/LevelInstanceTypes.h"
#include "Types/AttributeStorage.h"

// Sets default values
ATestActor::ATestActor()
{
	PrimaryActorTick.bCanEverTick = true;
	bAsyncPhysicsTickEnabled = true;
	bReplicates = true;
	bAlwaysRelevant = true;
	bOnlyRelevantToOwner = false;
	SetReplicatingMovement(false);
}

// Called when the game starts or when spawned
void ATestActor::BeginPlay()
{
	Super::BeginPlay();

	BtCollisionConfig = new btDefaultCollisionConfiguration();
	BtCollisionDispatcher = new btCollisionDispatcher(BtCollisionConfig);
	BtBroadphase = new btDbvtBroadphase();
	mt = new btSequentialImpulseConstraintSolver;
	mt->setRandSeed(1234);
	BtConstraintSolver = mt;
	BtWorld = new btDiscreteDynamicsWorld(BtCollisionDispatcher, BtBroadphase, BtConstraintSolver, BtCollisionConfig);
	BtWorld->setGravity(btVector3(0, 0, 0));
	
	// Gravity vector in our units (1=1cm)
	//getSimulationIslandManager()->setSplitIslands(false);
}

// Called every frame
void ATestActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	// Dep. Physics networking logic is now in async physics tick
}

void ATestActor::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);

	StepPhysics(DeltaTime, 1);
	randvar = mt->getRandSeed();
	CurrentState = GetCurrentState();
	if (HasAuthority())
	{
		// consume input
		for (auto& Pair : InputBuffers)
		{
			AActor* Actor = Pair.Key;
			auto InputBuf = *Pair.Value;
			if (!InputBuf.IsEmpty())
			{
				auto pawn = Cast<ABasicPhysicsPawn>(Actor);
				pawn->ApplyInputs(InputBuf.Get(0));
			}
		}
		
		// send state
		TArray<AActor*> InputActorArray;
		TArray<FTWPlayerInput> InputArray;

		// Loop through each actor and get their most recent input
		for (const auto& Pair : InputBuffers)
		{
			AActor* Actor = Pair.Key;
			TWRingBuffer<FTWPlayerInput>* Buffer = Pair.Value;
    
			InputActorArray.Add(Actor);
    
			// Get the newest input (or default if empty)
			if (!Buffer->IsEmpty())
			{
				InputArray.Add(Buffer->GetNewest());
			}
			else
			{
				InputArray.Add(FTWPlayerInput()); // Add default input
			}
		}


		// TODO: investigate filtering CurrentState by proximity/look direction/etc to client to save bandwidth
		// TODO: Don't send inputs of actors/pawns not being controlled
		if (tock) {
			MC_SendStateToClients(CurrentState, InputActorArray, InputArray);
			tock = false;
		} else {
			tock = true;
		}
		
	} else // if client
	{
		StateHistory.Push(CurrentState);
	}
}


void ATestActor::SendInputToServer(AActor* actor, FTWPlayerInput input)
{
	// If this actor doesn't have an input buffer yet, create one
	if (!InputBuffers.Contains(actor))
	{
		// Create a new buffer on the heap and store its pointer in the map
		TWRingBuffer<FTWPlayerInput>* newBuffer = new TWRingBuffer<FTWPlayerInput>(64);
		InputBuffers.Add(actor, newBuffer);
	}
    
	// Access the pointer and use -> to call Push
	InputBuffers[actor]->Push(input);
}

void ATestActor::MC_SendStateToClients_Implementation(FBulletSimulationState ServerState, const TArray<AActor*>& InputActors,
	const TArray<FTWPlayerInput>& PlayerInputs)
{
	if (!HasAuthority())
	{
		// get player controller
		APlayerController* PC = GetWorld()->GetFirstPlayerController();
		ATWPlayerController* TWPC = Cast<ATWPlayerController>(PC); if (!TWPC) return;

		int framesToRewind = FMath::RoundToInt(TWPC->RoundTripDelay / FixedDeltaTime);
		FBulletSimulationState HistoricState = StateHistory.Get(framesToRewind);

		// cache current state
		CurrentState = GetCurrentState();
		
		// set local state to server states for resim
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
		
		// simulate up to prediction
		for (int i = 0; i < framesToRewind; i++)
		{
			// for (int j = 0; j < InputActors.Num(); j++)
			// { // apply other actors' inputs
			// 	Cast<ABasicPhysicsPawn>(InputActors[j])->ApplyInputs(PlayerInputs[j]);
			// }
			
			FTWPlayerInput PastInput = LocalInputBuffer.Get(framesToRewind-i);
			LocalPawn->ApplyInputs(PastInput);
			// GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Got and applied input: %f, %f, %f"), PastInput.MovementInput.X, PastInput.MovementInput.Y, PastInput.MovementInput.Z));
			// UE_LOG(LogTemp, Warning, TEXT("Applied input: %f, %f, %f from position %i"), PastInput.MovementInput.X, PastInput.MovementInput.Y, PastInput.MovementInput.Z, framesToRewind-i );

			// step forward
			StepPhysics(FixedDeltaTime, 1);
		}
		debugShouldResim = false;
		//
		// // Reapply/interpolate states of objects that were "close enough"
		// for (const auto& objState : CurrentState.ObjectStates)
		// {
		//     if (ActorToBody.Contains(objState.Actor))
		//     {
		//         // Find the corresponding state in ServerState
		//         for (const auto& serverObjState : ServerState.ObjectStates)
		//         {
		//             if (serverObjState.Actor == objState.Actor)
		//             {
		//                 // Calculate the difference
		//                 FBulletObjectState Diff = serverObjState - objState;
		//                 
		//                 // If small enough difference, interpolate
		//                 if (Diff.Transform.GetLocation().Length() < 10.0f)
		//                 {
		//                     btRigidBody* body = ActorToBody[objState.Actor];
		//                     
		//                     // Simple 50/50 blend between current state and server state
		//                     FVector blendedLocation = (objState.Transform.GetLocation() + serverObjState.Transform.GetLocation()) * 0.5f;
		//                     FQuat currentQuat = objState.Transform.GetRotation();
		//                     FQuat serverQuat = serverObjState.Transform.GetRotation();
		//                     FQuat blendedQuat = FQuat::Slerp(currentQuat, serverQuat, 0.5f);
		//                     
		//                     FVector blendedVelocity = (objState.Velocity + serverObjState.Velocity) * 0.5f;
		//                     FVector blendedAngularVelocity = (objState.AngularVelocity + serverObjState.AngularVelocity) * 0.5f;
		//                     
		//                     // Create blended transform
		//                     FTransform blendedTransform(blendedQuat, blendedLocation, objState.Transform.GetScale3D());
		//                     
		//                     // Apply the blended state
		//                     body->clearForces();
		//                     body->setWorldTransform(BulletHelpers::ToBt(blendedTransform, GetActorLocation()));
		//                     body->setLinearVelocity(BulletHelpers::ToBtDir(blendedVelocity, true));
		//                     body->setAngularVelocity(BulletHelpers::ToBtDir(blendedAngularVelocity, true));
		//                 }
		//                 break;
		//             }
		//         }
		//     }
		// }
	}
}

void ATestActor::SR_debugResim_Implementation()
{
	debugShouldResim = true;
}

































void ATestActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

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

void ATestActor::AddRigidBody(AActor* actor, float Friction, float Restitution, float mass)
{
	btRigidBody* rb = AddRigidBody(actor, GetCachedDynamicShapeData(actor, mass), Friction, Restitution);
	BodyToActor.Add(rb, actor);
	ActorToBody.Add(actor, rb);
	// add input buffer?
}

btRigidBody* ATestActor::AddRigidBodyAndReturn(AActor* Body, float Friction, float Restitution, float mass)
{
	btRigidBody* rb = AddRigidBody(Body, GetCachedDynamicShapeData(Body, mass), Friction, Restitution);
	BodyToActor.Add(rb, Body);
	ActorToBody.Add(Body, rb);
	// add input buffer?
	return rb;
}

void ATestActor::UpdatePlayertransform(AActor* player, int ID)
{
		BtWorld->getCollisionObjectArray()[ID]->setWorldTransform(BulletHelpers::ToBt(player->GetActorTransform(), GetActorLocation()));
}

void ATestActor::AddImpulse( int ID, FVector Impulse, FVector Location)
{
	BtRigidBodies[ID]->applyImpulse(BulletHelpers::ToBtDir(Impulse, true), BulletHelpers::ToBtPos(Location, GetActorLocation()));
}

// these use IDs instead of references and shouldn't be used

void ATestActor::AddForce(int ID, FVector Force, FVector Location)
{
	BtRigidBodies[ID]->applyForce(BulletHelpers::ToBtDir(Force, true), BulletHelpers::ToBtPos(Location, GetActorLocation()));
}

void ATestActor::AddCentralForce(int ID, FVector Force)
{
	BtRigidBodies[ID]->applyCentralForce(BulletHelpers::ToBtDir(Force, true));
}

void ATestActor::AddTorque(int ID, FVector Torque)
{
	BtRigidBodies[ID]->applyTorque(BulletHelpers::ToBtDir(Torque, true));
}

void ATestActor::AddTorqueImpulse(int ID, FVector Torque)
{
	BtRigidBodies[ID]->applyTorqueImpulse(BulletHelpers::ToBtDir(Torque, true));
}

void ATestActor::GetVelocityAtLocation(int ID, FVector Location, FVector&Velocity)
{
	if (BtRigidBodies[ID]) {
		Velocity = BulletHelpers::ToUEPos(BtRigidBodies[ID]->getVelocityInLocalPoint(BulletHelpers::ToBtPos(Location, GetActorLocation())), FVector(0));
	}
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
	// Very important! Otherwise there's a gap between 
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
	GEngine->AddOnScreenDebugMessage(        -1,          // Key: Unique identifier for the message, -1 to display multiple times
		5.0f,        // Duration: Time in seconds the message stays on screen
		FColor::Red, // Color: Text color, e.g., FColor::Red, FColor::Green
		TEXT("A body was created") // Message: The string to display
	);
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
	if (BtRigidBodies[ID]) {
		// Debug output
		UE_LOG(LogTemp, Warning, TEXT("Raw Bullet Velocity: %f,%f,%f"), 
			BtRigidBodies[ID]->getLinearVelocity().x(),
			BtRigidBodies[ID]->getLinearVelocity().y(),
			BtRigidBodies[ID]->getLinearVelocity().z());

		transforms = BulletHelpers::ToUE(BtRigidBodies[ID]->getWorldTransform(), GetActorLocation());
		Velocity = BulletHelpers::ToUEDir(BtRigidBodies[ID]->getLinearVelocity(), true);
		AngularVelocity = BulletHelpers::ToUEDir(BtRigidBodies[ID]->getAngularVelocity(), true);
		Force = BulletHelpers::ToUEDir(BtRigidBodies[ID]->getTotalForce(), true);
	}
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
	BtWorld->setGravity(btVector3(0, 0, 0));
	BtBroadphase->resetPool(BtCollisionDispatcher);
	BtConstraintSolver->reset();
	for (int i = 0; i < BtRigidBodies.Num(); i++)
	{
		BtWorld->addRigidBody(BtRigidBodies[i]);
	}
}
