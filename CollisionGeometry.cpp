// CollisionGeometry.cpp
// 純幾何計算：OBB交差/分離、楕円体-OBB、カプセル-OBB、線分/点の最近距離

#include <cfloat>
#include <cmath>
#include "Collision.h"

using namespace DirectX;
using namespace mu;

//=========================================================================================================
// OBB vs OBB（Y軸回転のみ）
//=========================================================================================================

bool OBB_Intersect_Yaw(
    const XMFLOAT3& cA, const XMFLOAT3& hA, float yawA,
    const XMFLOAT3& cB, const XMFLOAT3& hB, float yawB)
{
    if (fabsf(cA.y - cB.y) > (hA.y + hB.y)) return false;

    auto getAxes = [](float yaw, XMFLOAT3& ax, XMFLOAT3& az) {
        float y = XMConvertToRadians(yaw);
        XMFLOAT3 fwd = { sinf(y), 0, cosf(y) };
        ax = { fwd.z, 0, -fwd.x };
        az = fwd;
    };

    XMFLOAT3 Ax, Az, Bx, Bz;
    getAxes(yawA, Ax, Az);
    getAxes(yawB, Bx, Bz);

    XMFLOAT3 T = { cB.x - cA.x, 0, cB.z - cA.z };

    float t0 = Dot2D(T, Ax), t1 = Dot2D(T, Az);
    float R00 = Dot2D(Ax, Bx), R01 = Dot2D(Ax, Bz);
    float R10 = Dot2D(Az, Bx), R11 = Dot2D(Az, Bz);

    const float eps = 1e-6f;
    float aR00 = fabsf(R00) + eps, aR01 = fabsf(R01) + eps;
    float aR10 = fabsf(R10) + eps, aR11 = fabsf(R11) + eps;

    if (fabsf(t0) > hA.x + hB.x * aR00 + hB.z * aR01) return false;
    if (fabsf(t1) > hA.z + hB.x * aR10 + hB.z * aR11) return false;
    if (fabsf(t0 * R00 + t1 * R10) > hB.x + hA.x * aR00 + hA.z * aR10) return false;
    if (fabsf(t0 * R01 + t1 * R11) > hB.z + hA.x * aR01 + hA.z * aR11) return false;

    return true;
}

//=========================================================================================================
// 楕円体 vs OBB（Y軸回転のみ）
//=========================================================================================================

bool Resolve_Ellipsoid_OBB_Yaw(
    const XMFLOAT3& ellC, const XMFLOAT3& ellR,
    const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYaw,
    XMFLOAT3* outPush, XMFLOAT3* outNormal)
{
    if (outPush)   *outPush   = { 0, 0, 0 };
    if (outNormal) *outNormal = { 0, 1, 0 };

    XMFLOAT3 invR = { 1.0f / ellR.x, 1.0f / ellR.y, 1.0f / ellR.z };
    XMFLOAT3 cS = Mul(ellC, invR);
    XMFLOAT3 bS = Mul(boxC, invR);
    XMFLOAT3 hS = Mul(boxH, invR);

    float yawRad = XMConvertToRadians(boxYaw);

    XMFLOAT3 d  = cS - bS;
    XMFLOAT3 dl = RotateY(d, -yawRad);
    dl.x = Clamp(dl.x, -hS.x, hS.x);
    dl.y = Clamp(dl.y, -hS.y, hS.y);
    dl.z = Clamp(dl.z, -hS.z, hS.z);
    XMFLOAT3 qS = bS + RotateY(dl, yawRad);

    XMFLOAT3 dS   = cS - qS;
    float    dist  = Length(dS);

    if (dist >= 1.0f) return false;

    XMFLOAT3 nS;
    float    pen;

    if (dist > 1e-6f)
    {
        nS  = dS * (1.0f / dist);
        pen = 1.0f - dist;
    }
    else
    {
        XMFLOAT3 lp = RotateY(cS - bS, -yawRad);
        float dx = hS.x - fabsf(lp.x);
        float dy = hS.y - fabsf(lp.y);
        float dz = hS.z - fabsf(lp.z);

        XMFLOAT3 ln = { 0, 1, 0 };
        if (dx <= dy && dx <= dz)      { ln = { (lp.x >= 0) ? 1.0f : -1.0f, 0, 0 }; pen = 1.0f + dx; }
        else if (dy <= dz)             { ln = { 0, (lp.y >= 0) ? 1.0f : -1.0f, 0 }; pen = 1.0f + dy; }
        else                           { ln = { 0, 0, (lp.z >= 0) ? 1.0f : -1.0f }; pen = 1.0f + dz; }

        nS = RotateY(ln, yawRad);
    }

    XMFLOAT3 pushS = nS * pen;
    XMFLOAT3 pushW = { pushS.x * ellR.x, pushS.y * ellR.y, pushS.z * ellR.z };
    XMFLOAT3 nW    = Normalize(XMFLOAT3{ nS.x * ellR.x, nS.y * ellR.y, nS.z * ellR.z });

    if (outPush)   *outPush   = pushW;
    if (outNormal) *outNormal = nW;
    return true;
}

