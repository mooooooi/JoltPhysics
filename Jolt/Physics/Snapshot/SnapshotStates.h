#pragma once

#include<Jolt/Math/Vec3.h>
#include<Jolt/Math/Quat.h>
#include<Jolt/Geometry/Sphere.h>
#include<Jolt/Physics/Body/BodyID.h>
#include<Jolt/Physics/StateRecorder.h>
#include<Jolt/Physics/Snapshot/BlobBuilder.h>

#include <Jolt/Physics/Body/BodyPair.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/SubShapeIDPair.h>
#include <Jolt/Physics/Character/CharacterID.h>
#include <Jolt/Physics/Character/CharacterGroundState.h>

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

struct CachedContactPointState
{
    Float3 position1;
    Float3 position2;
    float nonPenetrationLambda;
    float frictionLambda[2];
};

struct CachedManifoldState
{
    Float3 contactNormal;
    BlobArray<CachedContactPointState> contactPoints;
};

struct ManifoldKeyValueState
{
    SubShapeIDPair key;
    CachedManifoldState value;
};

struct CachedBodyPairState
{
    Float3 deltaPosition;
    Float3 deltaRotation;
    BlobArray<ManifoldKeyValueState> manifolds;
};

struct BodyPairKeyValueState
{
    BodyPair key;
    CachedBodyPairState value;
};

struct ManifoldCacheState
{
    BlobArray<BodyPairKeyValueState> bodyPairs;
    BlobArray<SubShapeIDPair> ccdManifolds;
};

struct ContactConstraintState
{
    ManifoldCacheState manifold;
};

struct PhysicsSystemState
{
	EStateRecorderState flags;
    GlobalState global;
    BlobArray<BodyState> bodies;
    ContactConstraintState contacts;
};

struct CharacterBaseState
{
    EGroundState groundState;
    BodyID groundBodyID;
    SubShapeID groundBodySubShapeID;
    Float3 groundPosition;
    Float3 groundNormal;
    Float3 groundVelocity;
};

struct CharacterVirtualContactKeyState
{
    BodyID bodyB;
    CharacterID characterIDB;
    SubShapeID subShapeIDB;
};

struct CharacterVirtualContactState
{
    CharacterVirtualContactKeyState key;
    Float3 position;
    Float3 linearVelocity;
    Float3 contactNormal;
    Float3 surfaceNormal;
    float distance;
    float fraction;
    EMotionType motionTypeB;
    bool isSensorB;
    bool hadCollision;
    bool wasDiscarded;
    bool canPushCharacter;
};

struct CharacterVirtualState
{
    CharacterBaseState base;
    Float3 position;
    Float4 rotation;
    Float3 linearVelocity;
    float lastDeltaTime;
    bool maxHitsExceeded;
    BlobArray<CharacterVirtualContactState> contacts;
};


JPH_NAMESPACE_END