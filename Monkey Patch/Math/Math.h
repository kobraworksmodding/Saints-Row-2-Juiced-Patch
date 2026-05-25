#pragma once
#include <chrono>
#include <safetyhook.hpp>
#include <cmath>

#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

template <typename T>
    requires std::is_floating_point_v<T>
T NormalizeRange(T value, T min, T max)
{
    return (std::clamp(value, min, max) - min) / (max - min);
}

template <typename T>
    requires std::is_floating_point_v<T>
T DenormalizeRange(T value, T min, T max)
{
    return min + (std::clamp(value, static_cast<T>(0), static_cast<T>(1)) * (max - min));
}

struct vector2 {
    float x;
    float y;

    // Constructors
    vector2() : x(0.0f), y(0.0f) {}
    vector2(float x_, float y_) : x(x_), y(y_) {}
    vector2(const vector2& other) : x(other.x), y(other.y) {}

    // Basic arithmetic operators
    vector2 operator+(const vector2& other) const {
        return vector2(x + other.x, y + other.y);
    }

    vector2 operator-(const vector2& other) const {
        return vector2(x - other.x, y - other.y);
    }

    vector2 operator*(float scalar) const {
        return vector2(x * scalar, y * scalar);
    }

    vector2 operator*(const vector2& other) const {
        return vector2(x * other.x, y * other.y);
    }

    vector2 operator/(float scalar) const {
        if (scalar != 0.0f) {
            return vector2(x / scalar, y / scalar);
        }
        return vector2(0.0f, 0.0f);
    }

    vector2 operator/(const vector2& other) const {
        return vector2(
            other.x != 0.0f ? x / other.x : 0.0f,
            other.y != 0.0f ? y / other.y : 0.0f
        );
    }

    // Unary operators
    vector2 operator-() const {
        return vector2(-x, -y);
    }

    vector2 operator+() const {
        return *this;
    }

    // Assignment operators
    vector2& operator=(const vector2& other) {
        if (this != &other) {
            x = other.x;
            y = other.y;
        }
        return *this;
    }

    vector2& operator+=(const vector2& other) {
        x += other.x;
        y += other.y;
        return *this;
    }

    vector2& operator-=(const vector2& other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }

