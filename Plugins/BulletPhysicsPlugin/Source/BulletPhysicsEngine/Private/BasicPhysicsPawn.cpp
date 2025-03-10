#include "BasicPhysicsPawn.h"

ABasicPhysicsPawn::ABasicPhysicsPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	
}


void ABasicPhysicsPawn::BeginPlay()
{
	Super::BeginPlay();
	
}

void ABasicPhysicsPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
}

void ABasicPhysicsPawn::SetupPlayerInputComponent(class UInputComponent* InputComponent)
{
	Super::SetupPlayerInputComponent(InputComponent);
	
}

