#include "ASpaceshipPhysicsPawn.h"

#include "BansheeBomb.h"

void ASpaceshipPhysicsPawn::BeginPlay()
{
    Super::BeginPlay();
    MyRigidBody->setDamping(0.5,0.5);
}

void ASpaceshipPhysicsPawn::ApplyInputs(const FTWPlayerInput& input)
{
    if (!MyRigidBody) { 
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING RigidBody ptr is null 2")); 
        return; 
    }

    // simple shooting logic
    if (bool firing = input.BoostInput)
    {
        // TODO: world->shootThing()
        world->shootThing(projectile1ToSpawn, {0,0,0}, {0,0,0}, this->GetActorLocation(), this);
    }
    
    // Apply movement force in local space
    btVector3 localForce = btVector3(
        input.MovementInput.X,
        input.MovementInput.Y,
        input.MovementInput.Z
    ) * 50.0f;
    btQuaternion currentRotation = MyRigidBody->getWorldTransform().getRotation();
    btVector3 worldForce = quatRotate(currentRotation, localForce);
    MyRigidBody->applyForce(worldForce, btVector3(0, 0, 0));
    

    // Define rotation torque in local space
    btVector3 localTorque = btVector3(
        input.RollRight * -1,
        input.TurnUp,
        input.TurnRight
    ) * 0.1f;

    // Manually transform local torque to world space using the current rotation
    btVector3 worldTorque = quatRotate(currentRotation, localTorque);

    // Apply the transformed world-space torque using applyTorque
    MyRigidBody->applyTorque(worldTorque);
}

void ASpaceshipPhysicsPawn::shootProjectile(TSubclassOf<ABasicPhysicsEntity> projectile, FVector direction,
    FVector inheritedVelocity)
{
    
}


