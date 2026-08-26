#pragma once

#include "RE/Bethesda/BSPointerHandle.h"
#include "RE/Bethesda/ReferenceEffectController.h"
#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	class ActiveEffect;

	class __declspec(novtable) ActiveEffectReferenceEffectController :
		public ReferenceEffectController  // 00
	{
	public:
		static constexpr std::size_t OG_SIZE = 0x10;
		static constexpr std::size_t AE_SIZE = 0x20;
		static constexpr auto RTTI{ RTTI::ActiveEffectReferenceEffectController };
		static constexpr auto VTABLE{ VTABLE::ActiveEffectReferenceEffectController };

		[[nodiscard]] static constexpr std::size_t GetRuntimeSize(const REL::Version& a_version) noexcept
		{
			return REL::Relocate(a_version, OG_SIZE, AE_SIZE);
		}

		[[nodiscard]] static std::size_t GetRuntimeSize() noexcept
		{
			return REL::Relocate(OG_SIZE, AE_SIZE);
		}

		[[nodiscard]] ActiveEffect* GetActiveEffect() const noexcept { return effect; }

		// override (ReferenceEffectController)
		void HandleEvent(const BSFixedString& a_event) override;                 // 01
		float GetElapsedTime() override;                                          // 02
		float GetScale() override;                                                // 03
		void SwitchAttachedRoot(NiNode* a_root, NiNode* a_attachRoot) override;  // 04
		const NiPoint3& GetSourcePosition() override;                              // 05
		bool GetUseSourcePosition() override;                                     // 06
		bool GetNoInitialFlare() override;                                        // 07
		bool GetEffectPersists() override;                                        // 08
		bool GetGoryVisuals() override;                                           // 09
		void RemoveHitEffect(ReferenceEffect* a_refEffect) override;              // 0A
		TESObjectREFR* GetTargetReference() override;                             // 0B
		BGSArtObject* GetHitEffectArt() override;                                 // 0C
		TESEffectShader* GetHitEffectShader() override;                           // 0D
		bool GetManagerHandlesSaveLoad() override;                                // 0E
		bool EffectShouldFaceTarget() override;                                   // 17
		TESObjectREFR* GetFacingTarget() override;                                // 18
		void SetWindPoint(const NiPoint3& a_point) override;                      // 1E
		const NiPoint3& GetWindPoint() override;                                  // 1F
		bool GetAllowNo3D() override;                                             // 20
		void SaveGame(BGSSaveGameBuffer* a_buf) override;                         // 21
		void LoadGame(BGSLoadGameBuffer* a_buf) override;                         // 22

		// members
		ActiveEffect* effect;      // 08
		NiPoint3 windPoint;        // 10
		ObjectRefHandle target;    // 1C
	};
	static_assert(sizeof(ActiveEffectReferenceEffectController) == 0x20);
}
