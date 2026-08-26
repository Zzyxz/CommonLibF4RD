#include "RE/NetImmerse/NiPoint3.h"

namespace RE
{
	bool NiPoint3::operator==(const NiPoint3& a_rhs) const noexcept
	{
		return x == a_rhs.x && y == a_rhs.y && z == a_rhs.z;
	}

	bool NiPoint3::operator!=(const NiPoint3& a_rhs) const noexcept
	{
		return !operator==(a_rhs);
	}

	NiPoint3 NiPoint3::operator+(const NiPoint3& a_rhs) const noexcept
	{
		return { x + a_rhs.x, y + a_rhs.y, z + a_rhs.z };
	}

	NiPoint3 NiPoint3::operator-(const NiPoint3& a_rhs) const noexcept
	{
		return { x - a_rhs.x, y - a_rhs.y, z - a_rhs.z };
	}

	float NiPoint3::operator*(const NiPoint3& a_rhs) const noexcept
	{
		return Dot(a_rhs);
	}

	NiPoint3 NiPoint3::operator*(float a_scalar) const noexcept
	{
		return { x * a_scalar, y * a_scalar, z * a_scalar };
	}

	NiPoint3 NiPoint3::operator/(float a_scalar) const noexcept
	{
		return { x / a_scalar, y / a_scalar, z / a_scalar };
	}

	NiPoint3 NiPoint3::operator-() const noexcept
	{
		return { -x, -y, -z };
	}

	NiPoint3& NiPoint3::operator+=(const NiPoint3& a_rhs) noexcept
	{
		x += a_rhs.x;
		y += a_rhs.y;
		z += a_rhs.z;
		return *this;
	}

	NiPoint3& NiPoint3::operator-=(const NiPoint3& a_rhs) noexcept
	{
		x -= a_rhs.x;
		y -= a_rhs.y;
		z -= a_rhs.z;
		return *this;
	}

	NiPoint3& NiPoint3::operator*=(const NiPoint3& a_rhs) noexcept
	{
		x *= a_rhs.x;
		y *= a_rhs.y;
		z *= a_rhs.z;
		return *this;
	}

	NiPoint3& NiPoint3::operator/=(const NiPoint3& a_rhs) noexcept
	{
		x /= a_rhs.x;
		y /= a_rhs.y;
		z /= a_rhs.z;
		return *this;
	}

	NiPoint3& NiPoint3::operator*=(float a_scalar) noexcept
	{
		x *= a_scalar;
		y *= a_scalar;
		z *= a_scalar;
		return *this;
	}

	NiPoint3& NiPoint3::operator/=(float a_scalar) noexcept
	{
		return operator*=(1.0F / a_scalar);
	}

	NiPoint3 NiPoint3::Cross(const NiPoint3& a_point) const noexcept
	{
		return {
			y * a_point.z - z * a_point.y,
			z * a_point.x - x * a_point.z,
			x * a_point.y - y * a_point.x
		};
	}

	float NiPoint3::Dot(const NiPoint3& a_point) const noexcept
	{
		return x * a_point.x + y * a_point.y + z * a_point.z;
	}

	float NiPoint3::GetDistance(const NiPoint3& a_point) const noexcept
	{
		return std::sqrt(GetSquaredDistance(a_point));
	}

	float NiPoint3::GetSquaredDistance(const NiPoint3& a_point) const noexcept
	{
		const auto dx = a_point.x - x;
		const auto dy = a_point.y - y;
		const auto dz = a_point.z - z;
		return dx * dx + dy * dy + dz * dz;
	}

	float NiPoint3::GetZAngleFromVector()
	{
		using func_t = decltype(&NiPoint3::GetZAngleFromVector);
		REL::Relocation<func_t> func{ REL::ID(1450064, 2269788) };
		return func(this);
	}

	float NiPoint3::Length() const noexcept
	{
		return std::sqrt(SqrLength());
	}

	float NiPoint3::SqrLength() const noexcept
	{
		return x * x + y * y + z * z;
	}

	NiPoint3 NiPoint3::UnitCross(const NiPoint3& a_point) const noexcept
	{
		auto result = Cross(a_point);
		result.Unitize();
		return result;
	}

	float NiPoint3::Unitize() noexcept
	{
		auto length = Length();
		if (length > std::numeric_limits<float>::epsilon()) {
			operator/=(length);
		} else {
			x = 0.0F;
			y = 0.0F;
			z = 0.0F;
			length = 0.0F;
		}
		return length;
	}
}
