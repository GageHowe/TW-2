#include "ASpaceshipPhysicsPawn.h"

void ASpaceshipPhysicsPawn::ApplyInputs(const FTWPlayerInput& input) const
{
    if (!MyRigidBody) { 
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("WARNING RigidBody ptr is null 2")); 
        return; 
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
    
    // Get the target rotation from controller input
    FQuat UEQuat = input.RotationInput.Quaternion();
    btQuaternion targetRotation(UEQuat.X, UEQuat.Y, UEQuat.Z, UEQuat.W);
    
    // Calculate the current angular velocity
    btVector3 currentAngVel = MyRigidBody->getAngularVelocity();
    float currentAngVelMagnitude = currentAngVel.length();
    
    // Define maximum allowed angular velocity (in radians per second)
    const float MAX_ANGULAR_VELOCITY = 3.0f; // Adjust this value as needed
    
    // Check if we need to apply rotation
    btTransform currentTransform = MyRigidBody->getWorldTransform();
    
    // Option 1: Smooth rotation with angular velocity limit
    if (currentAngVelMagnitude < MAX_ANGULAR_VELOCITY) {
        // Calculate angle between current and target rotation
        btScalar angle = currentRotation.angle(targetRotation);
        
        // Apply rotation only if the difference is significant
        if (angle > 0.01f) {
            // Scale the rotation amount based on how far we are from the target
            // The closer to the max velocity, the less we apply
            float velocityFactor = 1.0f - (currentAngVelMagnitude / MAX_ANGULAR_VELOCITY);
            float rotationStrength = 0.1f * velocityFactor; // Adjust this value for faster/slower rotation
            
            // Calculate a blended rotation (slerp between current and target)
            btQuaternion newRotation = currentRotation.slerp(targetRotation, rotationStrength);
            currentTransform.setRotation(newRotation);
            MyRigidBody->setWorldTransform(currentTransform);
        }
    }
    else {
        // We're at max angular velocity, so just damp the current rotation
        MyRigidBody->setAngularVelocity(currentAngVel * 0.95f);
    }
    
    // // Option 2: Direct velocity clamping (alternative approach)
    // // This can be used instead of or in addition to the above
    // if (currentAngVelMagnitude > MAX_ANGULAR_VELOCITY) {
    //     // Normalize and scale back to maximum
    //     btVector3 newAngVel = currentAngVel.normalized() * MAX_ANGULAR_VELOCITY;
    //     MyRigidBody->setAngularVelocity(newAngVel);
    // }
}
