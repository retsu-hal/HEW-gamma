#include "UtilDebug.h"
#include "UtilMath.h"
#include "camera.h"
#include "direct3d.h"



using namespace DirectX;
using namespace mu;

// ワールド座標をスクリーン座標に変換する関数
ScreenPt WorldToScreen(const XMFLOAT3& p)
{
    ScreenPt out{};
    out.valid = false;

    XMMATRIX vp = GetViewMatrix() * GetProjectionMatrix();
    XMVECTOR vW = XMVectorSet(p.x, p.y, p.z, 1.0f);
    XMVECTOR vV = XMVector3TransformCoord(vW, GetViewMatrix());

    if (XMVectorGetZ(vV) <= 0.01f) return out;

    XMVECTOR vC = XMVector3TransformCoord(vW, vp);
    XMFLOAT3 ndc;
    XMStoreFloat3(&ndc, vC);

    if (ndc.x < -1.5f || ndc.x > 1.5f || ndc.y < -1.5f || ndc.y > 1.5f)
        return out;

    ImGuiViewport* vp_ = ImGui::GetMainViewport();
    float w = (float)Direct3D_GetBackBufferWidth();
    float h = (float)Direct3D_GetBackBufferHeight();

    out.pos.x = vp_->Pos.x + (ndc.x * 0.5f + 0.5f) * w;
    out.pos.y = vp_->Pos.y + (-ndc.y * 0.5f + 0.5f) * h;
    out.valid = true;
    return out;
}

// 3D空間の線を描画する関数
void DrawLine3D(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col, float thick)
{
    ScreenPt sa = WorldToScreen(a);
    ScreenPt sb = WorldToScreen(b);
    if (sa.valid && sb.valid)
        ImGui::GetBackgroundDrawList()->AddLine(sa.pos, sb.pos, col, thick);
}

// 3D空間の点を描画する関数
void DrawPoint3D(const XMFLOAT3& p, ImU32 col, float size)
{
    ScreenPt sp = WorldToScreen(p);
    if (sp.valid)
        ImGui::GetBackgroundDrawList()->AddCircleFilled(sp.pos, size, col);
}

// 軸に沿った回転を持つOBBを描画する関数
void DebugDrawAABB(const XMFLOAT3& c, const XMFLOAT3& h, ImU32 col)
{
    XMFLOAT3 corners[8] = {
        {c.x - h.x, c.y - h.y, c.z - h.z}, {c.x + h.x, c.y - h.y, c.z - h.z},
        {c.x + h.x, c.y + h.y, c.z - h.z}, {c.x - h.x, c.y + h.y, c.z - h.z},
        {c.x - h.x, c.y - h.y, c.z + h.z}, {c.x + h.x, c.y - h.y, c.z + h.z},
        {c.x + h.x, c.y + h.y, c.z + h.z}, {c.x - h.x, c.y + h.y, c.z + h.z},
    };
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
    };
    for (int i = 0; i < 12; i++)
        DrawLine3D(corners[edges[i][0]], corners[edges[i][1]], col);
}

// 任意の回転を持つOBBを描画する関数
void DebugDrawOBB_FullRotation(const XMFLOAT3& center, const XMFLOAT3& half,
    const XMFLOAT3& rotDeg, ImU32 col)
{
    XMMATRIX rotMat = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(rotDeg.x),
        XMConvertToRadians(rotDeg.y),
        XMConvertToRadians(rotDeg.z));

    XMFLOAT3 local[8] = {
        {-half.x,-half.y,-half.z}, {+half.x,-half.y,-half.z},
        {+half.x,+half.y,-half.z}, {-half.x,+half.y,-half.z},
        {-half.x,-half.y,+half.z}, {+half.x,-half.y,+half.z},
        {+half.x,+half.y,+half.z}, {-half.x,+half.y,+half.z},
    };

    XMFLOAT3 corners[8];
    for (int i = 0; i < 8; i++)
    {
        XMVECTOR vLocal = XMLoadFloat3(&local[i]);
        XMVECTOR vWorld = XMVector3TransformNormal(vLocal, rotMat);
        XMFLOAT3 rotated;
        XMStoreFloat3(&rotated, vWorld);
        corners[i] = { center.x + rotated.x, center.y + rotated.y, center.z + rotated.z };
    }

    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
    };
    for (int i = 0; i < 12; i++)
        DrawLine3D(corners[edges[i][0]], corners[edges[i][1]], col);
}

