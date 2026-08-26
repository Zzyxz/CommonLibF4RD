#pragma once

#include <RE/Bethesda/Actor.h>
#include <RE/Bethesda/BSFixedString.h>
#include <RE/Bethesda/TESBoundObjects.h>
#include <RE/Bethesda/bhkPickData.h>
#include <RE/NetImmerse/NiAVObject.h>
#include <RE/NetImmerse/NiPoint3.h>
#include <REL/Relocation.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace RE
{
	namespace BSUtilities
	{
		inline NiAVObject* GetObjectByName(NiAVObject* root, const BSFixedString& name, bool tryInternal, bool dontAttach)
		{
			using func_t = decltype(&GetObjectByName);
			REL::Relocation<func_t> func{ REL::ID(843650, 2274841) };
			return func(root, name, tryInternal, dontAttach);
		}
	}

	namespace CombatUtilities
	{
		inline bool CalculateProjectileTrajectory(const NiPoint3& pos, const NiPoint3& vel, float gravity, const NiPoint3& targetPos, float X, NiPoint3& out)
		{
			using func_t = decltype(&CalculateProjectileTrajectory);
			REL::Relocation<func_t> func{ REL::ID(1575156, 2240611) };
			return func(pos, vel, gravity, targetPos, X, out);
		}

		inline bool CalculateProjectileLOS(Actor* a, BGSProjectile* proj, float speed, const NiPoint3& launchPos, const NiPoint3& targetPos, NiPoint3* hitPos, TESObjectREFR** collidee, float* dist)
		{
			typedef bool func_t(Actor*, BGSProjectile*, float, const NiPoint3&, const NiPoint3&, NiPoint3*, TESObjectREFR**, float*);
			REL::Relocation<func_t> func{ REL::ID(798616, 2240617) };
			return func(a, proj, speed, launchPos, targetPos, hitPos, collidee, dist);
		}

		inline bool CalculateProjectileLOS(Actor* a, BGSProjectile* proj, bhkPickData& pick)
		{
			typedef bool func_t(Actor*, BGSProjectile*, bhkPickData&);
			REL::Relocation<func_t> func{ REL::ID(55339, 2240616) };
			return func(a, proj, pick);
		}
		static REL::Relocation<float> fWorldGravity{ REL::ID(1378547, 2700340) };
	};

	namespace WeaponUtils
	{
		[[nodiscard]] inline bool GetAnimationTimes(
			const TESObjectWEAP& a_weapon,
			const TESObjectWEAP::InstanceData* a_instanceData,
			float& a_time1,
			float& a_time2,
			float& a_time3,
			float& a_time4)
		{
			using func_t = decltype(&GetAnimationTimes);
			REL::Relocation<func_t> func{ REL::ID(1313135, 2199154) };
			return func(a_weapon, a_instanceData, a_time1, a_time2, a_time3, a_time4);
		}
	}

	namespace AnimationSystemUtils
	{
		inline bool WillEventChangeState(const TESObjectREFR& ref, const BSFixedString& evn)
		{
			using func_t = decltype(&WillEventChangeState);
			REL::Relocation<func_t> func{ REL::ID(35074, 2214271) };
			return func(ref, evn);
		}
	}

	namespace BGSAnimationSystemUtils
	{
		inline bool InitializeActorInstant(Actor& a, bool b)
		{
			using func_t = decltype(&InitializeActorInstant);
			REL::Relocation<func_t> func{ REL::ID(672857, 2236393) };
			return func(a, b);
		}

		struct ActiveSyncInfo
		{
			BSTObjectArena<BSTTuple<BSFixedString, float>> otherSyncInfo;
			float currentAnimTime;
			float animSpeedMult;
			float totalAnimTime;
		};

		inline bool GetActiveSyncInfo(const IAnimationGraphManagerHolder* a_graphHolder, ActiveSyncInfo& a_infoOut)
		{
			using func_t = decltype(&GetActiveSyncInfo);
			REL::Relocation<func_t> func{ REL::ID(1349978, 2214289) };
			return func(a_graphHolder, a_infoOut);
		}

		inline bool IsActiveGraphInTransition(const TESObjectREFR* a_refr)
		{
			using func_t = decltype(&IsActiveGraphInTransition);
			REL::Relocation<func_t> func{ REL::ID(839650, 2214305) };
			return func(a_refr);
		}
	};

	namespace PerkUtilities
	{
		inline void RemoveGrenadeTrajectory()
		{
			using func_t = decltype(&RemoveGrenadeTrajectory);
			REL::Relocation<func_t> func{ REL::ID(672186, 2233303) };
			return func();
		}
	}

	namespace MaterialUtils
	{
		enum class Category : std::uint8_t
		{
			kUnknown,
			kMetal,
			kWood,
			kPlastic,
			kRubber,
			kGlass,
			kCeramic,
			kConcrete,
			kStone,
			kPaper,
			kFabric,
			kWater,
			kGround,
			kOrganic,
			kArmor
		};

		struct KnownMaterial
		{
			std::string_view name;
			Category category;
		};

		// Complete MNAM material catalog.
		inline constexpr std::array<KnownMaterial, 147> kKnownMaterials{
			KnownMaterial{ "ActorArmored", Category::kArmor },
			KnownMaterial{ "ActorArmoredCrab", Category::kArmor },
			KnownMaterial{ "ActorGeneric", Category::kOrganic },
			KnownMaterial{ "ActorGhost", Category::kOrganic },
			KnownMaterial{ "ActorGlowing", Category::kOrganic },
			KnownMaterial{ "ActorInsect", Category::kOrganic },
			KnownMaterial{ "ActorInsectSmall", Category::kOrganic },
			KnownMaterial{ "ActorMetal", Category::kMetal },
			KnownMaterial{ "ActorMetalArmoredPower", Category::kArmor },
			KnownMaterial{ "ActorMetalLarge", Category::kMetal },
			KnownMaterial{ "ActorMetalSmall", Category::kMetal },
			KnownMaterial{ "ActorSkeleton", Category::kOrganic },
			KnownMaterial{ "ActorSkin", Category::kOrganic },
			KnownMaterial{ "ActorSkinLarge", Category::kOrganic },
			KnownMaterial{ "ActorSkinSmall", Category::kOrganic },
			KnownMaterial{ "ArmorHeavy", Category::kArmor },
			KnownMaterial{ "ArmorLight", Category::kArmor },
			KnownMaterial{ "Arrow", Category::kWood },
			KnownMaterial{ "Axe1Hand", Category::kMetal },
			KnownMaterial{ "Basket", Category::kWood },
			KnownMaterial{ "Book", Category::kPaper },
			KnownMaterial{ "Bottle", Category::kGlass },
			KnownMaterial{ "BottleSmall", Category::kGlass },
			KnownMaterial{ "Brick", Category::kConcrete },
			KnownMaterial{ "Carpet", Category::kFabric },
			KnownMaterial{ "CeramicMedium", Category::kCeramic },
			KnownMaterial{ "CHAIN", Category::kMetal },
			KnownMaterial{ "Cloth", Category::kFabric },
			KnownMaterial{ "ClothCushion", Category::kFabric },
			KnownMaterial{ "Coin", Category::kMetal },
			KnownMaterial{ "Concrete", Category::kConcrete },
			KnownMaterial{ "ConcreteStairs", Category::kConcrete },
			KnownMaterial{ "debug", Category::kUnknown },
			KnownMaterial{ "Dirt", Category::kGround },
			KnownMaterial{ "DirtStairs", Category::kGround },
			KnownMaterial{ "Generic", Category::kUnknown },
			KnownMaterial{ "Glass", Category::kGlass },
			KnownMaterial{ "GlassStairs", Category::kGlass },
			KnownMaterial{ "Grass", Category::kGround },
			KnownMaterial{ "GrassStairs", Category::kGround },
			KnownMaterial{ "Gravel", Category::kGround },
			KnownMaterial{ "Ice", Category::kGround },
			KnownMaterial{ "Insect", Category::kOrganic },
			KnownMaterial{ "Insect Wing", Category::kOrganic },
			KnownMaterial{ "InsectFloor", Category::kOrganic },
			KnownMaterial{ "MaterialActorSynthGen1", Category::kMetal },
			KnownMaterial{ "MaterialBaseball", Category::kRubber },
			KnownMaterial{ "MaterialBodyBone", Category::kOrganic },
			KnownMaterial{ "MaterialCeramicCoffeeMug", Category::kCeramic },
			KnownMaterial{ "MaterialClipboard", Category::kPaper },
			KnownMaterial{ "MaterialGroundDirtLeaves", Category::kGround },
			KnownMaterial{ "MaterialGroundGrassBush", Category::kGround },
			KnownMaterial{ "MaterialGroundMetalA", Category::kGround },
			KnownMaterial{ "MaterialGroundTileVinyl", Category::kGround },
			KnownMaterial{ "MaterialGroundTileVinylSanctuaryHills", Category::kGround },
			KnownMaterial{ "MaterialMetalAuto", Category::kMetal },
			KnownMaterial{ "MaterialMetalBucket", Category::kMetal },
			KnownMaterial{ "MaterialMetalCanTin", Category::kMetal },
			KnownMaterial{ "MaterialMetalChairFolding", Category::kMetal },
			KnownMaterial{ "MaterialMetalGasLine", Category::kMetal },
			KnownMaterial{ "MaterialMetalGateVault", Category::kMetal },
			KnownMaterial{ "MaterialMetalLunchbox", Category::kMetal },
			KnownMaterial{ "MaterialMetalShoppingCart", Category::kMetal },
			KnownMaterial{ "MaterialMetalSign", Category::kMetal },
			KnownMaterial{ "MaterialMetalStairs", Category::kMetal },
			KnownMaterial{ "MaterialMetalTray", Category::kMetal },
			KnownMaterial{ "MaterialMetalVertibirdChassisLarge", Category::kMetal },
			KnownMaterial{ "MaterialMetalVertibirdChassisSmall", Category::kMetal },
			KnownMaterial{ "MaterialMetalWeight", Category::kMetal },
			KnownMaterial{ "MaterialOtherBone", Category::kOrganic },
			KnownMaterial{ "MaterialPaperBoxCardboard", Category::kPaper },
			KnownMaterial{ "MaterialPaperBoxSmall", Category::kPaper },
			KnownMaterial{ "MaterialPaperMagazine", Category::kPaper },
			KnownMaterial{ "MaterialPlasticBin", Category::kPlastic },
			KnownMaterial{ "MaterialPlasticBowlingBall", Category::kPlastic },
			KnownMaterial{ "MaterialPlasticLarge", Category::kPlastic },
			KnownMaterial{ "MaterialPlasticTrafficCone", Category::kPlastic },
			KnownMaterial{ "MaterialRubberBall", Category::kRubber },
			KnownMaterial{ "MaterialRubberTire", Category::kRubber },
			KnownMaterial{ "MaterialStoneSubwayPillarLarge", Category::kStone },
			KnownMaterial{ "MaterialSubwayGate", Category::kMetal },
			KnownMaterial{ "MaterialWeaponBlockBladeMedium", Category::kMetal },
			KnownMaterial{ "MaterialWeaponBlockBladeSmall", Category::kMetal },
			KnownMaterial{ "MaterialWeaponCasing", Category::kMetal },
			KnownMaterial{ "MaterialWeaponCasingMinigun", Category::kMetal },
			KnownMaterial{ "MaterialWeaponCasingPistol", Category::kMetal },
			KnownMaterial{ "MaterialWeaponCasingRifle", Category::kMetal },
			KnownMaterial{ "MaterialWeaponCasingShotgun", Category::kMetal },
			KnownMaterial{ "MaterialWeaponMetalBlade1Hand", Category::kMetal },
			KnownMaterial{ "MaterialWeaponMetalBlade1HandSmall", Category::kMetal },
			KnownMaterial{ "MaterialWeaponMetalGrenade", Category::kMetal },
			KnownMaterial{ "MaterialWeaponMetalMine", Category::kMetal },
			KnownMaterial{ "MaterialWeaponPistol", Category::kMetal },
			KnownMaterial{ "MaterialWeaponRifle", Category::kMetal },
			KnownMaterial{ "MaterialWeaponRifleLarge", Category::kMetal },
			KnownMaterial{ "MaterialWoodBoard", Category::kWood },
			KnownMaterial{ "MaterialWoodBroom", Category::kWood },
			KnownMaterial{ "MaterialWoodCrate", Category::kWood },
			KnownMaterial{ "Meat", Category::kOrganic },
			KnownMaterial{ "Metal", Category::kMetal },
			KnownMaterial{ "MetalBarrel", Category::kMetal },
			KnownMaterial{ "MetalBarrelTrashCan", Category::kMetal },
			KnownMaterial{ "MetalBarrelTrashCanOffice", Category::kMetal },
			KnownMaterial{ "MetalHeavy", Category::kMetal },
			KnownMaterial{ "MetalHollow", Category::kMetal },
			KnownMaterial{ "MetalLight", Category::kMetal },
			KnownMaterial{ "MetalSolid", Category::kMetal },
			KnownMaterial{ "MetalStairs", Category::kMetal },
			KnownMaterial{ "Mud", Category::kGround },
			KnownMaterial{ "Organic", Category::kOrganic },
			KnownMaterial{ "OrganicLarge", Category::kOrganic },
			KnownMaterial{ "OtherParent", Category::kUnknown },
			KnownMaterial{ "Paper", Category::kPaper },
			KnownMaterial{ "Plastic", Category::kPlastic },
			KnownMaterial{ "PotsPans", Category::kMetal },
			KnownMaterial{ "Sand", Category::kGround },
			KnownMaterial{ "Sandbag", Category::kGround },
			KnownMaterial{ "ShieldHeavy", Category::kArmor },
			KnownMaterial{ "ShieldLight", Category::kArmor },
			KnownMaterial{ "Skin", Category::kOrganic },
			KnownMaterial{ "Snow", Category::kGround },
			KnownMaterial{ "SnowStairs", Category::kGround },
			KnownMaterial{ "Stone", Category::kStone },
			KnownMaterial{ "StoneAsStairs", Category::kStone },
			KnownMaterial{ "StoneBoulderLarge", Category::kStone },
			KnownMaterial{ "StoneBoulderMedium", Category::kStone },
			KnownMaterial{ "StoneBoulderSmall", Category::kStone },
			KnownMaterial{ "StoneBroken", Category::kStone },
			KnownMaterial{ "StoneBrokenStairs", Category::kStone },
			KnownMaterial{ "StoneHeavy", Category::kStone },
			KnownMaterial{ "StoneStairs", Category::kStone },
			KnownMaterial{ "WARD", Category::kUnknown },
			KnownMaterial{ "Water", Category::kWater },
			KnownMaterial{ "WaterPuddle", Category::kWater },
			KnownMaterial{ "Weapon", Category::kUnknown },
			KnownMaterial{ "WeaponBlade2Hand", Category::kMetal },
			KnownMaterial{ "WeaponBlockBlunt", Category::kMetal },
			KnownMaterial{ "WeaponBlunt1Hand", Category::kMetal },
			KnownMaterial{ "WeaponBlunt2Hand", Category::kMetal },
			KnownMaterial{ "WeaponBowsStaves", Category::kWood },
			KnownMaterial{ "Web", Category::kFabric },
			KnownMaterial{ "Wood", Category::kWood },
			KnownMaterial{ "WoodAsStairs", Category::kWood },
			KnownMaterial{ "WoodBarrel", Category::kWood },
			KnownMaterial{ "WoodHeavy", Category::kWood },
			KnownMaterial{ "WoodLight", Category::kWood },
			KnownMaterial{ "WoodStairs", Category::kWood }
		};

		namespace detail
		{
			[[nodiscard]] constexpr char ToUpperASCII(char a_character) noexcept
			{
				return a_character >= 'a' && a_character <= 'z' ?
				           static_cast<char>(a_character - ('a' - 'A')) :
				           a_character;
			}

			[[nodiscard]] constexpr bool StartsWithCaseInsensitive(
				std::string_view a_value,
				std::string_view a_prefix) noexcept
			{
				if (a_prefix.empty() || a_value.size() < a_prefix.size()) {
					return false;
				}

				for (std::size_t i = 0; i < a_prefix.size(); ++i) {
					if (ToUpperASCII(a_value[i]) != ToUpperASCII(a_prefix[i])) {
						return false;
					}
				}
				return true;
			}

			[[nodiscard]] constexpr bool ContainsCaseInsensitive(
				std::string_view a_value,
				std::string_view a_needle) noexcept
			{
				if (a_needle.empty() || a_value.size() < a_needle.size()) {
					return false;
				}

				for (std::size_t offset = 0; offset <= a_value.size() - a_needle.size(); ++offset) {
					if (StartsWithCaseInsensitive(a_value.substr(offset), a_needle)) {
						return true;
					}
				}
				return false;
			}
		}

		[[nodiscard]] constexpr const KnownMaterial* FindKnownMaterial(std::string_view a_name) noexcept
		{
			for (const auto& material : kKnownMaterials) {
				if (material.name.size() == a_name.size() &&
				    detail::StartsWithCaseInsensitive(a_name, material.name)) {
					return &material;
				}
			}
			return nullptr;
		}

		[[nodiscard]] constexpr Category ClassifyName(std::string_view a_name) noexcept
		{
			using detail::ContainsCaseInsensitive;
			using detail::StartsWithCaseInsensitive;

			if (const auto* known = FindKnownMaterial(a_name)) {
				return known->category;
			}

			if (ContainsCaseInsensitive(a_name, "ARMOR") || StartsWithCaseInsensitive(a_name, "SHIELD") ||
			    StartsWithCaseInsensitive(a_name, "WARD")) {
				return Category::kArmor;
			}
			if (ContainsCaseInsensitive(a_name, "PLASTIC")) {
				return Category::kPlastic;
			}
			if (ContainsCaseInsensitive(a_name, "RUBBER") || ContainsCaseInsensitive(a_name, "TIRE")) {
				return Category::kRubber;
			}
			if (ContainsCaseInsensitive(a_name, "WOOD") || ContainsCaseInsensitive(a_name, "BASKET")) {
				return Category::kWood;
			}
			if (ContainsCaseInsensitive(a_name, "GLASS") || StartsWithCaseInsensitive(a_name, "BOTTLE")) {
				return Category::kGlass;
			}
			if (ContainsCaseInsensitive(a_name, "CERAMIC")) {
				return Category::kCeramic;
			}
			if (ContainsCaseInsensitive(a_name, "CONCRETE") || ContainsCaseInsensitive(a_name, "BRICK")) {
				return Category::kConcrete;
			}
			if (ContainsCaseInsensitive(a_name, "STONE") || ContainsCaseInsensitive(a_name, "BOULDER")) {
				return Category::kStone;
			}
			if (ContainsCaseInsensitive(a_name, "PAPER") || StartsWithCaseInsensitive(a_name, "BOOK") ||
			    ContainsCaseInsensitive(a_name, "CLIPBOARD")) {
				return Category::kPaper;
			}
			if (ContainsCaseInsensitive(a_name, "CLOTH") || ContainsCaseInsensitive(a_name, "CARPET") ||
			    ContainsCaseInsensitive(a_name, "WEB")) {
				return Category::kFabric;
			}
			if (ContainsCaseInsensitive(a_name, "WATER") || StartsWithCaseInsensitive(a_name, "PUDDLE")) {
				return Category::kWater;
			}
			if (StartsWithCaseInsensitive(a_name, "DIRT") || StartsWithCaseInsensitive(a_name, "GRASS") ||
			    StartsWithCaseInsensitive(a_name, "MUD") || StartsWithCaseInsensitive(a_name, "GRAVEL") ||
			    StartsWithCaseInsensitive(a_name, "SAND") || StartsWithCaseInsensitive(a_name, "SNOW") ||
			    StartsWithCaseInsensitive(a_name, "ICE") || StartsWithCaseInsensitive(a_name, "GROUND")) {
				return Category::kGround;
			}
			if (ContainsCaseInsensitive(a_name, "METAL") || StartsWithCaseInsensitive(a_name, "CHAIN") ||
			    StartsWithCaseInsensitive(a_name, "POTSPAN") || StartsWithCaseInsensitive(a_name, "COIN")) {
				return Category::kMetal;
			}
			if (ContainsCaseInsensitive(a_name, "ACTOR") || ContainsCaseInsensitive(a_name, "SKIN") ||
			    ContainsCaseInsensitive(a_name, "FLESH") || ContainsCaseInsensitive(a_name, "ORGANIC") ||
			    ContainsCaseInsensitive(a_name, "MEAT") || ContainsCaseInsensitive(a_name, "INSECT") ||
			    ContainsCaseInsensitive(a_name, "BONE")) {
				return Category::kOrganic;
			}
			return Category::kUnknown;
		}

		[[nodiscard]] inline Category Classify(
			const BGSMaterialType* a_material,
			std::size_t a_maximumParentDepth = 16) noexcept
		{
			std::size_t depth = 0;
			for (auto* current = a_material; current && depth < a_maximumParentDepth; current = current->parentType, ++depth) {
				const auto* name = current->materialName.c_str();
				const auto category = ClassifyName(name ? std::string_view{ name } : std::string_view{});
				if (category != Category::kUnknown) {
					return category;
				}
			}
			return Category::kUnknown;
		}

		[[nodiscard]] constexpr std::string_view GetCategoryName(Category a_category) noexcept
		{
			switch (a_category) {
			case Category::kMetal:
				return "Metal";
			case Category::kWood:
				return "Wood";
			case Category::kPlastic:
				return "Plastic";
			case Category::kRubber:
				return "Rubber";
			case Category::kGlass:
				return "Glass";
			case Category::kCeramic:
				return "Ceramic";
			case Category::kConcrete:
				return "Concrete";
			case Category::kStone:
				return "Stone";
			case Category::kPaper:
				return "Paper";
			case Category::kFabric:
				return "Fabric";
			case Category::kWater:
				return "Water";
			case Category::kGround:
				return "Ground";
			case Category::kOrganic:
				return "Organic";
			case Category::kArmor:
				return "Armor";
			default:
				return "Unknown";
			}
		}
	}
}