    vector2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }

    vector2& operator*=(const vector2& other) {
        x *= other.x;
        y *= other.y;
        return *this;
    }

    vector2& operator/=(float scalar) {
        if (scalar != 0.0f) {
            x /= scalar;
            y /= scalar;
        }
        return *this;
    }

    vector2& operator/=(const vector2& other) {
        x = other.x != 0.0f ? x / other.x : 0.0f;
        y = other.y != 0.0f ? y / other.y : 0.0f;
        return *this;
    }

    // Comparison operators
    bool operator==(const vector2& other) const {
        const float epsilon = 1e-6f;
        return (std::abs(x - other.x) < epsilon &&
            std::abs(y - other.y) < epsilon);
    }

    bool operator!=(const vector2& other) const {
        return !(*this == other);
    }

    bool operator<(const vector2& other) const {
        return (x < other.x) || (x == other.x && y < other.y);
    }

    bool operator>(const vector2& other) const {
        return other < *this;
    }

    bool operator<=(const vector2& other) const {
        return !(other < *this);
    }

    bool operator>=(const vector2& other) const {
        return !(*this < other);
    }

    // Vector operations
    float dot(const vector2& other) const {
        return x * other.x + y * other.y;
    }

    // Cross product in 2D returns scalar (z-component of 3D cross product)
    float cross(const vector2& other) const {
        return x * other.y - y * other.x;
    }

    float magnitude() const {
        return std::sqrt(x * x + y * y);
    }

    float length() const {
        return magnitude();
    }

    float magnitudeSquared() const {
        return x * x + y * y;
    }

    float lengthSquared() const {
        return magnitudeSquared();
    }

    vector2 normalized() const {
        float mag = magnitude();
        if (mag > 0.0f) {
            return *this / mag;
        }
        return vector2(0.0f, 0.0f);
    }

    void normalize() {
        float mag = magnitude();
        if (mag > 0.0f) {
            *this /= mag;
        }
    }

    // Set to unit length, return original length
    float normalizeAndGetLength() {
        float mag = magnitude();
        if (mag > 0.0f) {
            *this /= mag;
        }
        return mag;
    }

    float distance(const vector2& other) const {
        return (*this - other).magnitude();
    }

    float distanceSquared(const vector2& other) const {
        return (*this - other).magnitudeSquared();
    }

    // Linear interpolation
    vector2 lerp(const vector2& other, float t) const {
        return *this + (other - *this) * t;
    }

    // Smooth interpolation (cubic)
    vector2 smoothstep(const vector2& other, float t) const {
        t = t * t * (3.0f - 2.0f * t);
        return lerp(other, t);
    }

    // Spherical linear interpolation (for unit vectors)
    vector2 slerp(const vector2& other, float t) const {
        float dot_product = dot(other);
        dot_product = (std::max)(-1.0f, (std::min)(1.0f, dot_product));

        float theta = std::acos(dot_product) * t;
        vector2 relative = (other - *this * dot_product).normalized();

        return (*this * std::cos(theta)) + (relative * std::sin(theta));
    }

    // Reflect vector across normal
    vector2 reflect(const vector2& normal) const {
        return *this - normal * (2.0f * this->dot(normal));
    }

    // Project this vector onto another vector
    vector2 project(const vector2& onto) const {
        float mag_sq = onto.magnitudeSquared();
        if (mag_sq > 0.0f) {
            return onto * (this->dot(onto) / mag_sq);
        }
        return vector2::zero();
    }

    // Reject this vector from another vector (orthogonal component)
    vector2 reject(const vector2& from) const {
        return *this - project(from);
    }

    // Get perpendicular vector (90 degree rotation)
    vector2 perpendicular() const {
        return vector2(-y, x);
    }

    vector2 perp() const {
        return perpendicular();
    }

    // Rotate by angle (in radians)
    vector2 rotated(float angle) const {
        float cos_a = std::cos(angle);
        float sin_a = std::sin(angle);
        return vector2(
            x * cos_a - y * sin_a,
            x * sin_a + y * cos_a
        );
    }

    void rotate(float angle) {
        *this = rotated(angle);
    }

    // Get angle in radians
    float angle() const {
        return std::atan2(y, x);
    }

    // Get angle between two vectors
    float angleTo(const vector2& other) const {
        float dot_product = dot(other);
        float cross_product = cross(other);
        return std::atan2(cross_product, dot_product);
    }

    // Get signed angle between vectors (-PI to PI)
    float signedAngleTo(const vector2& other) const {
        return angleTo(other);
    }

    // Get unsigned angle between vectors (0 to PI)
    float unsignedAngleTo(const vector2& other) const {
        return std::abs(angleTo(other));
    }

    // Utility functions
    bool isNull() const {
        return x == 0.0f && y == 0.0f;
    }

    bool isZero() const {
        return isNull();
    }

    bool isNear(const vector2& other, float epsilon = 1e-6f) const {
        return distance(other) < epsilon;
    }

    bool isNormalized(float epsilon = 1e-6f) const {
        return std::abs(magnitudeSquared() - 1.0f) < epsilon;
    }

    bool isUnit(float epsilon = 1e-6f) const {
        return isNormalized(epsilon);
    }

    // Component access
    float& operator[](int index) {
        return (&x)[index];
    }

    const float& operator[](int index) const {
        return (&x)[index];
    }

    // Min/Max components - using explicit std:: to avoid macro conflicts
    vector2 minVec(const vector2& other) const {
        return vector2(
            (std::min)(x, other.x),
            (std::min)(y, other.y)
        );
    }

    vector2 maxVec(const vector2& other) const {
        return vector2(
            (std::max)(x, other.x),
            (std::max)(y, other.y)
        );
    }

    // Clamp components
    vector2 clamp(const vector2& min_vec, const vector2& max_vec) const {
        return vector2(
            (std::max)(min_vec.x, (std::min)(max_vec.x, x)),
            (std::max)(min_vec.y, (std::min)(max_vec.y, y))
        );
    }

    vector2 clamp(float min_val, float max_val) const {
        return vector2(
            (std::max)(min_val, (std::min)(max_val, x)),
            (std::max)(min_val, (std::min)(max_val, y))
        );
    }

    // Absolute value
    vector2 abs() const {
        return vector2(std::abs(x), std::abs(y));
    }

    // Floor, ceil, round
    vector2 floor() const {
        return vector2(std::floor(x), std::floor(y));
    }

    vector2 ceil() const {
        return vector2(std::ceil(x), std::ceil(y));
    }

    vector2 round() const {
        return vector2(std::round(x), std::round(y));
    }

    // Fractional part
    vector2 fract() const {
        return *this - floor();
    }

    // Sign function
    vector2 sign() const {
        return vector2(
            x > 0.0f ? 1.0f : (x < 0.0f ? -1.0f : 0.0f),
            y > 0.0f ? 1.0f : (y < 0.0f ? -1.0f : 0.0f)
        );
    }

    // Static utility functions
    static vector2 zero() { return vector2(0.0f, 0.0f); }
    static vector2 one() { return vector2(1.0f, 1.0f); }
    static vector2 up() { return vector2(0.0f, 1.0f); }
    static vector2 down() { return vector2(0.0f, -1.0f); }
    static vector2 left() { return vector2(-1.0f, 0.0f); }
    static vector2 right() { return vector2(1.0f, 0.0f); }

    // Create vector from angle and magnitude
    static vector2 fromAngle(float angle, float magnitude = 1.0f) {
        return vector2(
            std::cos(angle) * magnitude,
            std::sin(angle) * magnitude
        );
    }

    // Create random unit vector
    static vector2 randomUnit() {
        float angle = static_cast<float>(rand()) / RAND_MAX * 2.0f * static_cast<float>(M_PI);
        return fromAngle(angle);
    }

    // Create random vector in range
    static vector2 random(float min_x, float max_x, float min_y, float max_y) {
        return vector2(
            min_x + (static_cast<float>(rand()) / RAND_MAX) * (max_x - min_x),
            min_y + (static_cast<float>(rand()) / RAND_MAX) * (max_y - min_y)
        );
    }

    static vector2 random(const vector2& min_vec, const vector2& max_vec) {
        return random(min_vec.x, max_vec.x, min_vec.y, max_vec.y);
    }
};

