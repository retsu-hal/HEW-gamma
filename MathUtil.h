#pragma once
#include <DirectXMath.h>
#include <cmath>
#include <algorithm>

namespace mu
{
    using DirectX::XMFLOAT2;
    using DirectX::XMFLOAT3;


    constexpr float kEpsLen = 1e-6f;


    inline float Clamp(float v, float a, float b)
    {
        return (v < a) ? a : (v > b) ? b : v;
    }


    inline XMFLOAT3 operator+(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return { a.x + b.x, a.y + b.y, a.z + b.z };
    }

    inline XMFLOAT3 operator-(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return { a.x - b.x, a.y - b.y, a.z - b.z };
    }

    inline XMFLOAT3 operator*(const XMFLOAT3& a, float s)
    {
        return { a.x * s, a.y * s, a.z * s };
    }

    inline XMFLOAT3 operator*(float s, const XMFLOAT3& a)
    {
        return a * s;
    }

    inline XMFLOAT3& operator+=(XMFLOAT3& a, const XMFLOAT3& b)
    {
        a = a + b; return a;
    }

    inline XMFLOAT3& operator-=(XMFLOAT3& a, const XMFLOAT3& b)
    {
        a = a - b; return a;
    }


    inline XMFLOAT3 Mul(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return { a.x * b.x, a.y * b.y, a.z * b.z };
    }

    inline float Dot(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    inline XMFLOAT3 Cross(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return {
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        };
    }

    inline float LengthSq(const XMFLOAT3& v) { return Dot(v, v); }

    inline float Length(const XMFLOAT3& v) { return std::sqrt(LengthSq(v)); }

    inline XMFLOAT3 Normalize(const XMFLOAT3& v, const XMFLOAT3& fallback = { 0,1,0 })
    {
        float l = LengthSq(v);
        if (l < kEpsLen * kEpsLen) return fallback;
        float inv = 1.0f / std::sqrt(l);
        return v * inv;
    }

    inline float Dot2D(const XMFLOAT3& a, const XMFLOAT3& b)
    {
        return a.x * b.x + a.z * b.z;
    }

    inline float Length2D(const XMFLOAT3& v)
    {
        return std::sqrt(v.x * v.x + v.z * v.z);
    }

    inline XMFLOAT3 Normalize2D(const XMFLOAT3& v, const XMFLOAT3& fallback = { 0,0,1 })
    {
        float l = Length2D(v);
        if (l < kEpsLen) return fallback;
        return { v.x / l, 0.0f, v.z / l };
    }

    inline XMFLOAT3 RotateY(const XMFLOAT3& v, float yawRad)
    {
        const float c = std::cos(yawRad);
        const float s = std::sin(yawRad);
        return { v.x * c + v.z * s, v.y, -v.x * s + v.z * c };
    }

}
