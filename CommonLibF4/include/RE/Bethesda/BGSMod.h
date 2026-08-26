#pragma once

#include "RE/Bethesda/BSFixedString.h"
#include "RE/Bethesda/BSTArray.h"
#include "RE/Bethesda/BSTHashMap.h"
#include "RE/Bethesda/FormComponents.h"
#include "RE/Bethesda/TESForms.h"

namespace RE
{
	struct INSTANCE_FILTER;
}

namespace RE::BGSMod
{
	namespace Attachment
	{
		class Mod;

		struct Instance;
	}

	namespace Property
	{
		enum class OP : std::uint32_t
		{
			kSet = 0,
			kMul = 1,
			kAnd = 1,
			kRem = 1,
			kAdd = 2,
			kOr = 2
		};

		enum class TYPE : std::uint32_t
		{
			kInt,
			kFloat,
			kBool,
			kString,
			kForm,
			kEnum,
			kPair
		};

		class Mod  // id == 1
		{
		public:
			union FLOATINT
			{
			public:
				// members
				std::int32_t i;
				float f;
			};
			static_assert(sizeof(FLOATINT) == 0x4);

			struct MinMax
			{
			public:
				// members
				FLOATINT min;  // 0
				FLOATINT max;  // 4
			};
			static_assert(sizeof(MinMax) == 0x8);

			struct FormValuePair
			{
			public:
				// members
				std::uint32_t formID;  // 0
				float value;           // 4
			};
			static_assert(sizeof(FormValuePair) == 0x8);

			union DATATYPE
			{
			public:
				DATATYPE()
				noexcept :
					form(nullptr)
				{}

				~DATATYPE() noexcept {}

				// members
				BSFixedString str;
				TESForm* form;
				MinMax mm;
				FormValuePair fv;
			};
			static_assert(sizeof(DATATYPE) == 0x8);

			// members
			DATATYPE data;             // 00
			std::uint32_t target: 11;  // 08:00
			OP op: 2;                  // 08:11
			TYPE type: 3;              // 08:13
			std::int16_t step;         // 0C
		};
		static_assert(sizeof(Mod) == 0x10);
	}

	class Container :
		public BSTDataBuffer<2>  // 00
	{
	public:
		static constexpr auto RTTI{ RTTI::BGSMod__Container };

		struct Data
		{
		public:
			// members
			Attachment::Instance* attachments;  // 00
			Property::Mod* propertyMods;        // 08
			std::uint32_t attachmentCount;      // 10
			std::uint32_t propertyModCount;     // 14
		};
		static_assert(sizeof(Data) == 0x18);

		Data* GetData(Data* a_data) const
		{
			using func_t = decltype(&Container::GetData);
			REL::Relocation<func_t> func{ REL::ID(659507, 2189206) };
			return func(this, a_data);
		}

		void Set(const Data& a_data)
		{
			using func_t = decltype(&Container::Set);
			REL::Relocation<func_t> func{ REL::ID(33420, 2189188) };
			return func(this, a_data);
		}
	};
	static_assert(sizeof(Container) == 0x10);

	struct ObjectIndexData
	{
	public:
		std::uint32_t objectID;  // 0
		std::uint8_t index;      // 4
		std::uint8_t rank;       // 5
		std::uint8_t disabled;   // 6
	};
	static_assert(sizeof(ObjectIndexData) == 0x8);

	namespace Attachment
	{
		[[nodiscard]] inline BSTHashMap<const Mod*, TESObjectMISC*>& GetAllLooseMods()
		{
			REL::Relocation<BSTHashMap<const Mod*, TESObjectMISC*>*> mods{ REL::ID(1108112, 2661617), -0x8 };
			return *mods;
		}

