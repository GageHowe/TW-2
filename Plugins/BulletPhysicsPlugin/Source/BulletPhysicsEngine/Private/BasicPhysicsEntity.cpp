#include "BasicPhysicsEntity.h"
#include "TestActor.h"
#include "Kismet/GameplayStatics.h"

// This is for replicated entities that are NOT pawns, e.g. projectiles

ABasicPhysicsEntity::ABasicPhysicsEntity()
{
	PrimaryActorTick.bCanEverTick = true;
	bAsyncPhysicsTickEnabled = true;
	bReplicates = true;
	SetReplicatingMovement(false);

	// fix/deprecate this?
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	RootComponent = StaticMesh;

	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
}

void ABasicPhysicsEntity::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> worlds;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATestActor::StaticClass(), worlds);
	world = Cast<ATestActor>(worlds[0]);	// this will crash if no bullet world is present
											// if you ain't crashed, the reference is valid
	BulletWorld = world;
	MyRigidBody = world->AddRigidBodyAndReturn(this, 0.2, 0.2, 1);
	if (!MyRigidBody) { GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING RigidBody ptr is null")); }

	BulletWorld->ActorToBody.Add(this, MyRigidBody);
	BulletWorld->BodyToActor.Add(MyRigidBody, this);
}

void ABasicPhysicsEntity::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ABasicPhysicsEntity::AsyncPhysicsTickActor(float DeltaTime, float SimTime)
{
	Super::AsyncPhysicsTickActor(DeltaTime, SimTime);
}
