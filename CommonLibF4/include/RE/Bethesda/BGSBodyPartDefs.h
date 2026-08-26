#pragma once

#include "RE/Bethesda/BSFixedString.h"

namespace RE::BGSBodyPartDefs
{
	enum class CAUSE_OF_DEATH : std::uint32_t
	{
		kNone = static_cast<std::uint32_t>(-1),
		kExplosion = 0,
		kGun = 1,
		kBluntWeapon = 2,
		kHandToHand = 3,
		kObjectImpact = 4,
		kPoison = 5,
		kDecapitation = 6
	};

	enum class LIMB_ENUM : std::int32_t
	{
		kNone = -1,
		kTorso = 0,
		kHead1 = 1,
		kEye1 = 2,
		kLookAt1 = 3,
		kFlyGrab = 4,
		kHead2 = 5,
		kLeftArm1 = 6,
		kLeftArm2 = 7,
		kRightArm1 = 8,
		kRightArm2 = 9,
		kLeftLeg1 = 10,
		kLeftLeg2 = 11,
		kLeftLeg3 = 12,
		kRightLeg1 = 13,
		kRightLeg2 = 14,
		kRightLeg3 = 15,
		kBrain = 16,
		kWeapon = 17,
		kRoot = 18,
		kCom = 19,
		kPelvis = 20,
		kCamera = 21,
		kOffsetRoot = 22,
		kLeftFoot = 23,
		kRightFoot = 24,
		kFaceTargetSource = 25
	};

	struct HitReactionData
	{
	public:
		// members
		BSFixedString chainStart;  // 00
		BSFixedString chainEnd;    // 08
		BSFixedString variableX;   // 10
		BSFixedString variableY;   // 18
		BSFixedString variableZ;   // 20
	};
	static_assert(sizeof(HitReactionData) == 0x28);
}
