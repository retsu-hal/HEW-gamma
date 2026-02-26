#pragma once

#include <DirectXMath.h>
#include "imgui.h"
#include "Player2DCapsule.h"
#include "ShadowColliderBox.h"


struct ScreenPt//スクリーン座標の構造体
{
    ImVec2 pos;
    bool valid;
};

struct ShadowDebugOptions//シャドウの当たり判定のデバッグ描画オプション
{
	bool drawPrism = true;//シャドウプリズムを描画するか
	bool drawAABB = false;//AABBを描画するか
	bool drawNormal = true;//法線を描画するか
	bool drawVertices = false;//プリズムの頂点を描画するか
	bool drawStandSegments = true;//立ちセグメントを描画するか
	bool drawGround = true;//地面の当たり判定を描画するか

    unsigned int prismColor = 0xFF0000FF;
    unsigned int aabbColor = 0x80FFFF00;
    unsigned int normalColor = 0xFFFFFF00;
    unsigned int vertexColor = 0xFF00FF00;
    unsigned int standSegColor = 0xFFFF00FF;
    unsigned int groundColor = 0xFFFF00FF;
};

ScreenPt WorldToScreen(const DirectX::XMFLOAT3& worldPos);
void DrawLine3D(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b, ImU32 col, float thick = 1.0f);
void DrawPoint3D(const DirectX::XMFLOAT3& p, ImU32 col, float size = 4.0f);
void DebugDrawAABB(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& half, ImU32 col);
void DebugDrawOBB_FullRotation(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& half,
    const DirectX::XMFLOAT3& rotDeg, ImU32 col);
void DebugDrawOBB_Yaw(const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT3& h, float yawDeg, ImU32 col);
void DebugDrawEllipsoid(const DirectX::XMFLOAT3& c, const DirectX::XMFLOAT3& r, ImU32 col, int seg = 24);
void DebugDrawCapsule2D(const Capsule2D& capsule, ImU32 col, int segments = 20);
void DebugDrawShadowPrism(const ShadowPrism& prism, const ShadowDebugOptions& opts);