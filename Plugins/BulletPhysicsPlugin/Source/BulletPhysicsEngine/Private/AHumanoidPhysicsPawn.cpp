#include "AHumanoidPhysicsPawn.h"

void AHumanoidPhysicsPawn::BeginPlay()
{
	Super::BeginPlay();
	// yRigidBody->addConstraintRef()
}

void AHumanoidPhysicsPawn::ApplyInputs(const FTWPlayerInput& input) const
{
	Super::ApplyInputs(input);
	// walk around
}
