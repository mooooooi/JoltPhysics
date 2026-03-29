#pragma once

JPH_NAMESPACE_BEGIN

enum class EGroundState
{
    OnGround,						///< Character is on the ground and can move freely.
    OnSteepGround,					///< Character is on a slope that is too steep and can't climb up any further. The caller should start applying downward velocity if sliding from the slope is desired.
    NotSupported,					///< Character is touching an object, but is not supported by it and should fall. The GetGroundXXX functions will return information about the touched object.
    InAir,							///< Character is in the air and is not touching anything.
};

JPH_NAMESPACE_END
