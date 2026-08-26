#pragma once

#include <cstdint>
#include <limits>
#include <mutex>
#include <utility>
#define REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER_IMPL(a_nopropQual, a_propQual, ...)              \
	template <                                                                                    \
		class R,                                                                                  \
		class Cls,                                                                                \
		class... Args>                                                                            \
	struct member_function_pod_type<R (Cls::*)(Args...) __VA_ARGS__ a_nopropQual a_propQual>      \
	{                                                                                             \
		using type = R(__VA_ARGS__ Cls*, Args...) a_propQual;                                     \
	};                                                                                            \
                                                                                                  \
	template <                                                                                    \
		class R,                                                                                  \
		class Cls,                                                                                \
		class... Args>                                                                            \
	struct member_function_pod_type<R (Cls::*)(Args..., ...) __VA_ARGS__ a_nopropQual a_propQual> \
	{                                                                                             \
		using type = R(__VA_ARGS__ Cls*, Args..., ...) a_propQual;                                \
	};

#define REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER(a_qualifer, ...)              \
	REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER_IMPL(a_qualifer, , ##__VA_ARGS__) \
	REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER_IMPL(a_qualifer, noexcept, ##__VA_ARGS__)

#define REL_MAKE_MEMBER_FUNCTION_POD_TYPE(...)                 \
	REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER(, __VA_ARGS__)    \
	REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER(&, ##__VA_ARGS__) \
	REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER(&&, ##__VA_ARGS__)

#define REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER_IMPL(a_nopropQual, a_propQual, ...)              \
	template <                                                                                        \
		class R,                                                                                      \
		class Cls,                                                                                    \
		class... Args>                                                                                \
	struct member_function_non_pod_type<R (Cls::*)(Args...) __VA_ARGS__ a_nopropQual a_propQual>      \
	{                                                                                                 \
		using type = R&(__VA_ARGS__ Cls*, void*, Args...)a_propQual;                                  \
	};                                                                                                \
                                                                                                      \
	template <                                                                                        \
		class R,                                                                                      \
		class Cls,                                                                                    \
		class... Args>                                                                                \
	struct member_function_non_pod_type<R (Cls::*)(Args..., ...) __VA_ARGS__ a_nopropQual a_propQual> \
	{                                                                                                 \
		using type = R&(__VA_ARGS__ Cls*, void*, Args..., ...)a_propQual;                             \
	};

#define REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER(a_qualifer, ...)              \
	REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER_IMPL(a_qualifer, , ##__VA_ARGS__) \
	REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER_IMPL(a_qualifer, noexcept, ##__VA_ARGS__)

#define REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE(...)                 \
	REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER(, __VA_ARGS__)    \
	REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER(&, ##__VA_ARGS__) \
	REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER(&&, ##__VA_ARGS__)

namespace REL
{
	class ID;
	class IDManager;
	class Module;
	class Offset;
	class RuntimeDatabase;
	class Segment;
	class VariantOffset;
	class Version;

	template <class>
	class Relocation;

	enum class IDResolveMode : std::uint8_t
	{
		kNormal,
		kRuntimeOnly,
		kPatternsOnly,
		kPatternsShared
	};

	enum class IDResolveStatus : std::uint8_t
	{
		kResolvedLocalCache,
		kResolvedKnownRVA,
		kResolvedSharedCache,
		kResolvedPattern,
		kResolvedLegacy,
		kOGBridgeFailed,
		kNGBridgeFailed,
		kUnknownID,
		kNoCompatiblePattern,
		kPatternNotFound,
		kPatternAmbiguous,
		kInvalidPattern,
		kSemanticValidationFailed,
		kCallsiteNotFound,
		kCallsiteAmbiguous,
		kInvalidOffset,
		kInvalidCallsite,
		kUnresolved,
		kRuntimeUnavailable
	};

	struct IDResolveResult
	{
		std::uint64_t id{};
		std::optional<std::size_t> rva;
		IDResolveStatus status{ IDResolveStatus::kUnknownID };
		std::uint64_t elapsedMicroseconds{};
		std::optional<std::ptrdiff_t> selectedOffset;
		std::optional<std::size_t> finalRva;
		bool automaticOffset{};

		[[nodiscard]] explicit operator bool() const noexcept { return rva.has_value(); }
	};

	struct IDValidationSummary
	{
		std::vector<IDResolveResult> entries;
		std::size_t resolved{};
		std::size_t failed{};
		std::uint64_t elapsedMicroseconds{};
	};

	[[nodiscard]] std::string_view id_resolve_status_text(IDResolveStatus a_status) noexcept;

	namespace detail
	{
		template <class>
		struct member_function_pod_type;

		REL_MAKE_MEMBER_FUNCTION_POD_TYPE();
		REL_MAKE_MEMBER_FUNCTION_POD_TYPE(const);
		REL_MAKE_MEMBER_FUNCTION_POD_TYPE(volatile);
		REL_MAKE_MEMBER_FUNCTION_POD_TYPE(const volatile);

		template <class F>
		using member_function_pod_type_t = typename member_function_pod_type<F>::type;

		template <class>
		struct member_function_non_pod_type;

		REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE();
		REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE(const);
		REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE(volatile);
		REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE(const volatile);

		template <class F>
		using member_function_non_pod_type_t = typename member_function_non_pod_type<F>::type;

		// https://docs.microsoft.com/en-us/cpp/build/x64-calling-convention

		template <class T>
		struct meets_length_req :
			std::disjunction<
				std::bool_constant<sizeof(T) == 1>,
				std::bool_constant<sizeof(T) == 2>,
				std::bool_constant<sizeof(T) == 4>,
				std::bool_constant<sizeof(T) == 8>>
		{};

		template <class T>
		struct meets_function_req :
			std::conjunction<
				std::is_trivially_constructible<T>,
				std::is_trivially_destructible<T>,
				std::is_trivially_copy_assignable<T>,
				std::negation<
					std::is_polymorphic<T>>>
		{};

		template <class T>
		struct meets_member_req :
			std::is_standard_layout<T>
		{};

		template <class T, class = void>
		struct is_x64_pod :
			std::true_type
		{};

		template <class T>
		struct is_x64_pod<
			T,
			std::enable_if_t<
				std::is_union_v<T>>> :
			std::false_type
		{};

		template <class T>
		struct is_x64_pod<
			T,
			std::enable_if_t<
				std::is_class_v<T>>> :
			std::conjunction<
				meets_length_req<T>,
				meets_function_req<T>,
				meets_member_req<T>>
		{};

		template <class T>
		inline constexpr bool is_x64_pod_v = is_x64_pod<T>::value;

		template <
			class F,
			class First,
			class... Rest>
		decltype(auto) invoke_member_function_non_pod(F&& a_func, First&& a_first, Rest&&... a_rest)  //
			noexcept(std::is_nothrow_invocable_v<F, First, Rest...>)
		{
			using result_t = std::invoke_result_t<F, First, Rest...>;
			std::aligned_storage_t<sizeof(result_t), alignof(result_t)> result;

			using func_t = member_function_non_pod_type_t<F>;
			auto func = stl::unrestricted_cast<func_t*>(std::forward<F>(a_func));

			return func(std::forward<First>(a_first), std::addressof(result), std::forward<Rest>(a_rest)...);
		}
	}

	inline constexpr std::uint8_t NOP = 0x90;
	inline constexpr std::uint8_t RET = 0xC3;
	inline constexpr std::uint8_t INT3 = 0xCC;

	template <class F, class... Args>
	std::invoke_result_t<F, Args...> invoke(F&& a_func, Args&&... a_args)  //
		noexcept(std::is_nothrow_invocable_v<F, Args...>)                  //
		requires(std::invocable<F, Args...>)
	{
		if constexpr (std::is_member_function_pointer_v<std::decay_t<F>>) {
			if constexpr (detail::is_x64_pod_v<std::invoke_result_t<F, Args...>>) {  // member functions == free functions in x64
				using func_t = detail::member_function_pod_type_t<std::decay_t<F>>;
				auto func = stl::unrestricted_cast<func_t*>(std::forward<F>(a_func));
				return func(std::forward<Args>(a_args)...);
			} else {  // shift args to insert result
				return detail::invoke_member_function_non_pod(std::forward<F>(a_func), std::forward<Args>(a_args)...);
			}
		} else {
			return std::forward<F>(a_func)(std::forward<Args>(a_args)...);
		}
	}

	inline void safe_write(std::uintptr_t a_dst, const void* a_src, std::size_t a_count)
	{
		std::uint32_t old{ 0 };
		auto success =
			WinAPI::VirtualProtect(
				reinterpret_cast<void*>(a_dst),
				a_count,
				(WinAPI::PAGE_EXECUTE_READWRITE),
				std::addressof(old));
		if (success != 0) {
			std::memcpy(reinterpret_cast<void*>(a_dst), a_src, a_count);
			success =
				WinAPI::VirtualProtect(
					reinterpret_cast<void*>(a_dst),
					a_count,
					old,
					std::addressof(old));
		}

		assert(success != 0);
	}

	template <std::integral T>
	void safe_write(std::uintptr_t a_dst, const T& a_data)
	{
		safe_write(a_dst, std::addressof(a_data), sizeof(T));
	}

	template <class T>
	void safe_write(std::uintptr_t a_dst, std::span<T> a_data)
	{
		safe_write(a_dst, a_data.data(), a_data.size_bytes());
	}

	inline void safe_fill(std::uintptr_t a_dst, std::uint8_t a_value, std::size_t a_count)
	{
		std::uint32_t old{ 0 };
		auto success =
			WinAPI::VirtualProtect(
				reinterpret_cast<void*>(a_dst),
				a_count,
				(WinAPI::PAGE_EXECUTE_READWRITE),
				std::addressof(old));
		if (success != 0) {
			std::fill_n(reinterpret_cast<std::uint8_t*>(a_dst), a_count, a_value);
			success =
				WinAPI::VirtualProtect(
					reinterpret_cast<void*>(a_dst),
					a_count,
					old,
					std::addressof(old));
		}

		assert(success != 0);
	}

	class Version
	{
	public:
		using value_type = std::uint16_t;
		using reference = value_type&;
		using const_reference = const value_type&;

		constexpr Version() noexcept = default;

		explicit constexpr Version(std::array<value_type, 4> a_version) noexcept :
			_impl(a_version)
		{}

		constexpr Version(value_type a_v1, value_type a_v2 = 0, value_type a_v3 = 0, value_type a_v4 = 0) noexcept :
			_impl{ a_v1, a_v2, a_v3, a_v4 }
		{}

		[[nodiscard]] constexpr reference operator[](std::size_t a_idx) noexcept { return _impl[a_idx]; }
		[[nodiscard]] constexpr const_reference operator[](std::size_t a_idx) const noexcept { return _impl[a_idx]; }

		[[nodiscard]] constexpr decltype(auto) begin() const noexcept { return _impl.begin(); }
		[[nodiscard]] constexpr decltype(auto) cbegin() const noexcept { return _impl.cbegin(); }
		[[nodiscard]] constexpr decltype(auto) end() const noexcept { return _impl.end(); }
		[[nodiscard]] constexpr decltype(auto) cend() const noexcept { return _impl.cend(); }

		[[nodiscard]] std::strong_ordering constexpr compare(const Version& a_rhs) const noexcept
		{
			for (std::size_t i = 0; i < _impl.size(); ++i) {
				if ((*this)[i] != a_rhs[i]) {
					return (*this)[i] < a_rhs[i] ? std::strong_ordering::less : std::strong_ordering::greater;
				}
			}
			return std::strong_ordering::equal;
		}

		[[nodiscard]] std::string string() const
		{
			std::string result;
			for (auto&& ver : _impl) {
				result += std::to_string(ver);
				result += '-';
			}
			result.pop_back();
			return result;
		}

		[[nodiscard]] std::wstring wstring() const
		{
			std::wstring result;
			for (auto&& ver : _impl) {
				result += std::to_wstring(ver);
				result += L'-';
			}
			result.pop_back();
			return result;
		}

	private:
		std::array<value_type, 4> _impl{ 0, 0, 0, 0 };
	};

	[[nodiscard]] constexpr bool operator==(const Version& a_lhs, const Version& a_rhs) noexcept { return a_lhs.compare(a_rhs) == 0; }
	[[nodiscard]] constexpr std::strong_ordering operator<=>(const Version& a_lhs, const Version& a_rhs) noexcept { return a_lhs.compare(a_rhs); }

	enum class RuntimeFamily : std::uint8_t
	{
		kOG,
		kNG,
		kAE
	};

	[[nodiscard]] constexpr RuntimeFamily runtime_family(const Version& a_version) noexcept
	{
		if (a_version >= Version{ 1, 11, 0, 0 }) {
			return RuntimeFamily::kAE;
		}
		if (a_version >= Version{ 1, 10, 980, 0 }) {
			return RuntimeFamily::kNG;
		}
		return RuntimeFamily::kOG;
	}

	[[nodiscard]] inline std::optional<Version> get_file_version(stl::zwstring a_filename)
	{
		std::uint32_t dummy;
		std::vector<char> buf(WinAPI::GetFileVersionInfoSize(a_filename.data(), std::addressof(dummy)));
		if (buf.empty()) {
			return std::nullopt;
		}

		if (!WinAPI::GetFileVersionInfo(a_filename.data(), 0, static_cast<std::uint32_t>(buf.size()), buf.data())) {
			return std::nullopt;
		}

		void* verBuf{ nullptr };
		std::uint32_t verLen{ 0 };
		if (!WinAPI::VerQueryValue(buf.data(), L"\\StringFileInfo\\040904B0\\ProductVersion", std::addressof(verBuf), std::addressof(verLen))) {
			return std::nullopt;
		}

		Version version;
		std::wistringstream ss(
			std::wstring(static_cast<const wchar_t*>(verBuf), verLen));
		std::wstring token;
		for (std::size_t i = 0; i < 4 && std::getline(ss, token, L'.'); ++i) {
			version[i] = static_cast<std::uint16_t>(std::stoi(token));
		}

		return version;
	}

	class Segment
	{
	public:
		enum Name : std::size_t
		{
			text,
			interpr,
			idata,
			rdata,
			data,
			pdata,
			tls,
			total
		};

		constexpr Segment() noexcept = default;

		constexpr Segment(std::uintptr_t a_proxyBase, std::uintptr_t a_address, std::uintptr_t a_size) noexcept :
			_proxyBase(a_proxyBase),
			_address(a_address),
			_size(a_size)
		{}

		[[nodiscard]] constexpr std::uintptr_t address() const noexcept { return _address; }
		[[nodiscard]] constexpr std::size_t offset() const noexcept { return address() - _proxyBase; }
		[[nodiscard]] constexpr std::size_t size() const noexcept { return _size; }

		[[nodiscard]] void* pointer() const noexcept { return reinterpret_cast<void*>(address()); }

		template <class T>
		[[nodiscard]] T* pointer() const noexcept
		{
			return static_cast<T*>(pointer());
		}

	private:
		std::uintptr_t _proxyBase{ 0 };
		std::uintptr_t _address{ 0 };
		std::size_t _size{ 0 };
	};

	class Module
	{
	public:
		Module(const Module&) = delete;
		Module(Module&&) = delete;

		Module& operator=(const Module&) = delete;
		Module& operator=(Module&&) = delete;

		[[nodiscard]] static Module& get()
		{
			static Module singleton;
			return singleton;
		}

		[[nodiscard]] constexpr std::uintptr_t base() const noexcept { return _base; }
		[[nodiscard]] constexpr std::size_t image_size() const noexcept { return _imageSize; }
		[[nodiscard]] constexpr std::uintptr_t preferred_base() const noexcept { return _preferredBase; }
		[[nodiscard]] stl::zwstring filename() const noexcept { return _filename; }
		[[nodiscard]] constexpr bool is_og() const noexcept { return runtime_family(_version) == RuntimeFamily::kOG; }
		[[nodiscard]] constexpr bool is_ng() const noexcept { return runtime_family(_version) == RuntimeFamily::kNG; }
		[[nodiscard]] constexpr bool is_ae() const noexcept { return runtime_family(_version) == RuntimeFamily::kAE; }
		[[nodiscard]] constexpr Segment segment(Segment::Name a_segment) const noexcept { return _segments[a_segment]; }
		[[nodiscard]] constexpr Version version() const noexcept { return _version; }

		[[nodiscard]] void* pointer() const noexcept { return reinterpret_cast<void*>(base()); }

		template <class T>
		[[nodiscard]] T* pointer() const noexcept
		{
			return static_cast<T*>(pointer());
		}

	private:
		Module()
		{
			const auto getFilename = [&]() {
				return WinAPI::GetEnvironmentVariable(
					ENVIRONMENT.data(),
					_filename.data(),
					static_cast<std::uint32_t>(_filename.size()));
			};

			_filename.resize(getFilename());
			if (const auto result = getFilename();
				result != _filename.size() - 1 ||
				result == 0) {
				_filename = L"Fallout4.exe"sv;
			}

			load();
		}

		~Module() noexcept = default;

		void load()
		{
			auto handle = WinAPI::GetModuleHandle(_filename.c_str());
			if (handle == nullptr) {
				stl::report_and_fail("failed to obtain module handle"sv);
			}
			_base = reinterpret_cast<std::uintptr_t>(handle);
			_natvis = _base;

			load_version();
			load_segments();
		}

		void load_segments();

		void load_version()
		{
			const auto version = get_file_version(_filename);
			if (version) {
				_version = *version;
			} else {
				stl::report_and_fail("failed to obtain file version"sv);
			}
		}

		static constexpr auto ENVIRONMENT = L"F4SE_RUNTIME"sv;

		static constexpr std::array SEGMENTS{
			".text"sv,
			".interpr"sv,
			".idata"sv,
			".rdata"sv,
			".data"sv,
			".pdata"sv,
			".tls"sv
		};

		static inline std::uintptr_t _natvis{ 0 };

		std::wstring _filename;
		std::array<Segment, Segment::total> _segments;
		Version _version;
		std::uintptr_t _base{ 0 };
		std::size_t _imageSize{ 0 };
		std::uintptr_t _preferredBase{ 0 };
	};

	class IDDatabase
	{
	private:
		struct mapping_t
		{
			std::uint64_t id;
			std::uint64_t offset;
		};

	public:
		IDDatabase(const IDDatabase&) = delete;
		IDDatabase(IDDatabase&&) = delete;

		IDDatabase& operator=(const IDDatabase&) = delete;
		IDDatabase& operator=(IDDatabase&&) = delete;

		class Offset2ID
		{
		public:
			using value_type = mapping_t;
			using container_type = std::vector<value_type>;
			using size_type = typename container_type::size_type;
			using const_iterator = typename container_type::const_iterator;
			using const_reverse_iterator = typename container_type::const_reverse_iterator;

			template <class ExecutionPolicy>
			explicit Offset2ID(ExecutionPolicy&& a_policy)  // NOLINT(bugprone-forwarding-reference-overload)
				requires(std::is_execution_policy_v<std::decay_t<ExecutionPolicy>>)
			{
				const auto id2offset = IDDatabase::get().get_id2offset();
				_offset2id.reserve(id2offset.size());
				_offset2id.insert(_offset2id.begin(), id2offset.begin(), id2offset.end());
				std::sort(
					a_policy,
					_offset2id.begin(),
					_offset2id.end(),
					[](auto&& a_lhs, auto&& a_rhs) {
						return a_lhs.offset != a_rhs.offset ?
						         a_lhs.offset < a_rhs.offset :
						         a_lhs.id < a_rhs.id;
					});
			}

			Offset2ID() :
				Offset2ID(std::execution::sequenced_policy{})
			{}

			[[nodiscard]] std::uint64_t operator()(std::size_t a_offset) const
			{
				if (_offset2id.empty()) {
					stl::report_and_fail("data is empty"sv);
				}

				const mapping_t elem{ 0, a_offset };
				const auto it = std::lower_bound(
					_offset2id.begin(),
					_offset2id.end(),
					elem,
					[](auto&& a_lhs, auto&& a_rhs) {
						return a_lhs.offset < a_rhs.offset;
					});
				if (it == _offset2id.end() || it->offset != a_offset) {
					stl::report_and_fail("offset not found"sv);
				}

				return it->id;
			}

			[[nodiscard]] const_iterator begin() const noexcept { return _offset2id.begin(); }
			[[nodiscard]] const_iterator cbegin() const noexcept { return _offset2id.cbegin(); }

			[[nodiscard]] const_iterator end() const noexcept { return _offset2id.end(); }
			[[nodiscard]] const_iterator cend() const noexcept { return _offset2id.cend(); }

			[[nodiscard]] const_reverse_iterator rbegin() const noexcept { return _offset2id.rbegin(); }
			[[nodiscard]] const_reverse_iterator crbegin() const noexcept { return _offset2id.crbegin(); }

			[[nodiscard]] const_reverse_iterator rend() const noexcept { return _offset2id.rend(); }
			[[nodiscard]] const_reverse_iterator crend() const noexcept { return _offset2id.crend(); }

			[[nodiscard]] size_type size() const noexcept { return _offset2id.size(); }

		private:
			container_type _offset2id;
		};

		[[nodiscard]] static IDDatabase& get()
		{
			static IDDatabase singleton;
			return singleton;
		}

		[[nodiscard]] std::size_t id2offset(std::uint64_t a_id) const;
		[[nodiscard]] std::size_t id2offset(const ID& a_id) const;
		[[nodiscard]] std::size_t id2offset(const ID& a_id, const VariantOffset& a_offset) const;
		[[nodiscard]] IDResolveResult resolve(
			std::uint64_t a_id,
			IDResolveMode a_mode = IDResolveMode::kNormal) const;
		[[nodiscard]] IDResolveResult resolve(
			const ID& a_id,
			IDResolveMode a_mode = IDResolveMode::kNormal) const;
		[[nodiscard]] IDValidationSummary validate(
			std::span<const std::uint64_t> a_ids,
			const std::filesystem::path& a_logPath = {},
			IDResolveMode a_mode = IDResolveMode::kPatternsShared) const;

	protected:
		friend class Offset2ID;

		[[nodiscard]] std::span<const mapping_t> get_id2offset();

	private:
		IDDatabase();
		~IDDatabase();

		void load();
		void dump_requested_mapping() const;
		[[nodiscard]] IDResolveResult resolve_impl(
			std::uint64_t a_id,
			IDResolveMode a_mode,
			bool a_trace) const;

		mmio::mapped_file_source _mmap;
		std::unique_ptr<RuntimeDatabase> _runtime;
		std::mutex _mappingLock;
		std::vector<mapping_t> _runtimeMappings;
		std::span<const mapping_t> _id2offset;
	};

	class Offset
	{
	public:
		constexpr Offset() noexcept = default;

		explicit constexpr Offset(std::size_t a_offset) noexcept :
			_offset(a_offset)
		{}

		constexpr Offset& operator=(std::size_t a_offset) noexcept
		{
			_offset = a_offset;
			return *this;
		}

		[[nodiscard]] std::uintptr_t address() const { return base() + offset(); }
		[[nodiscard]] constexpr std::size_t offset() const noexcept { return _offset; }

	private:
		[[nodiscard]] static std::uintptr_t base() { return Module::get().base(); }

		std::size_t _offset{ 0 };
	};

	class ID
	{
	public:
		static constexpr std::uint64_t INVALID_ID = static_cast<std::uint64_t>(-1);

		constexpr ID() noexcept = default;

		explicit constexpr ID(std::uint64_t a_aeID) noexcept :
			_aeID(a_aeID)
		{}

		constexpr ID(std::uint64_t a_ogID, std::uint64_t a_aeID) noexcept :
			_ogID(a_ogID),
			_aeID(a_aeID)
		{}

		constexpr ID(std::uint64_t a_ogID, std::uint64_t a_ngID, std::uint64_t a_aeID) noexcept :
			_ogID(a_ogID),
			_ngID(a_ngID),
			_aeID(a_aeID)
		{}

		constexpr ID& operator=(std::uint64_t a_id) noexcept
		{
			_ogID = INVALID_ID;
			_ngID = INVALID_ID;
			_aeID = a_id;
			return *this;
		}

		[[nodiscard]] std::uintptr_t address() const { return base() + offset(); }
		[[nodiscard]] std::uint64_t id() const noexcept { return id(Module::get().version()); }
		[[nodiscard]] constexpr std::uint64_t id(const Version& a_version) const noexcept
		{
			switch (runtime_family(a_version)) {
			case RuntimeFamily::kOG:
				return og_id();
			case RuntimeFamily::kNG:
				return ng_id();
			case RuntimeFamily::kAE:
			default:
				return ae_id();
			}
		}
		[[nodiscard]] constexpr bool has_og_id() const noexcept { return _ogID != INVALID_ID; }
		[[nodiscard]] constexpr bool has_ng_id() const noexcept { return _ngID != INVALID_ID; }
		[[nodiscard]] constexpr std::uint64_t og_id() const noexcept { return has_og_id() ? _ogID : _aeID; }
		[[nodiscard]] constexpr std::uint64_t ng_id() const noexcept { return has_ng_id() ? _ngID : _aeID; }
		[[nodiscard]] constexpr std::uint64_t ae_id() const noexcept { return _aeID; }
		[[nodiscard]] std::size_t offset() const { return IDDatabase::get().id2offset(*this); }

	private:
		[[nodiscard]] static std::uintptr_t base() { return Module::get().base(); }

		std::uint64_t _ogID{ INVALID_ID };
		std::uint64_t _ngID{ INVALID_ID };
		std::uint64_t _aeID{ INVALID_ID };
	};

	struct AutoOffsetTag
	{};

	inline constexpr AutoOffsetTag AUTO_OFFSET{};

	enum class AutoCallsiteBranch : std::uint8_t
	{
		kCall,
		kJump,
		kCallOrJump
	};

	class AutoCallsite
	{
	public:
		static constexpr std::uint16_t LAST = std::numeric_limits<std::uint16_t>::max() - 1;
		static constexpr std::uint16_t UNIQUE = std::numeric_limits<std::uint16_t>::max();

		constexpr AutoCallsite() noexcept = default;

		explicit constexpr AutoCallsite(
			ID a_target,
			AutoCallsiteBranch a_branch = AutoCallsiteBranch::kCall,
			std::uint16_t a_occurrence = UNIQUE) noexcept :
			_target(a_target),
			_branch(a_branch),
			_occurrence(a_occurrence)
		{}

		[[nodiscard]] constexpr const ID& target() const noexcept { return _target; }
		[[nodiscard]] constexpr AutoCallsiteBranch branch() const noexcept { return _branch; }
		[[nodiscard]] constexpr std::uint16_t occurrence() const noexcept { return _occurrence; }
		[[nodiscard]] constexpr bool valid(const Version& a_version) const noexcept
		{
			return _target.id(a_version) != ID::INVALID_ID &&
			       (_branch == AutoCallsiteBranch::kCall ||
				   _branch == AutoCallsiteBranch::kJump ||
				   _branch == AutoCallsiteBranch::kCallOrJump);
		}

	private:
		ID _target;
		AutoCallsiteBranch _branch{ AutoCallsiteBranch::kCall };
		std::uint16_t _occurrence{ UNIQUE };
	};

	struct AutoCallsiteFactory
	{
		[[nodiscard]] constexpr AutoCallsite operator()(
			ID a_target,
			AutoCallsiteBranch a_branch = AutoCallsiteBranch::kCall,
			std::uint16_t a_occurrence = AutoCallsite::UNIQUE) const noexcept
		{
			return AutoCallsite{ a_target, a_branch, a_occurrence };
		}
	};

	inline constexpr AutoCallsiteFactory AUTO_CALLSITE{};

	[[nodiscard]] constexpr AutoCallsite AUTO_CALLSITE_FIRST(
		ID a_target,
		AutoCallsiteBranch a_branch = AutoCallsiteBranch::kCall) noexcept
	{
		return AutoCallsite{ a_target, a_branch, 0 };
	}

	[[nodiscard]] constexpr AutoCallsite AUTO_CALLSITE_LAST(
		ID a_target,
		AutoCallsiteBranch a_branch = AutoCallsiteBranch::kCall) noexcept
	{
		return AutoCallsite{ a_target, a_branch, AutoCallsite::LAST };
	}

	[[nodiscard]] constexpr AutoCallsite AUTO_CALLSITE_NTH(
		ID a_target,
		std::uint16_t a_occurrence,
		AutoCallsiteBranch a_branch = AutoCallsiteBranch::kCall) noexcept
	{
		return AutoCallsite{ a_target, a_branch, a_occurrence };
	}

	namespace detail
	{
		enum class CallsiteScanStatus : std::uint8_t
		{
			kResolved,
			kNotFound,
			kAmbiguous,
			kInvalid
		};

		struct CallsiteScanResult
		{
			std::optional<std::ptrdiff_t> offset;
			CallsiteScanStatus status{ CallsiteScanStatus::kInvalid };
			std::size_t matches{};
		};

		[[nodiscard]] CallsiteScanResult find_relative_callsite(
			std::span<const std::uint8_t> a_function,
			std::uint32_t a_functionRVA,
			std::uint32_t a_ownerRVA,
			std::uint32_t a_targetRVA,
			AutoCallsiteBranch a_branch,
			std::uint16_t a_occurrence = AutoCallsite::UNIQUE) noexcept;

		[[nodiscard]] std::vector<std::ptrdiff_t> find_relative_callsites(
			std::span<const std::uint8_t> a_function,
			std::uint32_t a_functionRVA,
			std::uint32_t a_ownerRVA,
			std::uint32_t a_targetRVA,
			AutoCallsiteBranch a_branch);
	}

	struct CallsiteResolveResult
	{
		std::vector<std::size_t> rvas;
		std::vector<std::ptrdiff_t> offsets;
		IDResolveStatus status{ IDResolveStatus::kInvalidCallsite };

		[[nodiscard]] explicit operator bool() const noexcept { return !rvas.empty(); }
	};

	[[nodiscard]] CallsiteResolveResult resolve_callsites(
		const ID& a_owner,
		const ID& a_target,
		AutoCallsiteBranch a_branch = AutoCallsiteBranch::kCall);

	template <class T>
	concept VariantOffsetArgument =
		std::integral<std::remove_cvref_t<T>> ||
		std::same_as<std::remove_cvref_t<T>, AutoOffsetTag> ||
		std::same_as<std::remove_cvref_t<T>, AutoCallsite>;

	class VariantOffset
	{
	public:
		constexpr VariantOffset() noexcept = default;

		template <VariantOffsetArgument T>
		explicit constexpr VariantOffset(T a_offset) noexcept :
			_ogOffset(encode(a_offset)),
			_ngOffset(encode(a_offset)),
			_aeOffset(encode(a_offset)),
			_ogCallsite(callsite(a_offset)),
			_ngCallsite(callsite(a_offset)),
			_aeCallsite(callsite(a_offset))
		{}

		template <VariantOffsetArgument OG, VariantOffsetArgument Modern>
		constexpr VariantOffset(OG a_ogOffset, Modern a_modernOffset) noexcept :
			_ogOffset(encode(a_ogOffset)),
			_ngOffset(encode(a_modernOffset)),
			_aeOffset(encode(a_modernOffset)),
			_ogCallsite(callsite(a_ogOffset)),
			_ngCallsite(callsite(a_modernOffset)),
			_aeCallsite(callsite(a_modernOffset))
		{}

		template <VariantOffsetArgument OG, VariantOffsetArgument NG, VariantOffsetArgument AE>
		constexpr VariantOffset(OG a_ogOffset, NG a_ngOffset, AE a_aeOffset) noexcept :
			_ogOffset(encode(a_ogOffset)),
			_ngOffset(encode(a_ngOffset)),
			_aeOffset(encode(a_aeOffset)),
			_ogCallsite(callsite(a_ogOffset)),
			_ngCallsite(callsite(a_ngOffset)),
			_aeCallsite(callsite(a_aeOffset))
		{}

		[[nodiscard]] constexpr std::ptrdiff_t offset(const Version& a_version) const noexcept
		{
			switch (runtime_family(a_version)) {
			case RuntimeFamily::kOG:
				return _ogOffset;
			case RuntimeFamily::kNG:
				return _ngOffset;
			case RuntimeFamily::kAE:
			default:
				return _aeOffset;
			}
		}

		[[nodiscard]] std::ptrdiff_t offset() const noexcept
		{
			return offset(Module::get().version());
		}

		[[nodiscard]] constexpr bool valid(const Version& a_version) const noexcept
		{
			return offset(a_version) != INVALID_VALUE;
		}

		[[nodiscard]] constexpr bool is_auto(const Version& a_version) const noexcept
		{
			return offset(a_version) == AUTO_VALUE;
		}

		[[nodiscard]] bool is_auto() const noexcept
		{
			return is_auto(Module::get().version());
		}

		[[nodiscard]] constexpr std::ptrdiff_t og_offset() const noexcept { return _ogOffset; }
		[[nodiscard]] constexpr std::ptrdiff_t ng_offset() const noexcept { return _ngOffset; }
		[[nodiscard]] constexpr std::ptrdiff_t ae_offset() const noexcept { return _aeOffset; }
		[[nodiscard]] constexpr const AutoCallsite* auto_callsite(const Version& a_version) const noexcept
		{
			const AutoCallsite* selected{};
			switch (runtime_family(a_version)) {
			case RuntimeFamily::kOG:
				selected = std::addressof(_ogCallsite);
				break;
			case RuntimeFamily::kNG:
				selected = std::addressof(_ngCallsite);
				break;
			case RuntimeFamily::kAE:
			default:
				selected = std::addressof(_aeCallsite);
				break;
			}
			return is_auto(a_version) && selected->valid(a_version) ? selected : nullptr;
		}

	private:
		static constexpr auto AUTO_VALUE = std::numeric_limits<std::ptrdiff_t>::min();
		static constexpr auto INVALID_VALUE = AUTO_VALUE + 1;

		template <std::integral T>
		[[nodiscard]] static constexpr std::ptrdiff_t encode(T a_offset) noexcept
		{
			if (!std::in_range<std::ptrdiff_t>(a_offset)) {
				return INVALID_VALUE;
			}
			const auto encoded = static_cast<std::ptrdiff_t>(a_offset);
			return encoded == AUTO_VALUE || encoded == INVALID_VALUE ? INVALID_VALUE : encoded;
		}

		[[nodiscard]] static constexpr std::ptrdiff_t encode(AutoOffsetTag) noexcept
		{
			return AUTO_VALUE;
		}

		[[nodiscard]] static constexpr std::ptrdiff_t encode(AutoCallsite) noexcept
		{
			return AUTO_VALUE;
		}

		template <std::integral T>
		[[nodiscard]] static constexpr AutoCallsite callsite(T) noexcept
		{
			return {};
		}

		[[nodiscard]] static constexpr AutoCallsite callsite(AutoOffsetTag) noexcept
		{
			return {};
		}

		[[nodiscard]] static constexpr AutoCallsite callsite(AutoCallsite a_callsite) noexcept
		{
			return a_callsite;
		}

		std::ptrdiff_t _ogOffset{};
		std::ptrdiff_t _ngOffset{};
		std::ptrdiff_t _aeOffset{};
		AutoCallsite _ogCallsite;
		AutoCallsite _ngCallsite;
		AutoCallsite _aeCallsite;
	};

	template <class T>
	class Relocation
	{
	public:
		using value_type =
			std::conditional_t<
				std::is_member_pointer_v<T> || std::is_function_v<std::remove_pointer_t<T>>,
				std::decay_t<T>,
				T>;

		constexpr Relocation() noexcept = default;

		explicit constexpr Relocation(std::uintptr_t a_address) noexcept :
			_impl{ a_address }
		{}

		explicit Relocation(Offset a_offset) :
			_impl{ a_offset.address() }
		{}

		explicit Relocation(ID a_id) :
			_impl{ a_id.address() }
		{}

		explicit Relocation(ID a_id, std::ptrdiff_t a_offset) :
			_impl{ base() + IDDatabase::get().id2offset(a_id, VariantOffset{ a_offset }) }
		{}

		explicit Relocation(ID a_id, VariantOffset a_offset) :
			_impl{ base() + IDDatabase::get().id2offset(a_id, a_offset) }
		{}

		constexpr Relocation& operator=(std::uintptr_t a_address) noexcept
		{
			_impl = a_address;
			return *this;
		}

		Relocation& operator=(Offset a_offset)
		{
			_impl = a_offset.address();
			return *this;
		}

		Relocation& operator=(ID a_id)
		{
			_impl = a_id.address();
			return *this;
		}

		template <class U = value_type>
		[[nodiscard]] decltype(auto) operator*() const noexcept  //
			requires(std::is_pointer_v<U>)
		{
			return *get();
		}

		template <class U = value_type>
		[[nodiscard]] auto operator->() const noexcept  //
			requires(std::is_pointer_v<U>)
		{
			return get();
		}

		template <class... Args>
		std::invoke_result_t<const value_type&, Args...> operator()(Args&&... a_args) const  //
			noexcept(std::is_nothrow_invocable_v<const value_type&, Args...>)                //
			requires(std::invocable<const value_type&, Args...>)
		{
			return REL::invoke(get(), std::forward<Args>(a_args)...);
		}

		[[nodiscard]] constexpr std::uintptr_t address() const noexcept { return _impl; }
		[[nodiscard]] std::size_t offset() const { return _impl - base(); }

		[[nodiscard]] value_type get() const  //
			noexcept(std::is_nothrow_copy_constructible_v<value_type>)
		{
			assert(_impl != 0);
			return stl::unrestricted_cast<value_type>(_impl);
		}

		template <class U = value_type>
		std::uintptr_t write_vfunc(std::size_t a_idx, std::uintptr_t a_newFunc)  //
			requires(std::same_as<U, std::uintptr_t>)
		{
			const auto addr = address() + (sizeof(void*) * a_idx);
			const auto result = *reinterpret_cast<std::uintptr_t*>(addr);
			safe_write(addr, a_newFunc);
			return result;
		}

		template <class F>
		std::uintptr_t write_vfunc(std::size_t a_idx, F a_newFunc)  //
			requires(std::same_as<value_type, std::uintptr_t>)
		{
			return write_vfunc(a_idx, stl::unrestricted_cast<std::uintptr_t>(a_newFunc));
		}

	private:
		// clang-format off
		[[nodiscard]] static std::uintptr_t base() { return Module::get().base(); }
		// clang-format on

		std::uintptr_t _impl{ 0 };
	};

	template <class T>
	[[nodiscard]] constexpr T Relocate(const Version& a_version, T a_og, T a_ae) noexcept
	{
		return runtime_family(a_version) == RuntimeFamily::kOG ? a_og : a_ae;
	}

	template <class T>
	[[nodiscard]] constexpr T Relocate(
		const Version& a_version,
		T a_og,
		T a_ng,
		T a_ae) noexcept
	{
		switch (runtime_family(a_version)) {
		case RuntimeFamily::kOG:
			return a_og;
		case RuntimeFamily::kNG:
			return a_ng;
		case RuntimeFamily::kAE:
		default:
			return a_ae;
		}
	}

	template <class T>
	[[nodiscard]] T Relocate(T a_og, T a_ae) noexcept
	{
		return Relocate(Module::get().version(), a_og, a_ae);
	}

	template <class T>
	[[nodiscard]] T Relocate(T a_og, T a_ng, T a_ae) noexcept
	{
		return Relocate(Module::get().version(), a_og, a_ng, a_ae);
	}

	template <class T>
	[[nodiscard]] constexpr T RelocateIfNewer(
		const Version& a_current,
		const Version& a_threshold,
		T a_older,
		T a_newer) noexcept
	{
		return a_current < a_threshold ? a_older : a_newer;
	}

	template <class T>
	[[nodiscard]] T RelocateIfNewer(
		const Version& a_threshold,
		T a_older,
		T a_newer) noexcept
	{
		return RelocateIfNewer(Module::get().version(), a_threshold, a_older, a_newer);
	}

	template <class T>
	[[nodiscard]] T& RelocateMember(
		void* a_self,
		const Version& a_version,
		std::ptrdiff_t a_og,
		std::ptrdiff_t a_ae) noexcept
	{
		return *reinterpret_cast<T*>(
			reinterpret_cast<std::uintptr_t>(a_self) + Relocate(a_version, a_og, a_ae));
	}

	template <class T>
	[[nodiscard]] const T& RelocateMember(
		const void* a_self,
		const Version& a_version,
		std::ptrdiff_t a_og,
		std::ptrdiff_t a_ae) noexcept
	{
		return *reinterpret_cast<const T*>(
			reinterpret_cast<std::uintptr_t>(a_self) + Relocate(a_version, a_og, a_ae));
	}

	template <class T>
	[[nodiscard]] T& RelocateMember(
		void* a_self,
		const Version& a_version,
		std::ptrdiff_t a_og,
		std::ptrdiff_t a_ng,
		std::ptrdiff_t a_ae) noexcept
	{
		return *reinterpret_cast<T*>(
			reinterpret_cast<std::uintptr_t>(a_self) + Relocate(a_version, a_og, a_ng, a_ae));
	}

	template <class T>
	[[nodiscard]] const T& RelocateMember(
		const void* a_self,
		const Version& a_version,
		std::ptrdiff_t a_og,
		std::ptrdiff_t a_ng,
		std::ptrdiff_t a_ae) noexcept
	{
		return *reinterpret_cast<const T*>(
			reinterpret_cast<std::uintptr_t>(a_self) + Relocate(a_version, a_og, a_ng, a_ae));
	}

	template <class T>
	[[nodiscard]] T& RelocateMember(void* a_self, std::ptrdiff_t a_og, std::ptrdiff_t a_ae) noexcept
	{
		return RelocateMember<T>(a_self, Module::get().version(), a_og, a_ae);
	}

	template <class T>
	[[nodiscard]] const T& RelocateMember(
		const void* a_self,
		std::ptrdiff_t a_og,
		std::ptrdiff_t a_ae) noexcept
	{
		return RelocateMember<T>(a_self, Module::get().version(), a_og, a_ae);
	}

	template <class T>
	[[nodiscard]] T& RelocateMember(
		void* a_self,
		std::ptrdiff_t a_og,
		std::ptrdiff_t a_ng,
		std::ptrdiff_t a_ae) noexcept
	{
		return RelocateMember<T>(a_self, Module::get().version(), a_og, a_ng, a_ae);
	}

	template <class T>
	[[nodiscard]] const T& RelocateMember(
		const void* a_self,
		std::ptrdiff_t a_og,
		std::ptrdiff_t a_ng,
		std::ptrdiff_t a_ae) noexcept
	{
		return RelocateMember<T>(a_self, Module::get().version(), a_og, a_ng, a_ae);
	}

	template <class T>
	[[nodiscard]] T& RelocateMemberIfNewer(
		void* a_self,
		const Version& a_current,
		const Version& a_threshold,
		std::ptrdiff_t a_older,
		std::ptrdiff_t a_newer) noexcept
	{
		return *reinterpret_cast<T*>(
			reinterpret_cast<std::uintptr_t>(a_self) +
			RelocateIfNewer(a_current, a_threshold, a_older, a_newer));
	}

	template <class T>
	[[nodiscard]] const T& RelocateMemberIfNewer(
		const void* a_self,
		const Version& a_current,
		const Version& a_threshold,
		std::ptrdiff_t a_older,
		std::ptrdiff_t a_newer) noexcept
	{
		return *reinterpret_cast<const T*>(
			reinterpret_cast<std::uintptr_t>(a_self) +
			RelocateIfNewer(a_current, a_threshold, a_older, a_newer));
	}

	template <class T>
	[[nodiscard]] T& RelocateMemberIfNewer(
		void* a_self,
		const Version& a_threshold,
		std::ptrdiff_t a_older,
		std::ptrdiff_t a_newer) noexcept
	{
		return RelocateMemberIfNewer<T>(
			a_self,
			Module::get().version(),
			a_threshold,
			a_older,
			a_newer);
	}

	template <class T>
	[[nodiscard]] const T& RelocateMemberIfNewer(
		const void* a_self,
		const Version& a_threshold,
		std::ptrdiff_t a_older,
		std::ptrdiff_t a_newer) noexcept
	{
		return RelocateMemberIfNewer<T>(
			a_self,
			Module::get().version(),
			a_threshold,
			a_older,
			a_newer);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtual(
		const Version& a_version,
		std::ptrdiff_t a_ogVtableOffset,
		std::ptrdiff_t a_aeVtableOffset,
		std::size_t a_ogIndex,
		std::size_t a_aeIndex,
		This* a_self,
		Args&&... a_args)
	{
		const auto vtableOffset = Relocate(a_version, a_ogVtableOffset, a_aeVtableOffset);
		const auto index = Relocate(a_version, a_ogIndex, a_aeIndex);
		const auto address = reinterpret_cast<std::uintptr_t>(a_self) + vtableOffset;
		const auto vtable = *reinterpret_cast<const std::uintptr_t* const*>(address);
		return Relocation<Fn>{ vtable[index] }(a_self, std::forward<Args>(a_args)...);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtual(
		std::ptrdiff_t a_ogVtableOffset,
		std::ptrdiff_t a_aeVtableOffset,
		std::size_t a_ogIndex,
		std::size_t a_aeIndex,
		This* a_self,
		Args&&... a_args)
	{
		return RelocateVirtual<Fn>(
			Module::get().version(),
			a_ogVtableOffset,
			a_aeVtableOffset,
			a_ogIndex,
			a_aeIndex,
			a_self,
			std::forward<Args>(a_args)...);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtual(
		const Version& a_version,
		std::size_t a_ogIndex,
		std::size_t a_aeIndex,
		This* a_self,
		Args&&... a_args)
	{
		return RelocateVirtual<Fn>(
			a_version,
			0,
			0,
			a_ogIndex,
			a_aeIndex,
			a_self,
			std::forward<Args>(a_args)...);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtual(
		std::size_t a_ogIndex,
		std::size_t a_aeIndex,
		This* a_self,
		Args&&... a_args)
	{
		return RelocateVirtual<Fn>(
			Module::get().version(),
			a_ogIndex,
			a_aeIndex,
			a_self,
			std::forward<Args>(a_args)...);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtualIfNewer(
		const Version& a_current,
		const Version& a_threshold,
		std::ptrdiff_t a_olderVtableOffset,
		std::ptrdiff_t a_newerVtableOffset,
		std::size_t a_olderIndex,
		std::size_t a_newerIndex,
		This* a_self,
		Args&&... a_args)
	{
		const auto vtableOffset = RelocateIfNewer(
			a_current,
			a_threshold,
			a_olderVtableOffset,
			a_newerVtableOffset);
		const auto index = RelocateIfNewer(a_current, a_threshold, a_olderIndex, a_newerIndex);
		const auto address = reinterpret_cast<std::uintptr_t>(a_self) + vtableOffset;
		const auto vtable = *reinterpret_cast<const std::uintptr_t* const*>(address);
		return Relocation<Fn>{ vtable[index] }(a_self, std::forward<Args>(a_args)...);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtualIfNewer(
		const Version& a_threshold,
		std::ptrdiff_t a_olderVtableOffset,
		std::ptrdiff_t a_newerVtableOffset,
		std::size_t a_olderIndex,
		std::size_t a_newerIndex,
		This* a_self,
		Args&&... a_args)
	{
		return RelocateVirtualIfNewer<Fn>(
			Module::get().version(),
			a_threshold,
			a_olderVtableOffset,
			a_newerVtableOffset,
			a_olderIndex,
			a_newerIndex,
			a_self,
			std::forward<Args>(a_args)...);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtualIfNewer(
		const Version& a_current,
		const Version& a_threshold,
		std::size_t a_olderIndex,
		std::size_t a_newerIndex,
		This* a_self,
		Args&&... a_args)
	{
		return RelocateVirtualIfNewer<Fn>(
			a_current,
			a_threshold,
			0,
			0,
			a_olderIndex,
			a_newerIndex,
			a_self,
			std::forward<Args>(a_args)...);
	}

	template <class Fn, class This, class... Args>
	decltype(auto) RelocateVirtualIfNewer(
		const Version& a_threshold,
		std::size_t a_olderIndex,
		std::size_t a_newerIndex,
		This* a_self,
		Args&&... a_args)
	{
		return RelocateVirtualIfNewer<Fn>(
			Module::get().version(),
			a_threshold,
			a_olderIndex,
			a_newerIndex,
			a_self,
			std::forward<Args>(a_args)...);
	}
}

#undef REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE
#undef REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER
#undef REL_MAKE_MEMBER_FUNCTION_NON_POD_TYPE_HELPER_IMPL

#undef REL_MAKE_MEMBER_FUNCTION_POD_TYPE
#undef REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER
#undef REL_MAKE_MEMBER_FUNCTION_POD_TYPE_HELPER_IMPL
