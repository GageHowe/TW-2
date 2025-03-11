#include "BasicPhysicsPawn.h"

#include "TestActor.h"
#include "Kismet/GameplayStatics.h"

ABasicPhysicsPawn::ABasicPhysicsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
	RootComponent = StaticMesh;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	Camera->SetupAttachment(StaticMesh);
	SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
}

void ABasicPhysicsPawn::BeginPlay()
{
	Super::BeginPlay();
	
	TArray<AActor*> worlds;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), ATestActor::StaticClass(), worlds);
	world = Cast<ATestActor>(worlds[0]); // this will crash if no world is present
										// if you ain't crashed, the reference is valid
	world->AddRigidBody(this, 0.2, 0.2, 1);
}

void ABasicPhysicsPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void ABasicPhysicsPawn::SetupPlayerInputComponent(class UInputComponent* ThisInputComponent)
{
	Super::SetupPlayerInputComponent(ThisInputComponent);
	InputComponent->BindAxis(TEXT("MoveForward"), this, &ABasicPhysicsPawn::SetForwardInput);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &ABasicPhysicsPawn::SetRightInput);
	InputComponent->BindAxis(TEXT("MoveUp"), this, &ABasicPhysicsPawn::SetUpInput);
}
