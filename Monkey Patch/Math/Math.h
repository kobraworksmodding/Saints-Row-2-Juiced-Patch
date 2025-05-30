#pragma once
#include <chrono>
#include <safetyhook.hpp>
#include <cmath>

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
	extern struct Velocity {
		float vx;
		float vy;
		float vz;
		float magnitude;
	};
	namespace Fixes {
		extern SafetyHookMid matrix_operator_multiplication_midhook;
	}
	inline bool isVectorNull(vector3& vec) {
		return vec.isNull();
	}
}