#include "BasicPhysicsPawn.h"

ABasicPhysicsPawn::ABasicPhysicsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlayerMesh"));
	RootComponent = StaticMesh;
	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("PlayerCamera"));
	Camera->SetupAttachment(StaticMesh);
}

void ABasicPhysicsPawn::BeginPlay()
{
	Super::BeginPlay();
	
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
