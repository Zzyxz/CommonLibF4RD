#pragma once

namespace RE::msvc
{
	class type_info;

	template <class>
	class function;

	template <class>
	class function_ngae;

	// class std::_Func_class
	template <class R, class... Args>
	class function<R(Args...)>
	{
	private:
		class proxy_t;
		template <class F>
		class proxy_impl;

	public:
		using result_type = R;

		template <class F>
		[[nodiscard]] static function make(F&& a_function)
		{
			using proxy_type = proxy_impl<std::decay_t<F>>;
			return function(new proxy_type(std::forward<F>(a_function)));
		}

		function(const function&) = delete;
		function(function&&) = delete;
		function& operator=(const function&) = delete;
		function& operator=(function&&) = delete;

		~function()
		{
			if (_fn) {
				_fn->delete_this(!local());
			}
		}

		[[nodiscard]] explicit operator bool() const noexcept { return good(); }

		result_type operator()(Args&&... a_args) const
		{
			assert(good());
			return _fn->do_call(std::forward<Args>(a_args)...);
		}

	private:
		// class std::_Func_base
		class __declspec(novtable) proxy_t
		{
		public:
			// add
			virtual proxy_t* copy(void*) = 0;                  // 00
			virtual proxy_t* move(void*) = 0;                  // 01
			virtual result_type do_call(Args&&...) = 0;        // 02
			virtual const type_info& target_type() const = 0;  // 03
			virtual void delete_this(bool) = 0;                // 04
			virtual ~proxy_t() = default;                      // 05
			virtual const void* get() const = 0;               // 06
		};

		template <class F>
		class proxy_impl final : public proxy_t
		{
		public:
			explicit proxy_impl(F&& a_function) : function_(std::move(a_function)) {}
			explicit proxy_impl(const F& a_function) : function_(a_function) {}

			// The caller always passes its small buffer; like MSVC, only place the callable there when it fits.
			[[nodiscard]] static constexpr bool fits_local() noexcept
			{
				return sizeof(proxy_impl) <= sizeof(function::_storage) &&
				       alignof(proxy_impl) <= alignof(std::max_align_t) &&
				       std::is_nothrow_move_constructible_v<F>;
			}

			proxy_t* copy(void* a_storage) override
			{
				return a_storage && fits_local() ? new (a_storage) proxy_impl(function_) : new proxy_impl(function_);
			}

			proxy_t* move(void* a_storage) override
			{
				return a_storage && fits_local() ? new (a_storage) proxy_impl(std::move(function_)) : new proxy_impl(std::move(function_));
			}

			result_type do_call(Args&&... a_args) override
			{
				return function_(std::forward<Args>(a_args)...);
			}

			const type_info& target_type() const override
			{
				return reinterpret_cast<const type_info&>(typeid(F));
			}

			void delete_this(bool a_heap) override
			{
				if (a_heap) delete this;
				else this->~proxy_impl();
			}

			const void* get() const override { return std::addressof(function_); }

		private:
			F function_;
		};

		explicit function(proxy_t* a_function) noexcept : _fn(a_function) {}

		[[nodiscard]] bool good() const noexcept { return _fn != nullptr; }
		[[nodiscard]] bool local() const noexcept
		{
			const auto functionAddress = reinterpret_cast<std::uintptr_t>(_fn);
			const auto storageAddress = reinterpret_cast<std::uintptr_t>(std::addressof(_storage));
			return functionAddress >= storageAddress && functionAddress < storageAddress + sizeof(_storage);
		}

		std::aligned_storage_t<3 * sizeof(void*), alignof(long double)> _storage{};  // 00
		proxy_t* _fn;                                                              // 18
	};
	static_assert(sizeof(function<void()>) == 0x20);

	// Fallout 4 1.10.980 and later use the newer MSVC std::function ABI.  The
	// callable proxy moved from 0x18 to 0x38; passing the legacy layout to game
	// code makes it dereference captured data as a vtable pointer.
	template <class R, class... Args>
	class function_ngae<R(Args...)>
	{
	private:
		class proxy_t;
		template <class F>
		class proxy_impl;

	public:
		using result_type = R;

		template <class F>
		[[nodiscard]] static function_ngae make(F&& a_function)
		{
			using proxy_type = proxy_impl<std::decay_t<F>>;
			return function_ngae(new proxy_type(std::forward<F>(a_function)));
		}

		function_ngae(const function_ngae&) = delete;
		function_ngae(function_ngae&&) = delete;
		function_ngae& operator=(const function_ngae&) = delete;
		function_ngae& operator=(function_ngae&&) = delete;

		~function_ngae()
		{
			if (_fn) {
				_fn->delete_this(!local());
			}
		}

		[[nodiscard]] explicit operator bool() const noexcept { return _fn != nullptr; }

		result_type operator()(Args&&... a_args) const
		{
			assert(_fn != nullptr);
			return _fn->do_call(std::forward<Args>(a_args)...);
		}

	private:
		class __declspec(novtable) proxy_t
		{
		public:
			virtual proxy_t* copy(void*) = 0;
			virtual proxy_t* move(void*) = 0;
			virtual result_type do_call(Args&&...) = 0;
			virtual const type_info& target_type() const = 0;
			virtual void delete_this(bool) = 0;
			virtual ~proxy_t() = default;
			virtual const void* get() const = 0;
		};

		template <class F>
		class proxy_impl final : public proxy_t
		{
		public:
			explicit proxy_impl(F&& a_function) : function_(std::move(a_function)) {}
			explicit proxy_impl(const F& a_function) : function_(a_function) {}

			// The caller always passes its small buffer; like MSVC, only place the callable there when it fits.
			[[nodiscard]] static constexpr bool fits_local() noexcept
			{
				return sizeof(proxy_impl) <= sizeof(function_ngae::_storage) &&
				       alignof(proxy_impl) <= alignof(std::max_align_t) &&
				       std::is_nothrow_move_constructible_v<F>;
			}

			proxy_t* copy(void* a_storage) override
			{
				return a_storage && fits_local() ? new (a_storage) proxy_impl(function_) : new proxy_impl(function_);
			}

			proxy_t* move(void* a_storage) override
			{
				return a_storage && fits_local() ? new (a_storage) proxy_impl(std::move(function_)) : new proxy_impl(std::move(function_));
			}

			result_type do_call(Args&&... a_args) override
			{
				return function_(std::forward<Args>(a_args)...);
			}

			const type_info& target_type() const override
			{
				return reinterpret_cast<const type_info&>(typeid(F));
			}

			void delete_this(bool a_heap) override
			{
				if (a_heap) delete this;
				else this->~proxy_impl();
			}

			const void* get() const override { return std::addressof(function_); }

		private:
			F function_;
		};

		explicit function_ngae(proxy_t* a_function) noexcept : _fn(a_function) {}

		[[nodiscard]] bool local() const noexcept
		{
			const auto functionAddress = reinterpret_cast<std::uintptr_t>(_fn);
			const auto storageAddress = reinterpret_cast<std::uintptr_t>(std::addressof(_storage));
			return functionAddress >= storageAddress && functionAddress < storageAddress + sizeof(_storage);
		}

		std::byte _storage[0x38]{};
		proxy_t* _fn;
	};
	static_assert(sizeof(function_ngae<void()>) == 0x40);
}
