//Player2DCapsule.h
#pragma once

#include <d3d11.h>
#include <DirectXMath.h>
#include <cmath>


#define PLAYER2D_CAPSULE_RADIUS     (0.35f)
#define PLAYER2D_CAPSULE_HEIGHT     (1.3f)
#define PLAYER2D_CAPSULE_HALF_Z     (0.07f)

struct Capsule2D
{
    DirectX::XMFLOAT3 center;
    float radius;     
    float halfHeight; 
    float halfZ;      
    float rotationZ;  

    Capsule2D()
        : center{ 0, 0, 0 }
        , radius(PLAYER2D_CAPSULE_RADIUS)
        , halfHeight(PLAYER2D_CAPSULE_HEIGHT * 0.5f)
        , halfZ(PLAYER2D_CAPSULE_HALF_Z)
        , rotationZ(0.0f)
    {
    }

    DirectX::XMFLOAT3 GetTop() const
    {
        float cosZ = cosf(rotationZ);
        float sinZ = sinf(rotationZ);
        return DirectX::XMFLOAT3(
            center.x - halfHeight * sinZ,
            center.y + halfHeight * cosZ,
            center.z
        );
    }

    DirectX::XMFLOAT3 GetBottom() const
    {
        float cosZ = cosf(rotationZ);
        float sinZ = sinf(rotationZ);
        return DirectX::XMFLOAT3(
            center.x + halfHeight * sinZ,
            center.y - halfHeight * cosZ,
            center.z
        );
    }

    float GetTotalHeight() const
    {
        return halfHeight * 2.0f + radius * 2.0f;
    }

    DirectX::XMFLOAT3 GetBoundingHalfSize() const
    {
        float cosZ = fabsf(cosf(rotationZ));
        float sinZ = fabsf(sinf(rotationZ));
        return DirectX::XMFLOAT3(
            radius + halfHeight * sinZ,
            radius + halfHeight * cosZ,
            halfZ
        );
    }
};