		class __declspec(novtable) Mod :
			public TESForm,               // 00
			public TESFullName,           // 20
			public TESDescription,        // 30
			public BGSModelMaterialSwap,  // 48
			public Container              // 88
		{
		public:
			static constexpr auto RTTI{ RTTI::BGSMod__Attachment__Mod };
			static constexpr auto VTABLE{ VTABLE::BGSMod__Attachment__Mod };
			static constexpr auto FORM_ID{ ENUM_FORM_ID::kOMOD };

			struct Data :
				public Container::Data  // 00
			{
			public:
				// members
				stl::enumeration<ENUM_FORM_ID, std::uint8_t> targetFormType;  // 18
				std::int8_t maxRank;                                          // 19
				std::int8_t lvlsPerTierScaledOffset;                          // 1A
				bool optional;                                                // 1B
				bool childrenExclusive;                                       // 1C
			};
			static_assert(sizeof(Data) == 0x20);

			static void FindModsForLooseMod(TESObjectMISC* a_looseMod, BSScrapArray<BGSMod::Attachment::Mod*>& a_result)
			{
				using func_t = decltype(&Mod::FindModsForLooseMod);
				REL::Relocation<func_t> func{ REL::ID(410363, 2197524) };
				return func(a_looseMod, a_result);
			}

			void GetData(Data& a_data) const
			{
				a_data.targetFormType = *targetFormType;
				a_data.maxRank = static_cast<std::int8_t>(maxRank);
				a_data.lvlsPerTierScaledOffset = static_cast<std::int8_t>(lvlsPerTierScaledOffset);
				a_data.optional = optional;
				a_data.childrenExclusive = childrenExclusive;
				static_cast<const Container*>(this)->GetData(std::addressof(a_data));
			}

			void Set(const Data& a_data)
			{
				static_cast<Container*>(this)->Set(a_data);
				targetFormType = *a_data.targetFormType;
				maxRank = static_cast<std::uint8_t>(a_data.maxRank);
				lvlsPerTierScaledOffset = static_cast<std::uint8_t>(a_data.lvlsPerTierScaledOffset);
				optional = a_data.optional;
				childrenExclusive = a_data.childrenExclusive;
			}

			TESObjectMISC* GetLooseMod()
			{
				using func_t = decltype(&Mod::GetLooseMod);
				REL::Relocation<func_t> func{ REL::ID(1359613, 2197514) };
				return func(this);
			}

			void SetLooseMod(TESObjectMISC* misc)
			{
				using func_t = decltype(&Mod::SetLooseMod);
				REL::Relocation<func_t> func{ REL::ID(123132, 2197515) };
				return func(this, misc);
			}

			// members
			BGSAttachParentArray attachParents;                                           // 98
			BGSTypedKeywordValueArray<KeywordType::kInstantiationFilter> filterKeywords;  // B0
			BGSTypedKeywordValue<KeywordType::kAttachPoint> attachPoint;                  // C0
			stl::enumeration<ENUM_FORM_ID, std::uint8_t> targetFormType;                  // C2
			std::uint8_t maxRank;                                                         // C3
			std::uint8_t lvlsPerTierScaledOffset;                                         // C4
			std::int8_t priority;                                                         // C5
			bool optional: 1;                                                             // C6:0
			bool childrenExclusive: 1;                                                    // C6:1
		};
		static_assert(sizeof(Mod) == 0xC8);

		struct Instance  // id == 0
		{
		public:
			// members
			Mod* mod;                   // 00
			std::uint8_t index;         // 08
			bool optional: 1;           // 09:0
			bool childrenExclusive: 1;  // 09:1
		};
		static_assert(sizeof(Instance) == 0x10);
	}

	namespace Template
	{
		class __declspec(novtable) Item :
			public TESFullName,       // 00
			public BGSMod::Container  // 10
		{
		public:
			static constexpr auto RTTI{ RTTI::BGSMod__Template__Item };
			static constexpr auto VTABLE{ VTABLE::BGSMod__Template__Item };

			// members
			BGSMod::Template::Items* parentTemplate;  // 20
			BGSKeyword** nameKeywordA;                // 28
			std::uint16_t parent;                     // 30
			std::int8_t levelMin;                     // 32
			std::int8_t levelMax;                     // 33
			std::int8_t keywords;                     // 34
			std::int8_t tierStartLevel;               // 35
			std::int8_t altLevelsPerTier;             // 36
			bool isDefault: 1;                        // 37:1
			bool fullNameEditorOnly: 1;               // 37:2
		};
		static_assert(sizeof(Item) == 0x38);

		class __declspec(novtable) Items :
			public BaseFormComponent  // 00
		{
		public:
			static constexpr auto RTTI{ RTTI::BGSMod__Template__Items };
			static constexpr auto VTABLE{ VTABLE::BGSMod__Template__Items };

			static void CreateInstanceDataForObjectAndExtra(TESBoundObject& a_object, ExtraDataList& a_extra, const INSTANCE_FILTER* a_filter, bool a_useDefault)
			{
				using func_t = decltype(&Items::CreateInstanceDataForObjectAndExtra);
				REL::Relocation<func_t> func{ REL::ID(147297, 2189244) };
				return func(a_object, a_extra, a_filter, a_useDefault);
			}

			// override (BaseFormComponent)
			std::uint32_t GetFormComponentType() const override { return 'TJBO'; }  // 01
			void InitializeDataComponent() override { return; }                     // 02
			void ClearDataComponent() override;                                     // 03
			void InitComponent() override;                                          // 04
			void CopyComponent(BaseFormComponent*) override { return; }             // 06
			void CopyComponent(BaseFormComponent*, TESForm*) override;              // 05

			// members
			BSTArray<Item*> items;  // 08
		};
		static_assert(sizeof(Items) == 0x20);
	}
}