//=========================================================================================================
// OBB vs OBB（Z軸 + Y軸回転）? SAT全15軸
//=========================================================================================================

bool Resolve_OBB_OBB_ZY(
    const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
    const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,
    XMFLOAT3* outPush, XMFLOAT3* outNorm)
{
    XMMATRIX rotMatA = XMMatrixRotationZ(rotZRadA);
    XMMATRIX rotMatB = XMMatrixRotationY(XMConvertToRadians(rotYDegB));

    XMFLOAT3 axesA[3], axesB[3];

    XMVECTOR axA0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatA);
    XMVECTOR axA1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatA);
    XMVECTOR axA2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatA);
    XMStoreFloat3(&axesA[0], axA0);
    XMStoreFloat3(&axesA[1], axA1);
    XMStoreFloat3(&axesA[2], axA2);

    XMVECTOR axB0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatB);
    XMVECTOR axB1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatB);
    XMVECTOR axB2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatB);
    XMStoreFloat3(&axesB[0], axB0);
    XMStoreFloat3(&axesB[1], axB1);
    XMStoreFloat3(&axesB[2], axB2);

    XMVECTOR vD = XMVectorSet(posA.x - posB.x, posA.y - posB.y, posA.z - posB.z, 0);

    float halfExtA[3] = { halfA.x, halfA.y, halfA.z };
    float halfExtB[3] = { halfB.x, halfB.y, halfB.z };

    float    minPen  = FLT_MAX;
    XMFLOAT3 minAxis = { 0, 1, 0 };

    auto TestAxis = [&](XMVECTOR axis) -> bool {
        float len = XMVectorGetX(XMVector3Length(axis));
        if (len < 1e-6f) return true;
        axis = XMVector3Normalize(axis);

        float rA = 0, rB = 0;
        for (int i = 0; i < 3; i++)
        {
            rA += halfExtA[i] * fabsf(XMVectorGetX(XMVector3Dot(axis, XMLoadFloat3(&axesA[i]))));
            rB += halfExtB[i] * fabsf(XMVectorGetX(XMVector3Dot(axis, XMLoadFloat3(&axesB[i]))));
        }

        float dist = fabsf(XMVectorGetX(XMVector3Dot(vD, axis)));
        float pen  = (rA + rB) - dist;
        if (pen < 0) return false;
        if (pen < minPen) { minPen = pen; XMStoreFloat3(&minAxis, axis); }
        return true;
    };

    // 6 face axes
    if (!TestAxis(axA0)) return false;
    if (!TestAxis(axA1)) return false;
    if (!TestAxis(axA2)) return false;
    if (!TestAxis(axB0)) return false;
    if (!TestAxis(axB1)) return false;
    if (!TestAxis(axB2)) return false;

    // 9 edge-cross axes
    if (!TestAxis(XMVector3Cross(axA0, axB0))) return false;
    if (!TestAxis(XMVector3Cross(axA0, axB1))) return false;
    if (!TestAxis(XMVector3Cross(axA0, axB2))) return false;
    if (!TestAxis(XMVector3Cross(axA1, axB0))) return false;
    if (!TestAxis(XMVector3Cross(axA1, axB1))) return false;
    if (!TestAxis(XMVector3Cross(axA1, axB2))) return false;
    if (!TestAxis(XMVector3Cross(axA2, axB0))) return false;
    if (!TestAxis(XMVector3Cross(axA2, axB1))) return false;
    if (!TestAxis(XMVector3Cross(axA2, axB2))) return false;

    XMVECTOR normAxis = XMLoadFloat3(&minAxis);
    if (XMVectorGetX(XMVector3Dot(vD, normAxis)) < 0)
        normAxis = XMVectorNegate(normAxis);

    XMStoreFloat3(outPush, normAxis * minPen);
    XMStoreFloat3(outNorm, XMVector3Normalize(normAxis));
    return true;
}

bool OBB_Intersect_ZY(
    const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,
    const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB)
{
    XMFLOAT3 push, norm;
    return Resolve_OBB_OBB_ZY(posA, halfA, rotZRadA, posB, halfB, rotYDegB, &push, &norm);
}

