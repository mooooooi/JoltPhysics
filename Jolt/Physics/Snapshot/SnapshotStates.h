#pragma once

#include<Jolt/Math/Vec3.h>
#include<Jolt/Math/Quat.h>
#include<Jolt/Geometry/Sphere.h>
#include<Jolt/Physics/Body/BodyID.h>
#include<Jolt/Physics/StateRecorder.h>
#include<Jolt/Physics/Snapshot/BlobBuilder.h>

JPH_NAMESPACE_BEGIN

struct GlobalState
{
    float previousStepDeltaTime;
    Float3 gravity;
};

struct MotionPropertiesState
{
    Float3 linearVelocity;
    Float3 angularVelocity;
    Float3 force;
    Float3 torque;
#ifdef JPH_DOUBLE_PRECISION
	Double3 sleepTestOffset;
#endif // JPH_DOUBLE_PRECISION
    JPH::Sphere sleepTestSpheres[3];
    float sleepTestTimer;
    bool allowSleeping;
};

struct BodyState
{
    BodyID id;
    bool isActive;
    Float3 position;
    Float4 rotation;
    MotionPropertiesState motionProperties;
};

struct PhysicsSystemState
{
	EStateRecorderState flags;
    GlobalState global;
    BlobArray<BodyState> bodies;
};



JPH_NAMESPACE_END