// Non-member operators for scalar multiplication
inline vector2 operator*(float scalar, const vector2& vec) {
    return vec * scalar;
}

struct vector3
{
	float x;
	float y;
	float z;

	// Constructors
	vector3() : x(0.0f), y(0.0f), z(0.0f) {}
	vector3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	// Basic arithmetic operators
	vector3 operator+(const vector3& other) const {
		return vector3(x + other.x, y + other.y, z + other.z);
	}

	vector3 operator-(const vector3& other) const {
		return vector3(x - other.x, y - other.y, z - other.z);
	}

	vector3 operator*(float scalar) const {
		return vector3(x * scalar, y * scalar, z * scalar);
	}
	vector3& operator*=(const vector3& other) {
		x *= other.x;
		y *= other.y;
		z *= other.z;
		return *this;
	}

	vector3 operator/(float scalar) const {
		if (scalar != 0.0f) {
			return vector3(x / scalar, y / scalar, z / scalar);
		}
		return vector3(0.0f, 0.0f, 0.0f);
	}

	// Assignment operators
	vector3& operator+=(const vector3& other) {
		x += other.x;
		y += other.y;
		z += other.z;
		return *this;
	}

	vector3& operator-=(const vector3& other) {
		x -= other.x;
		y -= other.y;
		z -= other.z;
		return *this;
	}

	vector3& operator*=(float scalar) {
		x *= scalar;
		y *= scalar;
		z *= scalar;
		return *this;
	}

	vector3& operator/=(float scalar) {
		if (scalar != 0.0f) {
			x /= scalar;
			y /= scalar;
			z /= scalar;
		}
		return *this;
	}

	// Comparison operators
	bool operator==(const vector3& other) const {
		const float epsilon = 1e-6f;
		return (std::abs(x - other.x) < epsilon &&
			std::abs(y - other.y) < epsilon &&
			std::abs(z - other.z) < epsilon);
	}