//=========================================================================================================
// 線分＆点の最近距離ユーティリティ
//=========================================================================================================

XMFLOAT3 ClosestPointOnSegment(const XMFLOAT3& p, const XMFLOAT3& a, const XMFLOAT3& b)
{
    XMFLOAT3 ab = { b.x - a.x, b.y - a.y, b.z - a.z };
    XMFLOAT3 ap = { p.x - a.x, p.y - a.y, p.z - a.z };

    float abLenSq = ab.x * ab.x + ab.y * ab.y + ab.z * ab.z;
    if (abLenSq < 1e-8f) return a;

    float t = (ap.x * ab.x + ap.y * ab.y + ap.z * ab.z) / abLenSq;
    t = Clamp(t, 0.0f, 1.0f);

    return XMFLOAT3(a.x + ab.x * t, a.y + ab.y * t, a.z + ab.z * t);
}

float ClosestParamOnSegment(const XMFLOAT3& p, const XMFLOAT3& a, const XMFLOAT3& b)
{
    XMFLOAT3 ab = b - a;
    XMFLOAT3 ap = p - a;
    float abLenSq = Dot(ab, ab);
    if (abLenSq < 1e-8f) return 0.0f;
    return Clamp(Dot(ap, ab) / abLenSq, 0.0f, 1.0f);
}

XMFLOAT3 ClosestPointOnOBB(
    const XMFLOAT3& point,
    const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg)
{
    float yawRad = XMConvertToRadians(boxYawDeg);
    float cosY   = cosf(yawRad);
    float sinY   = sinf(yawRad);

    XMFLOAT3 d = point - boxC;
    XMFLOAT3 localP = {
        d.x * cosY + d.z * sinY,
        d.y,
       -d.x * sinY + d.z * cosY
    };

    XMFLOAT3 clamped = {
        Clamp(localP.x, -boxH.x, boxH.x),
        Clamp(localP.y, -boxH.y, boxH.y),
        Clamp(localP.z, -boxH.z, boxH.z)
    };

    return XMFLOAT3{
        boxC.x + clamped.x * cosY - clamped.z * sinY,
        boxC.y + clamped.y,
        boxC.z + clamped.x * sinY + clamped.z * cosY
    };
}

static XMFLOAT3 ClosestPointOnOBB_Full(
    const XMFLOAT3& point,
    const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg)
{
    float yawRad = XMConvertToRadians(boxYawDeg);
    float cy = cosf(yawRad), sy = sinf(yawRad);

    XMFLOAT3 axisX = { cy, 0, sy };
    XMFLOAT3 axisY = { 0, 1, 0 };
    XMFLOAT3 axisZ = { -sy, 0, cy };

    XMFLOAT3 d = point - boxC;
    float projX = Clamp(Dot(d, axisX), -boxH.x, boxH.x);
    float projY = Clamp(Dot(d, axisY), -boxH.y, boxH.y);
    float projZ = Clamp(Dot(d, axisZ), -boxH.z, boxH.z);

    return boxC + axisX * projX + axisY * projY + axisZ * projZ;
}

float SegmentToOBB_Closest(
    const XMFLOAT3& segA, const XMFLOAT3& segB,
    const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg,
    XMFLOAT3* outSegPoint, XMFLOAT3* outBoxPoint)
{
    XMFLOAT3 segDir = segB - segA;

    float    t          = 0.5f;
    XMFLOAT3 bestSeg{}, bestBox{};
    float    bestDistSq = FLT_MAX;

    for (int iter = 0; iter < 8; ++iter)
    {
        XMFLOAT3 segPt    = segA + segDir * t;
        XMFLOAT3 boxPt    = ClosestPointOnOBB_Full(segPt, boxC, boxH, boxYawDeg);
        float    tNew      = ClosestParamOnSegment(boxPt, segA, segB);
        XMFLOAT3 segPtNew = segA + segDir * tNew;

        XMFLOAT3 diff   = segPtNew - boxPt;
        float    distSq = Dot(diff, diff);

        if (distSq < bestDistSq) { bestDistSq = distSq; bestSeg = segPtNew; bestBox = boxPt; }
        if (fabsf(tNew - t) < 1e-5f) break;
        t = tNew;
    }

    for (int e = 0; e < 2; ++e)
    {
        XMFLOAT3 segPt = (e == 0) ? segA : segB;
        XMFLOAT3 boxPt = ClosestPointOnOBB_Full(segPt, boxC, boxH, boxYawDeg);
        XMFLOAT3 diff  = segPt - boxPt;
        float distSq   = Dot(diff, diff);
        if (distSq < bestDistSq) { bestDistSq = distSq; bestSeg = segPt; bestBox = boxPt; }
    }

    if (outSegPoint) *outSegPoint = bestSeg;
    if (outBoxPoint) *outBoxPoint = bestBox;
    return sqrtf(bestDistSq);
}