// Yaw回転のみを持つOBBを描画する関数（水平面での回転のみ）
void DebugDrawOBB_Yaw(const XMFLOAT3& c, const XMFLOAT3& h, float yawDeg, ImU32 col)
{
    float yaw = XMConvertToRadians(yawDeg);

    XMFLOAT3 local[8] = {
        {-h.x,-h.y,-h.z}, {+h.x,-h.y,-h.z}, {+h.x,+h.y,-h.z}, {-h.x,+h.y,-h.z},
        {-h.x,-h.y,+h.z}, {+h.x,-h.y,+h.z}, {+h.x,+h.y,+h.z}, {-h.x,+h.y,+h.z},
    };

    XMFLOAT3 corners[8];
    for (int i = 0; i < 8; i++)
        corners[i] = c + RotateY(local[i], yaw);

    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, {4,5},{5,6},{6,7},{7,4}, {0,4},{1,5},{2,6},{3,7}
    };

    for (int i = 0; i < 12; i++)
        DrawLine3D(corners[edges[i][0]], corners[edges[i][1]], col);
}

// 楕円体を描画する関数
void DebugDrawEllipsoid(const XMFLOAT3& c, const XMFLOAT3& r, ImU32 col, int seg)
{
    const float PI2 = 6.28318530718f;

    auto drawRing = [&](int plane) {
        XMFLOAT3 prev;
        for (int i = 0; i <= seg; i++)
        {
            float t = PI2 * i / seg;
            XMFLOAT3 p = c;
            if (plane == 0) { p.x += cosf(t) * r.x; p.z += sinf(t) * r.z; }
            if (plane == 1) { p.x += cosf(t) * r.x; p.y += sinf(t) * r.y; }
            if (plane == 2) { p.y += cosf(t) * r.y; p.z += sinf(t) * r.z; }
            if (i > 0) DrawLine3D(prev, p, col);
            prev = p;
        }
        };

    drawRing(0); drawRing(1); drawRing(2);
}

// カプセルを描画する関数
void DebugDrawCapsule2D(const Capsule2D& capsule, ImU32 col, int segments)
{
    XMFLOAT3 top = capsule.GetTop();
    XMFLOAT3 bottom = capsule.GetBottom();

    XMFLOAT3 axis = top - bottom;
    float axisLen = Length(axis);
    if (axisLen < 1e-6f)
    {
        DrawPoint3D(capsule.center, col, capsule.radius * 10.0f);
        return;
    }
    XMFLOAT3 axisDir = axis * (1.0f / axisLen);

    float cosZ = cosf(capsule.rotationZ);
    float sinZ = sinf(capsule.rotationZ);

    XMFLOAT3 right = { cosZ, sinZ, 0.0f };
    XMFLOAT3 forward = { 0.0f, 0.0f, 1.0f };

    const float PI = 3.14159265358979f;

    XMFLOAT3 topL = top - right * capsule.radius;
    XMFLOAT3 topR = top + right * capsule.radius;
    XMFLOAT3 botL = bottom - right * capsule.radius;
    XMFLOAT3 botR = bottom + right * capsule.radius;
    DrawLine3D(topL, botL, col, 2.0f);
    DrawLine3D(topR, botR, col, 2.0f);

    {
        XMFLOAT3 prev = topR;
        for (int i = 1; i <= segments; ++i)
        {
            float angle = PI * (float)i / (float)segments;
            float c = cosf(angle);
            float s = sinf(angle);
            XMFLOAT3 p = top + right * (c * capsule.radius) + axisDir * (s * capsule.radius);
            DrawLine3D(prev, p, col, 2.0f);
            prev = p;
        }
    }

    {
        XMFLOAT3 prev = botL;
        for (int i = 1; i <= segments; ++i)
        {
            float angle = PI * (float)i / (float)segments;
            float c = cosf(angle);
            float s = sinf(angle);
            XMFLOAT3 p = bottom - right * (c * capsule.radius) - axisDir * (s * capsule.radius);
            DrawLine3D(prev, p, col, 2.0f);
            prev = p;
        }
    }

    auto drawZRing = [&](const XMFLOAT3& center, float r, ImU32 c2) {
        const float PI2 = PI * 2.0f;
        XMFLOAT3 prev;
        for (int i = 0; i <= segments; ++i)
        {
            float a = PI2 * (float)i / (float)segments;
            XMFLOAT3 p = center + right * (cosf(a) * r) + forward * (sinf(a) * r);
            if (i > 0) DrawLine3D(prev, p, c2, 1.0f);
            prev = p;
        }
        };

    ImU32 colFaded = (col & 0x00FFFFFF) | 0x80000000;
    drawZRing(top, capsule.radius, colFaded);
    drawZRing(bottom, capsule.radius, colFaded);
    drawZRing(capsule.center, capsule.radius, colFaded);

    XMFLOAT3 topF = top + forward * capsule.radius;
    XMFLOAT3 topB = top - forward * capsule.radius;
    XMFLOAT3 botF = bottom + forward * capsule.radius;
    XMFLOAT3 botB = bottom - forward * capsule.radius;
    DrawLine3D(topF, botF, colFaded, 1.0f);
    DrawLine3D(topB, botB, colFaded, 1.0f);

    DrawLine3D(top, bottom, col, 1.0f);
    DrawPoint3D(capsule.center, col, 3.0f);
    DrawPoint3D(top, col, 2.0f);
    DrawPoint3D(bottom, col, 2.0f);
}