	bool operator!=(const vector3& other) const {
		return !(*this == other);
	}

	// Vector operations
	float dot(const vector3& other) const {
		return x * other.x + y * other.y + z * other.z;
	}

	vector3 cross(const vector3& other) const {
		return vector3(
			y * other.z - z * other.y,
			z * other.x - x * other.z,
			x * other.y - y * other.x
		);
	}

	float magnitude() const {
		return std::sqrt(x * x + y * y + z * z);
	}

	float magnitudeSquared() const {
		return x * x + y * y + z * z;
	}

	vector3 normalized() const {
		float mag = magnitude();
		if (mag > 0.0f) {
			return *this / mag;
		}
		return vector3(0.0f, 0.0f, 0.0f);
	}

	void normalize() {
		float mag = magnitude();
		if (mag > 0.0f) {
			*this /= mag;
		}
	}

	float distance(const vector3& other) const {
		return (*this - other).magnitude();
	}

	float distanceSquared(const vector3& other) const {
		return (*this - other).magnitudeSquared();
	}

	vector3 lerp(const vector3& other, float t) const {
		return *this + (other - *this) * t;
	}

	vector3 reflect(const vector3& normal) const {
		return *this - normal * (2.0f * this->dot(normal));
	}

	bool isNull() const {
		return x == 0.0f && y == 0.0f && z == 0.0f;
	}

	bool isNear(const vector3& other, float epsilon = 1e-6f) const {
		return distance(other) < epsilon;
	}

	// Static utility functions
	static vector3 zero() { return vector3(0.0f, 0.0f, 0.0f); }
	static vector3 one() { return vector3(1.0f, 1.0f, 1.0f); }
	static vector3 up() { return vector3(0.0f, 1.0f, 0.0f); }
	static vector3 right() { return vector3(1.0f, 0.0f, 0.0f); }
	static vector3 forward() { return vector3(0.0f, 0.0f, 1.0f); }
};

// Non-member operators for scalar multiplication
inline vector3 operator*(float scalar, const vector3& vec) {
	return vec * scalar;
}

struct matrix
{
	vector3 rvec;  // Right vector
	vector3 uvec;  // Up vector
	vector3 fvec;  // Forward vector

	// Constructors
	matrix() : rvec(1.0f, 0.0f, 0.0f), uvec(0.0f, 1.0f, 0.0f), fvec(0.0f, 0.0f, 1.0f) {}
	matrix(const vector3& r, const vector3& u, const vector3& f) : rvec(r), uvec(u), fvec(f) {}

	// Matrix multiplication
	void multiply(const matrix* lhs, const matrix* rhs);

	matrix operator*(const matrix& other) const {
		matrix result;
		result.multiply(this, &other);
		return result;
	}

	matrix& operator*=(const matrix& other) {
		matrix temp = *this * other;
		*this = temp;
		return *this;
	}

	// Vector transformation
	vector3 transform(const vector3& vec) const {
		return vector3(
			rvec.x * vec.x + uvec.x * vec.y + fvec.x * vec.z,
			rvec.y * vec.x + uvec.y * vec.y + fvec.y * vec.z,
			rvec.z * vec.x + uvec.z * vec.y + fvec.z * vec.z
		);
	}

	vector3 operator*(const vector3& vec) const {
		return transform(vec);
	}

	// Matrix operations
	matrix transpose() const {
		return matrix(
			vector3(rvec.x, uvec.x, fvec.x),
			vector3(rvec.y, uvec.y, fvec.y),
			vector3(rvec.z, uvec.z, fvec.z)
		);
	}

	float determinant() const {
		return rvec.x * (uvec.y * fvec.z - uvec.z * fvec.y) -
			rvec.y * (uvec.x * fvec.z - uvec.z * fvec.x) +
			rvec.z * (uvec.x * fvec.y - uvec.y * fvec.x);
	}