XMFLOAT3 GetOBBEscapeNormal(
    const XMFLOAT3& point,
    const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYawDeg,
    float* outPenetration)
{
    float yawRad = XMConvertToRadians(boxYawDeg);
    float cy = cosf(yawRad), sy = sinf(yawRad);

    XMFLOAT3 axes[3] = { { cy, 0, sy }, { 0, 1, 0 }, { -sy, 0, cy } };
    float    halfs[3] = { boxH.x, boxH.y, boxH.z };

    XMFLOAT3 d = point - boxC;
    float    minPen     = FLT_MAX;
    XMFLOAT3 bestNormal = { 0, 1, 0 };

    for (int i = 0; i < 3; ++i)
    {
        float proj = Dot(d, axes[i]);
        float pen  = halfs[i] - fabsf(proj);
        if (pen < minPen) { minPen = pen; bestNormal = axes[i] * ((proj >= 0) ? 1.0f : -1.0f); }
    }

    if (outPenetration) *outPenetration = minPen;
    return bestNormal;
}

void ClosestPointsOnSegments(
    const XMFLOAT3& a0, const XMFLOAT3& a1,
    const XMFLOAT3& b0, const XMFLOAT3& b1,
    XMFLOAT3& closestA, XMFLOAT3& closestB)
{
    XMFLOAT3 d1 = a1 - a0;
    XMFLOAT3 d2 = b1 - b0;
    XMFLOAT3 r  = a0 - b0;

    float a = Dot(d1, d1);
    float e = Dot(d2, d2);
    float f = Dot(d2, r);

    float s, t;

    if (a < 1e-8f && e < 1e-8f) { s = t = 0.0f; }
    else if (a < 1e-8f)         { s = 0.0f; t = Clamp(f / e, 0.0f, 1.0f); }
    else
    {
        float c = Dot(d1, r);
        if (e < 1e-8f) { t = 0.0f; s = Clamp(-c / a, 0.0f, 1.0f); }
        else
        {
            float b     = Dot(d1, d2);
            float denom = a * e - b * b;

            s = (denom != 0.0f) ? Clamp((b * f - c * e) / denom, 0.0f, 1.0f) : 0.0f;
            t = (b * s + f) / e;

            if (t < 0.0f)      { t = 0.0f; s = Clamp(-c / a, 0.0f, 1.0f); }
            else if (t > 1.0f) { t = 1.0f; s = Clamp((b - c) / a, 0.0f, 1.0f); }
        }
    }

    closestA = a0 + d1 * s;
    closestB = b0 + d2 * t;
}

//=========================================================================================================
// カプセル vs OBB
//=========================================================================================================

bool Resolve_Capsule2D_OBB(
    const Capsule2D& capsule,
    const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg,
    XMFLOAT3* outPush, XMFLOAT3* outNormal)
{
    if (outPush)   *outPush   = { 0, 0, 0 };
    if (outNormal) *outNormal = { 0, 1, 0 };

    XMFLOAT3 capTop    = capsule.GetTop();
    XMFLOAT3 capBottom = capsule.GetBottom();

    XMFLOAT3 boundHalf = capsule.GetBoundingHalfSize();
    float dx = fabsf(capsule.center.x - boxCenter.x);
    float dy = fabsf(capsule.center.y - boxCenter.y);
    float dz = fabsf(capsule.center.z - boxCenter.z);
    if (dx > boundHalf.x + boxHalf.x + 0.1f ||
        dy > boundHalf.y + boxHalf.y + 0.1f ||
        dz > boundHalf.z + boxHalf.z + 0.1f)
        return false;

    XMFLOAT3 closestSeg, closestBox;
    float dist = SegmentToOBB_Closest(capTop, capBottom, boxCenter, boxHalf, boxYawDeg,
                                      &closestSeg, &closestBox);

    const float kContactEps = 0.01f;
    if (dist > capsule.radius + kContactEps) return false;

    float    penetration;
    XMFLOAT3 normal;

    if (dist > 1e-5f)
    {
        normal      = Normalize(closestSeg - closestBox);
        penetration = (dist < capsule.radius) ? (capsule.radius - dist) : 0.0f;
    }
    else
    {
        normal      = GetOBBEscapeNormal(closestSeg, boxCenter, boxHalf, boxYawDeg, &penetration);
        penetration += capsule.radius;
    }

    if (outPush)   *outPush   = normal * penetration;
    if (outNormal) *outNormal = normal;
    return true;
}