// シャドウプリズムを描画する関数
void DebugDrawShadowPrism(const ShadowPrism& prism, const ShadowDebugOptions& opts)
{
    if (!prism.isValid || prism.poly.size() < 3) return;

    int n = (int)prism.baseWorld.size();
    ImU32 colPrism = opts.prismColor;
    ImU32 colFaded = (colPrism & 0x00FFFFFF) | 0x60000000;

    if (opts.drawPrism)
    {
        for (int i = 0; i < n; i++)
            DrawLine3D(prism.baseWorld[i], prism.baseWorld[(i + 1) % n], colPrism, 2.0f);

        if (!prism.topWorld.empty())
        {
            for (int i = 0; i < n; i++)
                DrawLine3D(prism.topWorld[i], prism.topWorld[(i + 1) % n], colPrism, 2.0f);

            for (int i = 0; i < n; i++)
                DrawLine3D(prism.baseWorld[i], prism.topWorld[i], colFaded, 1.0f);
        }
    }

    if (opts.drawNormal)
    {
        XMFLOAT3 nEnd = prism.origin + prism.n * 0.5f;
        DrawLine3D(prism.origin, nEnd, opts.normalColor, 2.0f);
        DrawPoint3D(prism.origin, opts.normalColor, 5.0f);
    }

    if (opts.drawVertices)
    {
        for (const auto& p : prism.baseWorld)
            DrawPoint3D(p, opts.vertexColor, 4.0f);
    }


    if (opts.drawStandSegments && !prism.standSegments.empty())
    {
        for (const auto& seg : prism.standSegments)
        {
            DrawLine3D(seg.a, seg.b, opts.standSegColor, 4.0f);
            DrawPoint3D(seg.a, opts.standSegColor, 4.0f);
            DrawPoint3D(seg.b, opts.standSegColor, 4.0f);
        }
    }

    if (opts.drawAABB)
    {
        XMFLOAT3 c = (prism.aabbMin + prism.aabbMax) * 0.5f;
        XMFLOAT3 h = (prism.aabbMax - prism.aabbMin) * 0.5f;
        DebugDrawAABB(c, h, opts.aabbColor);
    }

    if (opts.drawGround && prism.groundPoly.size() >= 3)
    {
        ImU32 colG = opts.groundColor;
        int gn = (int)prism.groundPoly.size();
        for (int i = 0; i < gn; ++i)
        {
            XMFLOAT2 a2 = prism.groundPoly[i];
            XMFLOAT2 b2 = prism.groundPoly[(i + 1) % gn];
            XMFLOAT3 aW = prism.origin + prism.u * a2.x + prism.v * a2.y;
            XMFLOAT3 bW = prism.origin + prism.u * b2.x + prism.v * b2.y;
            DrawLine3D(aW, bW, colG, 3.0f);
        }

        XMFLOAT2 c2{ 0,0 };
        for (auto& p : prism.groundPoly) { c2.x += p.x; c2.y += p.y; }
        c2.x /= (float)gn; c2.y /= (float)gn;
        XMFLOAT3 cW = prism.origin + prism.u * c2.x + prism.v * c2.y;
        XMFLOAT3 upEnd = cW + XMFLOAT3(0, 0.35f, 0);
        DrawLine3D(cW, upEnd, colG, 2.0f);
        DrawPoint3D(cW, colG, 6.0f);
    }
}