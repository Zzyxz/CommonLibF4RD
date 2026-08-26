#pragma once

namespace F4SE::WinAPI
{
	inline constexpr auto(MEM_RELEASE){ static_cast<std::uint32_t>(0x00008000) };
	inline constexpr auto(PAGE_EXECUTE_READWRITE){ static_cast<std::uint32_t>(0x40) };

	struct CRITICAL_SECTION
	{
	public:
		// members
		void* DebugInfo;              // 00
		std::int32_t LockCount;       // 08
		std::int32_t RecursionCount;  // 0C
		void* OwningThread;           // 10
		void* LockSemaphore;          // 18
		std::uint64_t* SpinCount;     // 20
	};
	static_assert(sizeof(CRITICAL_SECTION) == 0x28);

	[[nodiscard]] bool(CloseHandle)(void* a_handle) noexcept;

	[[nodiscard]] void*(CreateFileMapping)(
		std::size_t a_size,
		const wchar_t* a_name) noexcept;

	[[nodiscard]] void*(CreateMutex)(const wchar_t* a_name) noexcept;

	[[nodiscard]] void*(GetCurrentModule)() noexcept;

	[[nodiscard]] void*(GetCurrentProcess)() noexcept;

	[[nodiscard]] std::uint32_t(GetCurrentProcessID)() noexcept;

	[[nodiscard]] std::uint32_t(GetCurrentThreadID)() noexcept;

	[[nodiscard]] std::uint32_t(GetEnvironmentVariable)(
		const char* a_name,
		char* a_buffer,
		std::uint32_t a_size) noexcept;

	[[nodiscard]] std::uint32_t(GetEnvironmentVariable)(
		const wchar_t* a_name,
		wchar_t* a_buffer,
		std::uint32_t a_size) noexcept;

	[[nodiscard]] bool(GetFileVersionInfo)(
		const char* a_filename,
		std::uint32_t a_handle,
		std::uint32_t a_len,
		void* a_data) noexcept;

	[[nodiscard]] bool(GetFileVersionInfo)(
		const wchar_t* a_filename,
		std::uint32_t a_handle,
		std::uint32_t a_len,
		void* a_data) noexcept;

	[[nodiscard]] std::uint32_t(GetFileVersionInfoSize)(
		const char* a_filename,
		std::uint32_t* a_handle) noexcept;

	[[nodiscard]] std::uint32_t(GetFileVersionInfoSize)(
		const wchar_t* a_filename,
		std::uint32_t* a_handle) noexcept;

	[[nodiscard]] std::size_t(GetMaxPath)() noexcept;

	[[nodiscard]] std::uint32_t(GetModuleFileName)(
		void* a_module,
		char* a_filename,
		std::uint32_t a_size) noexcept;

	[[nodiscard]] std::uint32_t(GetModuleFileName)(
		void* a_module,
		wchar_t* a_filename,
		std::uint32_t a_size) noexcept;

	[[nodiscard]] void*(GetModuleHandle)(const char* a_moduleName) noexcept;

	[[nodiscard]] void*(GetModuleHandle)(const wchar_t* a_moduleName) noexcept;

	[[nodiscard]] void*(GetProcAddress)(void* a_module,
		const char* a_procName) noexcept;

	[[nodiscard]] void*(MapViewOfFile)(
		void* a_mapping,
		std::size_t a_size) noexcept;

	std::int32_t(MessageBox)(
		void* a_wnd,
		const char* a_text,
		const char* a_caption,
		unsigned int a_type) noexcept;

	std::int32_t(MessageBox)(
		void* a_wnd,
		const wchar_t* a_text,
		const wchar_t* a_caption,
		unsigned int a_type) noexcept;

	void(OutputDebugString)(
		const char* a_outputString) noexcept;

	void(OutputDebugString)(
		const wchar_t* a_outputString) noexcept;

	[[nodiscard]] bool(ReleaseMutex)(void* a_mutex) noexcept;

	[[noreturn]] void(TerminateProcess)(
		void* a_process,
		unsigned int a_exitCode) noexcept;

	[[nodiscard]] void*(TlsGetValue)(std::uint32_t a_tlsIndex) noexcept;

	bool(TlsSetValue)(
		std::uint32_t a_tlsIndex,
		void* a_tlsValue) noexcept;

	[[nodiscard]] bool(UnmapViewOfFile)(const void* a_address) noexcept;

	bool(VirtualFree)(
		void* a_address,
		std::size_t a_size,
		std::uint32_t a_freeType) noexcept;

	[[nodiscard]] bool(VerQueryValue)(
		const void* a_block,
		const char* a_subBlock,
		void** a_buffer,
		unsigned int* a_len) noexcept;

	[[nodiscard]] bool(VerQueryValue)(
		const void* a_block,
		const wchar_t* a_subBlock,
		void** a_buffer,
		unsigned int* a_len) noexcept;

	[[nodiscard]] bool(VirtualProtect)(
		void* a_address,
		std::size_t a_size,
		std::uint32_t a_newProtect,
		std::uint32_t* a_oldProtect) noexcept;

	[[nodiscard]] bool(WaitForMutex)(void* a_mutex) noexcept;
}

namespace RE
{
	struct _FILETIME  // NOLINT(bugprone-reserved-identifier)
	{
	public:
		// members
		std::uint32_t dwLowDateTime;   // 00
		std::uint32_t dwHighDateTime;  // 04
	};
	static_assert(sizeof(_FILETIME) == 0x8);
}