bool Capsule2D_Intersect_OBB(
    const Capsule2D& capsule,
    const XMFLOAT3& boxCenter, const XMFLOAT3& boxHalf, float boxYawDeg)
{
    XMFLOAT3 push, normal;
    return Resolve_Capsule2D_OBB(capsule, boxCenter, boxHalf, boxYawDeg, &push, &normal);
}

//=========================================================================================================
// 2D多角形ユーティリティ（影衝突用）
//=========================================================================================================

bool PointInPolygon2D(float px, float py, const std::vector<XMFLOAT2>& poly)
{
    if (poly.size() < 3) return false;
    bool inside = false;
    int  n = (int)poly.size();

    for (int i = 0, j = n - 1; i < n; j = i++)
    {
        float xi = poly[i].x, yi = poly[i].y;
        float xj = poly[j].x, yj = poly[j].y;
        if (((yi > py) != (yj > py)) &&
            (px < (xj - xi) * (py - yi) / (yj - yi) + xi))
            inside = !inside;
    }
    return inside;
}

float PointToPolygonEdgeDist2D(
    const XMFLOAT2& p, const std::vector<XMFLOAT2>& poly,
    XMFLOAT2* outClosest, XMFLOAT2* outEdgeNormal)
{
    float    bestDistSq = FLT_MAX;
    XMFLOAT2 bestClosest = p;
    XMFLOAT2 bestNormal  = { 0, 1 };
    int      n = (int)poly.size();

    for (int i = 0; i < n; ++i)
    {
        XMFLOAT2 a  = poly[i];
        XMFLOAT2 b  = poly[(i + 1) % n];
        XMFLOAT2 ab = { b.x - a.x, b.y - a.y };
        XMFLOAT2 ap = { p.x - a.x, p.y - a.y };

        float abLenSq = ab.x * ab.x + ab.y * ab.y;
        float t = 0.0f;
        if (abLenSq > 1e-8f)
            t = Clamp((ap.x * ab.x + ap.y * ab.y) / abLenSq, 0.0f, 1.0f);

        XMFLOAT2 closest = { a.x + ab.x * t, a.y + ab.y * t };
        float dx     = p.x - closest.x;
        float dy     = p.y - closest.y;
        float distSq = dx * dx + dy * dy;

        if (distSq < bestDistSq)
        {
            bestDistSq  = distSq;
            bestClosest = closest;
            float eLen  = sqrtf(abLenSq);
            if (eLen > 1e-6f)
                bestNormal = { -ab.y / eLen, ab.x / eLen };
        }
    }

    if (outClosest)    *outClosest    = bestClosest;
    if (outEdgeNormal) *outEdgeNormal = bestNormal;
    return sqrtf(bestDistSq);
}

float SignedDistToPolygon2D(
    const XMFLOAT2& p, const std::vector<XMFLOAT2>& poly,
    XMFLOAT2* outNormal)
{
    int n = (int)poly.size();
    if (n < 3) return FLT_MAX;

    float maxSignedDist = -FLT_MAX;
    int   closestEdge   = 0;

    for (int i = 0; i < n; ++i)
    {
        XMFLOAT2 a    = poly[i];
        XMFLOAT2 b    = poly[(i + 1) % n];
        XMFLOAT2 e    = { b.x - a.x, b.y - a.y };
        float    eLen  = sqrtf(e.x * e.x + e.y * e.y);
        if (eLen < 1e-6f) continue;

        XMFLOAT2 nIn = { -e.y / eLen, e.x / eLen };
        XMFLOAT2 ap  = { p.x - a.x, p.y - a.y };
        float dist   = ap.x * nIn.x + ap.y * nIn.y;

        if (-dist > maxSignedDist)
        {
            maxSignedDist = -dist;
            closestEdge   = i;
        }
    }

    XMFLOAT2 a    = poly[closestEdge];
    XMFLOAT2 b    = poly[(closestEdge + 1) % n];
    XMFLOAT2 e    = { b.x - a.x, b.y - a.y };
    float    eLen  = sqrtf(e.x * e.x + e.y * e.y);
    if (eLen > 1e-6f && outNormal)
        *outNormal = { -e.y / eLen, e.x / eLen };

    XMFLOAT2 dummy;
    if (maxSignedDist <= 0.0f)
        return -PointToPolygonEdgeDist2D(p, poly, &dummy, outNormal);
    else
        return PointToPolygonEdgeDist2D(p, poly, &dummy, outNormal);
}