	matrix inverse() const {
		float det = determinant();
		if (std::abs(det) < 1e-6f) {
			return matrix(); // Return identity if not invertible
		}

		float invDet = 1.0f / det;
		matrix result;

		result.rvec.x = (uvec.y * fvec.z - uvec.z * fvec.y) * invDet;
		result.rvec.y = (rvec.z * fvec.y - rvec.y * fvec.z) * invDet;
		result.rvec.z = (rvec.y * uvec.z - rvec.z * uvec.y) * invDet;

		result.uvec.x = (uvec.z * fvec.x - uvec.x * fvec.z) * invDet;
		result.uvec.y = (rvec.x * fvec.z - rvec.z * fvec.x) * invDet;
		result.uvec.z = (rvec.z * uvec.x - rvec.x * uvec.z) * invDet;

		result.fvec.x = (uvec.x * fvec.y - uvec.y * fvec.x) * invDet;
		result.fvec.y = (rvec.y * fvec.x - rvec.x * fvec.y) * invDet;
		result.fvec.z = (rvec.x * uvec.y - rvec.y * uvec.x) * invDet;

		return result;
	}

	// Create transformation matrices
	static matrix identity() {
		return matrix();
	}

	static matrix scale(float sx, float sy, float sz) {
		return matrix(
			vector3(sx, 0.0f, 0.0f),
			vector3(0.0f, sy, 0.0f),
			vector3(0.0f, 0.0f, sz)
		);
	}

	static matrix scale(const vector3& s) {
		return scale(s.x, s.y, s.z);
	}

	static matrix rotationX(float angle) {
		float c = std::cos(angle);
		float s = std::sin(angle);
		return matrix(
			vector3(1.0f, 0.0f, 0.0f),
			vector3(0.0f, c, -s),
			vector3(0.0f, s, c)
		);
	}

	static matrix rotationY(float angle) {
		float c = std::cos(angle);
		float s = std::sin(angle);
		return matrix(
			vector3(c, 0.0f, s),
			vector3(0.0f, 1.0f, 0.0f),
			vector3(-s, 0.0f, c)
		);
	}

	static matrix rotationZ(float angle) {
		float c = std::cos(angle);
		float s = std::sin(angle);
		return matrix(
			vector3(c, -s, 0.0f),
			vector3(s, c, 0.0f),
			vector3(0.0f, 0.0f, 1.0f)
		);
	}

	// Utility functions
	bool isIdentity(float epsilon = 1e-6f) const {
		matrix id = identity();
		return rvec.isNear(id.rvec, epsilon) &&
			uvec.isNear(id.uvec, epsilon) &&
			fvec.isNear(id.fvec, epsilon);
	}

	bool isOrthogonal(float epsilon = 1e-6f) const {
		return std::abs(rvec.dot(uvec)) < epsilon &&
			std::abs(rvec.dot(fvec)) < epsilon &&
			std::abs(uvec.dot(fvec)) < epsilon;
	}

	void orthonormalize() {
		rvec.normalize();
		uvec = uvec - rvec * uvec.dot(rvec);
		uvec.normalize();
		fvec = rvec.cross(uvec);
	}

	// Get Euler angles (in radians)
	vector3 getEulerAngles() const {
		vector3 angles;

		// Extract pitch (X rotation)
		angles.x = std::asin(-fvec.y);

		// Extract yaw (Y rotation) and roll (Z rotation)
		if (std::abs(fvec.y) < 0.99999f) {
			angles.y = std::atan2(fvec.x, fvec.z);
			angles.z = std::atan2(rvec.y, uvec.y);
		}
		else {
			angles.y = std::atan2(-rvec.z, rvec.x);
			angles.z = 0.0f;
		}

		return angles;
	}

	// Create matrix from Euler angles (in radians)
	static matrix fromEulerAngles(float pitch, float yaw, float roll) {
		return rotationY(yaw) * rotationX(pitch) * rotationZ(roll);
	}

	static matrix fromEulerAngles(const vector3& angles) {
		return fromEulerAngles(angles.x, angles.y, angles.z);
	}
};

namespace Math
{
	extern void Init();
	namespace Fixes {
		extern SafetyHookMid matrix_operator_multiplication_midhook;
		extern signed char SSE_hack;
        extern signed char FixWater;
        extern bool SimulateWaterBug;
	}
	inline bool isVectorNull(vector3& vec) {
		return vec.isNull();
	}
}