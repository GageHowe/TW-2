#include "AHumanoidPhysicsPawn.h"

void AHumanoidPhysicsPawn::BeginPlay()
{
	Super::BeginPlay();
	// yRigidBody->addConstraintRef()
}

void AHumanoidPhysicsPawn::ApplyInputs(const FTWPlayerInput& input)
{
	Super::ApplyInputs(input);
	// walk around
}
