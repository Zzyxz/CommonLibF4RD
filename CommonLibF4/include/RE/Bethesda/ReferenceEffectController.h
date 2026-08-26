#pragma once

#include "RE/Bethesda/BSFixedString.h"

namespace RE
{
	class BGSArtObject;
	class BGSLoadGameBuffer;
	class BGSSaveGameBuffer;
	class NiAVObject;
	class NiNode;
	class NiPoint3;
	class ReferenceEffect;
	class TESEffectShader;
	class TESObjectREFR;

	class __declspec(novtable) ReferenceEffectController
	{
	public:
		static constexpr auto RTTI{ RTTI::ReferenceEffectController };
		static constexpr auto VTABLE{ VTABLE::ReferenceEffectController };

		virtual ~ReferenceEffectController();  // 00

		// add
		virtual void HandleEvent(const BSFixedString& a_event);                 // 01
		virtual float GetElapsedTime();                                          // 02
		virtual float GetScale();                                                // 03
		virtual void SwitchAttachedRoot(NiNode* a_root, NiNode* a_attachRoot);  // 04
		virtual const NiPoint3& GetSourcePosition();                              // 05
		virtual bool GetUseSourcePosition();                                     // 06
		virtual bool GetNoInitialFlare();                                        // 07
		virtual bool GetEffectPersists();                                        // 08
		virtual bool GetGoryVisuals();                                           // 09
		virtual void RemoveHitEffect(ReferenceEffect* a_refEffect);              // 0A
		virtual TESObjectREFR* GetTargetReference() = 0;                         // 0B
		virtual BGSArtObject* GetHitEffectArt() = 0;                             // 0C
		virtual TESEffectShader* GetHitEffectShader() = 0;                       // 0D
		virtual bool GetManagerHandlesSaveLoad() = 0;                            // 0E
		virtual NiAVObject* GetAttachRoot();                                      // 0F
		virtual float GetParticleAttachExtent();                                 // 10
		virtual bool GetUseParticleAttachExtent();                               // 11
		virtual bool GetDoParticles();                                           // 12
		virtual bool GetParticlesUseLocalSpace();                                // 13
		virtual bool GetUseRootWorldRotate();                                    // 14
		virtual bool GetIsRootActor();                                           // 15
		virtual bool GetClearWhenCellIsUnloaded();                               // 16
		virtual bool EffectShouldFaceTarget();                                   // 17
		virtual TESObjectREFR* GetFacingTarget();                                // 18
		virtual bool GetShaderUseParentCell();                                   // 19
		virtual bool EffectAttachesToCamera();                                   // 1A
		virtual bool EffectRotatesWithCamera();                                  // 1B
		virtual bool GetAllowTargetRoot();                                       // 1C
		virtual bool IsReadyForAttach();                                         // 1D
		virtual void SetWindPoint(const NiPoint3& a_point);                      // 1E
		virtual const NiPoint3& GetWindPoint();                                  // 1F
		virtual bool GetAllowNo3D();                                             // 20
		virtual void SaveGame(BGSSaveGameBuffer* a_buf);                         // 21
		virtual void LoadGame(BGSLoadGameBuffer* a_buf);                         // 22
	};
	static_assert(sizeof(ReferenceEffectController) == 0x8);
}
