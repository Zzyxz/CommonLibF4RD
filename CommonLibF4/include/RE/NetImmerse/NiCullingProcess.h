#pragma once

#include "RE/NetImmerse/NiFrustum.h"
#include "RE/NetImmerse/NiFrustumPlanes.h"
#include "RE/NetImmerse/NiObject.h"
#include "RE/NetImmerse/NiVisibleArray.h"

namespace RE
{
	class NiAVObject;

	class __declspec(novtable) NiCullingProcess
	{
	public:
		static constexpr auto RTTI{ RTTI::NiCullingProcess };
		static constexpr auto VTABLE{ VTABLE::NiCullingProcess };
		static constexpr auto Ni_RTTI{ Ni_RTTI::NiCullingProcess };

		virtual const NiRTTI* GetRTTI() const { return nullptr; }                                                  // 00
		virtual const NiNode* IsNode() const { return nullptr; }                                                   // 01
		virtual NiNode* IsNode() { return nullptr; }                                                               // 02
		virtual NiSwitchNode* IsSwitchNode() { return nullptr; }                                                   // 03
		virtual BSFadeNode* IsFadeNode() { return nullptr; }                                                       // 04
		virtual BSMultiBoundNode* IsMultiBoundNode() { return nullptr; }                                           // 05
		virtual BSGeometry* IsGeometry() { return nullptr; }                                                       // 06
		virtual NiTriStrips* IsTriStrips() { return nullptr; }                                                     // 07
		virtual BSTriShape* IsTriShape() { return nullptr; }                                                       // 08
		virtual BSDynamicTriShape* IsDynamicTriShape() { return nullptr; }                                         // 09
		virtual BSSegmentedTriShape* IsSegmentedTriShape() { return nullptr; }                                     // 0A
		virtual BSSubIndexTriShape* IsSubIndexTriShape() { return nullptr; }                                       // 0B
		virtual NiGeometry* IsNiGeometry() { return nullptr; }                                                     // 0C
		virtual NiTriBasedGeom* IsNiTriBasedGeom() { return nullptr; }                                             // 0D
		virtual NiTriShape* IsNiTriShape() { return nullptr; }                                                     // 0E
		virtual NiParticles* IsParticlesGeom() { return nullptr; }                                                 // 0F
		virtual NiParticleSystem* IsParticleSystem() { return nullptr; }                                           // 10
		virtual BSLines* IsLinesGeom() { return nullptr; }                                                         // 11
		virtual NiLight* IsLight() { return nullptr; }                                                             // 12
		virtual bhkNiCollisionObject* IsBhkNiCollisionObject() { return nullptr; }                                 // 13
		virtual bhkBlendCollisionObject* IsBhkBlendCollisionObject() { return nullptr; }                           // 14
		virtual bhkRigidBody* IsBhkRigidBody() { return nullptr; }                                                 // 15
		virtual bhkLimitedHingeConstraint* IsBhkLimitedHingeConstraint() { return nullptr; }                       // 16
		virtual bhkNPCollisionObject* IsbhkNPCollisionObject() { return nullptr; }                                 // 17
		virtual ~NiCullingProcess() = default;                                                                      // 18
		virtual void Process(NiAVObject*) {}                                                                        // 19
		virtual void Process(const NiCamera*, NiAVObject*, NiVisibleArray*) {}                                     // 1A
		virtual void AppendVirtual(BSGeometry*) {}                                                                  // 1B

		// members
		bool useVirtualAppend;       // 008
		NiVisibleArray* visibleSet;  // 010
		NiCamera* camera;            // 018
		NiFrustum frustum;            // 020
		NiFrustumPlanes planes;       // 03C
		NiFrustumPlanes customCullPlanes;  // 0AC
		bool cameraRelatedUpdates;   // 11C
		bool updateAccumulateFlag;   // 11D
		bool ignorePreprocess;       // 11E
		bool bCustomCullPlanes;      // 11F
	};
	static_assert(sizeof(NiCullingProcess) == 0x120);
	static_assert(offsetof(NiCullingProcess, visibleSet) == 0x10);
	static_assert(offsetof(NiCullingProcess, frustum) == 0x20);
	static_assert(offsetof(NiCullingProcess, planes) == 0x3C);
}
