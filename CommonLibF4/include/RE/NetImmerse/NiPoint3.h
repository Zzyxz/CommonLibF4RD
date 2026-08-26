#pragma once

namespace RE
{
	class NiPoint3
	{
	public:
		using value_type = float;
		using size_type = std::size_t;
		using reference = value_type&;
		using const_reference = const value_type&;
		using pointer = value_type*;
		using const_pointer = const value_type*;

		[[nodiscard]] reference operator[](size_type a_pos) noexcept
		{
			assert(a_pos < 3);
			return reinterpret_cast<pointer>(std::addressof(x))[a_pos];
		}

		[[nodiscard]] const_reference operator[](size_type a_pos) const noexcept
		{
			assert(a_pos < 3);
			return reinterpret_cast<const_pointer>(std::addressof(x))[a_pos];
		}

		[[nodiscard]] bool operator==(const NiPoint3& a_rhs) const noexcept;
		[[nodiscard]] bool operator!=(const NiPoint3& a_rhs) const noexcept;
		[[nodiscard]] NiPoint3 operator+(const NiPoint3& a_rhs) const noexcept;
		[[nodiscard]] NiPoint3 operator-(const NiPoint3& a_rhs) const noexcept;
		[[nodiscard]] float operator*(const NiPoint3& a_rhs) const noexcept;
		[[nodiscard]] NiPoint3 operator*(float a_scalar) const noexcept;
		[[nodiscard]] NiPoint3 operator/(float a_scalar) const noexcept;
		[[nodiscard]] NiPoint3 operator-() const noexcept;
		NiPoint3& operator+=(const NiPoint3& a_rhs) noexcept;
		NiPoint3& operator-=(const NiPoint3& a_rhs) noexcept;
		NiPoint3& operator*=(const NiPoint3& a_rhs) noexcept;
		NiPoint3& operator/=(const NiPoint3& a_rhs) noexcept;
		NiPoint3& operator*=(float a_scalar) noexcept;
		NiPoint3& operator/=(float a_scalar) noexcept;

		[[nodiscard]] NiPoint3 Cross(const NiPoint3& a_point) const noexcept;
		[[nodiscard]] float Dot(const NiPoint3& a_point) const noexcept;
		[[nodiscard]] float GetDistance(const NiPoint3& a_point) const noexcept;
		[[nodiscard]] float GetSquaredDistance(const NiPoint3& a_point) const noexcept;
		[[nodiscard]] float GetZAngleFromVector();
		[[nodiscard]] float Length() const noexcept;
		[[nodiscard]] float SqrLength() const noexcept;
		[[nodiscard]] NiPoint3 UnitCross(const NiPoint3& a_point) const noexcept;
		float Unitize() noexcept;

		// members
		value_type x{ 0.0F };  // 0
		value_type y{ 0.0F };  // 4
		value_type z{ 0.0F };  // 8
	};
	static_assert(sizeof(NiPoint3) == 0xC);

	class alignas(0x10) NiPoint3A :
		public NiPoint3
	{
	public:
	};
	static_assert(sizeof(NiPoint3A) == 0x10);
}
