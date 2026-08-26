#pragma once

#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	class hkVector4f
	{
	public:
		hkVector4f() noexcept :
			quad(_mm_setzero_ps())
		{}

		hkVector4f(const NiPoint3& a_point) noexcept :
			quad(_mm_set_ps(0.0F, a_point.z, a_point.y, a_point.x))
		{}

		hkVector4f(float a_x, float a_y, float a_z, float a_w = 0.0F) noexcept :
			quad(_mm_set_ps(a_w, a_z, a_y, a_x))
		{}

		hkVector4f(const hkVector4f&) noexcept = default;
		hkVector4f& operator=(const hkVector4f&) noexcept = default;

		[[nodiscard]] hkVector4f operator+(const hkVector4f& a_rhs) const noexcept
		{
			return { x + a_rhs.x, y + a_rhs.y, z + a_rhs.z };
		}

		hkVector4f& operator+=(const hkVector4f& a_rhs) noexcept
		{
			quad = _mm_add_ps(quad, a_rhs.quad);
			return *this;
		}

		[[nodiscard]] hkVector4f operator-(const hkVector4f& a_rhs) const noexcept
		{
			return { x - a_rhs.x, y - a_rhs.y, z - a_rhs.z };
		}

		[[nodiscard]] hkVector4f operator-() const noexcept
		{
			return { -x, -y, -z };
		}

		hkVector4f& operator-=(const hkVector4f& a_rhs) noexcept
		{
			quad = _mm_sub_ps(quad, a_rhs.quad);
			return *this;
		}

		[[nodiscard]] hkVector4f operator*(float a_scalar) const noexcept
		{
			return { x * a_scalar, y * a_scalar, z * a_scalar, w * a_scalar };
		}

		hkVector4f& operator*=(float a_scalar) noexcept
		{
			quad = _mm_mul_ps(quad, _mm_set1_ps(a_scalar));
			return *this;
		}

		[[nodiscard]] hkVector4f operator*(const hkVector4f& a_rhs) const noexcept
		{
			return { x * a_rhs.x, y * a_rhs.y, z * a_rhs.z, w * a_rhs.w };
		}

		hkVector4f& operator*=(const hkVector4f& a_rhs) noexcept
		{
			quad = _mm_mul_ps(quad, a_rhs.quad);
			return *this;
		}

		[[nodiscard]] hkVector4f operator/(float a_scalar) const noexcept
		{
			return { x / a_scalar, y / a_scalar, z / a_scalar };
		}

		hkVector4f& operator/=(float a_scalar) noexcept
		{
			quad = _mm_div_ps(quad, _mm_set1_ps(a_scalar));
			return *this;
		}

		[[nodiscard]] hkVector4f operator/(const hkVector4f& a_rhs) const noexcept
		{
			return { x / a_rhs.x, y / a_rhs.y, z / a_rhs.z, w / a_rhs.w };
		}

		hkVector4f& operator/=(const hkVector4f& a_rhs) noexcept
		{
			quad = _mm_div_ps(quad, a_rhs.quad);
			return *this;
		}

		[[nodiscard]] float Length() const noexcept
		{
			return std::sqrt(x * x + y * y + z * z);
		}

		hkVector4f& Normalize() noexcept
		{
			const auto length = Length();
			if (length == 0.0F) {
				x = 0.0F;
				y = 0.0F;
				z = 0.0F;
			} else {
				x /= length;
				y /= length;
				z /= length;
			}
			return *this;
		}

		[[nodiscard]] hkVector4f GetNormalized() const noexcept
		{
			auto normalized = *this;
			normalized.Normalize();
			return normalized;
		}

		[[nodiscard]] float Dot(const hkVector4f& a_rhs) const noexcept
		{
			return x * a_rhs.x + y * a_rhs.y + z * a_rhs.z;
		}

		[[nodiscard]] operator NiPoint3() const noexcept
		{
			return { x, y, z };
		}

		union
		{
			__m128 quad;  // 00
			struct
			{
				float x;
				float y;
				float z;
				float w;
			};
		};
	};
	static_assert(sizeof(hkVector4f) == 0x10);
}
