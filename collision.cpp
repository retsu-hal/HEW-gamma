#include "Collision.h"
#include "player3D.h"
#include "camera.h"
#include "direct3d.h"
#include "Player2D.h"
#include "debug.h"
#include "MathUtil.h"
#include <vector>


using namespace DirectX;
using namespace mu;

static bool debugMode = TRUE;

static std::vector<const ShadowPrism*> g_ShadowPrisms;
static ShadowDebugOptions g_ShadowDebugOpts;

struct ExtraDebugBox
{
	bool isOBB = false;
	XMFLOAT3 center{};
	XMFLOAT3 half{};
	XMFLOAT3 rotDeg{};
	ImU32 color = 0;
};

static std::vector<ExtraDebugBox> g_ExtraBoxes;

//=========================================================================================================
// �{�[���ƃt�B�[���h�̓����蔻��
//=========================================================================================================

void Collision_SetShadowPrisms(const std::vector<const ShadowPrism*>& prisms)
{
	g_ShadowPrisms = prisms;
}

const std::vector<const ShadowPrism*>& Collision_GetShadowPrisms()
{
	return g_ShadowPrisms;
}

void Collision_SetShadowPrism(const ShadowPrism* prism)
{
	g_ShadowPrisms.clear();
	if (prism && prism->isValid)
	{
		g_ShadowPrisms.push_back(prism);
	}
}

const ShadowPrism* Collision_GetShadowPrism()
{
	return g_ShadowPrisms.empty() ? nullptr : g_ShadowPrisms[0];
}

static ImU32 MakeColor(unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	return ((ImU32)a << 24) | ((ImU32)b << 16) | ((ImU32)g << 8) | (ImU32)r;
}

// �{�b�N�X�̔����̃T�C�Y���擾
static XMFLOAT3 Field_GetHalfSize(const MAPDATA& m)
{
	if (m.useCustomCollider)
	{
		return m.colliderHalf;
	}
	return XMFLOAT3{
		BOX_RADIUS * m.scale.x,
		BOX_RADIUS * m.scale.y,
		BOX_RADIUS * m.scale.z
	};
}

// ���[���h���W���X�N���[�����W�ɕϊ�
struct ScreenPt { ImVec2 pos; bool valid; };
static ScreenPt WorldToScreen(const XMFLOAT3& p)
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

// 3D���C����`��
static void DrawLine3D(const XMFLOAT3& a, const XMFLOAT3& b, ImU32 col, float thick = 1.0f)
{
	ScreenPt sa = WorldToScreen(a);
	ScreenPt sb = WorldToScreen(b);
	if (sa.valid && sb.valid)
		ImGui::GetBackgroundDrawList()->AddLine(sa.pos, sb.pos, col, thick);
}
// 3D�|�C���g��`��
static void DrawPoint3D(const XMFLOAT3& p, ImU32 col, float size = 4.0f)
{
	ScreenPt sp = WorldToScreen(p);
	if (sp.valid)
		ImGui::GetBackgroundDrawList()->AddCircleFilled(sp.pos, size, col);
}


// AABB��`��
static void DebugDrawAABB(const XMFLOAT3& c, const XMFLOAT3& h, ImU32 col)
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

// OBB��Yaw��]�ŕ`��
static void DebugDrawOBB_Yaw(const XMFLOAT3& c, const XMFLOAT3& h, float yawDeg, ImU32 col)
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

// �ȉ~�̂�`��
static void DebugDrawEllipsoid(const XMFLOAT3& c, const XMFLOAT3& r, ImU32 col, int seg = 24)
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


static void DebugDrawShadowPrism(const ShadowPrism& prism, const ShadowDebugOptions& opts)
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

	if (opts.drawAABB)
	{
		XMFLOAT3 c = (prism.aabbMin + prism.aabbMax) * 0.5f;
		XMFLOAT3 h = (prism.aabbMax - prism.aabbMin) * 0.5f;
		DebugDrawAABB(c, h, opts.aabbColor);
	}
}


// �v���C���[�̌ő̃R���C�_�[���S���擾
static XMFLOAT3 GetPlayerSolidCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetSolidHalfSize().y;
	return c;
}
// �v���C���[2D�̌ő̃R���C�_�[���S���擾
static XMFLOAT3 GetPlayer2DSolidCollider()
{
	PLAYER* p = GetPlayer2D();
	XMFLOAT3 c = p->Position;
	c.y += Player2D_GetSolidHalfSize().y + 0.1f;
	return c;
}
// �v���C���[�̃g���K�[�R���C�_�[���S���擾
static XMFLOAT3 GetPlayerTriggerCollider()
{
	PLAYER* p = GetPlayer3D();
	XMFLOAT3 c = p->Position;
	c.y += Player3D_GetTriggerHalfSize().y;
	return c;
}

// �t�B�[���h���ő̂��ǂ������擾
static bool Field_IsSolid(FIELD t)
{
	switch (t)
	{
	case FIELD_GROUND:
	case FIELD_WALL:
	case FIELD_OBJ_BOX:
	case FIELD_EMPTY_BOX:
	case FIELD_OBJ_1:
	case FIELD_SEESAW_1:
	case FIELD_SEESAW_2:
	case FIELD_MANHOLE:
		return true;
	default:
		return false;
	}
}
// �t�B�[���h���g���K�[���ǂ������擾
static bool Field_IsTrigger(FIELD t)
{
	switch (t)
	{
	case FIELD_GOAL:
	case FIELD_OBJ_1:
	case FIELD_OBJ_2:
		return true;
	default:
		return false;
	}
}
// �t�B�[���h���g���K�[���ǂ������擾
//For debug only

// OBB vs OBB ���Z�iYaw��]�̂ݑΉ��j
static bool OBB_Intersect_Yaw(
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

// �ȉ~�� vs OBB ���Z�iYaw��]�̂ݑΉ��j
static bool Resolve_Ellipsoid_OBB_Yaw(
	const XMFLOAT3& ellC, const XMFLOAT3& ellR,
	const XMFLOAT3& boxC, const XMFLOAT3& boxH, float boxYaw,
	XMFLOAT3* outPush, XMFLOAT3* outNormal)
{
	if (outPush) *outPush = { 0,0,0 };
	if (outNormal) *outNormal = { 0,1,0 };

	XMFLOAT3 invR = { 1.0f / ellR.x, 1.0f / ellR.y, 1.0f / ellR.z };
	XMFLOAT3 cS = Mul(ellC, invR);
	XMFLOAT3 bS = Mul(boxC, invR);
	XMFLOAT3 hS = Mul(boxH, invR);

	float yawRad = XMConvertToRadians(boxYaw);

	XMFLOAT3 d = cS - bS;
	XMFLOAT3 dl = RotateY(d, -yawRad);
	dl.x = Clamp(dl.x, -hS.x, hS.x);
	dl.y = Clamp(dl.y, -hS.y, hS.y);
	dl.z = Clamp(dl.z, -hS.z, hS.z);
	XMFLOAT3 qS = bS + RotateY(dl, yawRad);

	XMFLOAT3 dS = cS - qS;
	float dist = Length(dS);

	if (dist >= 1.0f) return false;

	XMFLOAT3 nS;
	float pen;

	if (dist > 1e-6f)
	{
		nS = dS * (1.0f / dist);
		pen = 1.0f - dist;
	}
	else
	{
		XMFLOAT3 lp = RotateY(cS - bS, -yawRad);
		float dx = hS.x - fabsf(lp.x);
		float dy = hS.y - fabsf(lp.y);
		float dz = hS.z - fabsf(lp.z);

		XMFLOAT3 ln = { 0,1,0 };
		if (dx <= dy && dx <= dz) { ln = { (lp.x >= 0) ? 1.0f : -1.0f, 0, 0 }; pen = 1.0f + dx; }
		else if (dy <= dz) { ln = { 0, (lp.y >= 0) ? 1.0f : -1.0f, 0 }; pen = 1.0f + dy; }
		else { ln = { 0, 0, (lp.z >= 0) ? 1.0f : -1.0f }; pen = 1.0f + dz; }

		nS = RotateY(ln, yawRad);
	}

	XMFLOAT3 pushS = nS * pen;
	XMFLOAT3 pushW = { pushS.x * ellR.x, pushS.y * ellR.y, pushS.z * ellR.z };
	XMFLOAT3 nW = Normalize(XMFLOAT3{ nS.x * ellR.x, nS.y * ellR.y, nS.z * ellR.z });

	if (outPush) *outPush = pushW;
	if (outNormal) *outNormal = nW;
	return true;
}

// �g���K�[�����������ʂ��v�Z�iPlayer3d Yaw��j
static TRIGGER_SIDE CalcTriggerSide(const XMFLOAT3& playerC, const XMFLOAT3& targetC)
{
	PLAYER* p = GetPlayer3D();
	if (!p) return TRIGGER_SIDE_NONE;

	float yaw = XMConvertToRadians(p->Rotation.y);
	XMFLOAT3 fwd = Normalize2D({ sinf(yaw), 0, cosf(yaw) });
	XMFLOAT3 right = { fwd.z, 0, -fwd.x };
	XMFLOAT3 to = { targetC.x - playerC.x, 0, targetC.z - playerC.z };

	float f = Dot2D(to, fwd);
	float r = Dot2D(to, right);

	if (fabsf(f) < 1e-5f && fabsf(r) < 1e-5f) return TRIGGER_SIDE_NONE;
	if (fabsf(f) >= fabsf(r)) return (f >= 0) ? TRIGGER_SIDE_FRONT : TRIGGER_SIDE_BACK;
	return (r >= 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
}


void Collision_SetShadowDebugOptions(const ShadowDebugOptions& options)
{
	g_ShadowDebugOpts = options;
}

void Collision_DebugClearExtraBoxes()
{
	g_ExtraBoxes.clear();
}
// AABB���f�o�b�O�`�惊�X�g�ɒǉ�
void Collision_DebugAddExtraAABB(const XMFLOAT3& c, const XMFLOAT3& h,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	ExtraDebugBox box;
	box.isOBB = false;
	box.center = c;
	box.half = h;
	box.color = MakeColor(r, g, b, a);
	g_ExtraBoxes.push_back(box);
}
// OBB���f�o�b�O�`�惊�X�g�ɒǉ�
void Collision_DebugAddExtraOBB(const XMFLOAT3& c, const XMFLOAT3& h, const XMFLOAT3& rot,
	unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
	ExtraDebugBox box;
	box.isOBB = true;
	box.center = c;
	box.half = h;
	box.rotDeg = rot;
	box.color = MakeColor(r, g, b, a);
	g_ExtraBoxes.push_back(box);
}

bool Resolve_OBB_OBB_ZY(
	const XMFLOAT3& posA, const XMFLOAT3& halfA, float rotZRadA,  // �v���C���[�iZ��]�j
	const XMFLOAT3& posB, const XMFLOAT3& halfB, float rotYDegB,  // �t�B�[���h�iY��]�j
	XMFLOAT3* outPush, XMFLOAT3* outNorm)
{
	// �v���C���[��Z��]�s��i2D���ʂł̉�]�j
	XMMATRIX rotMatA = XMMatrixRotationZ(rotZRadA);

	// �t�B�[���h��Y��]�s��
	float rotYRadB = XMConvertToRadians(rotYDegB);
	XMMATRIX rotMatB = XMMatrixRotationY(rotYRadB);

	// �eOBB�̃��[�J�������擾
	XMFLOAT3 axesA[3], axesB[3];

	// �v���C���[�̎��iZ��]�j
	XMVECTOR axA0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatA);
	XMVECTOR axA1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatA);
	XMVECTOR axA2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatA);
	XMStoreFloat3(&axesA[0], axA0);
	XMStoreFloat3(&axesA[1], axA1);
	XMStoreFloat3(&axesA[2], axA2);

	// �t�B�[���h�̎��iY��]�j
	XMVECTOR axB0 = XMVector3TransformNormal(XMVectorSet(1, 0, 0, 0), rotMatB);
	XMVECTOR axB1 = XMVector3TransformNormal(XMVectorSet(0, 1, 0, 0), rotMatB);
	XMVECTOR axB2 = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), rotMatB);
	XMStoreFloat3(&axesB[0], axB0);
	XMStoreFloat3(&axesB[1], axB1);
	XMStoreFloat3(&axesB[2], axB2);

	// ���S�ԃx�N�g��
	XMVECTOR vD = XMVectorSet(posA.x - posB.x, posA.y - posB.y, posA.z - posB.z, 0);

	float halfExtA[3] = { halfA.x, halfA.y, halfA.z };
	float halfExtB[3] = { halfB.x, halfB.y, halfB.z };

	float minPen = FLT_MAX;
	XMFLOAT3 minAxis = { 0, 1, 0 };

	// 15����SAT����
	auto TestAxis = [&](XMVECTOR axis) -> bool
		{
			float len = XMVectorGetX(XMVector3Length(axis));
			if (len < 1e-6f) return true;  // ���s�Ȏ��̓X�L�b�v

			axis = XMVector3Normalize(axis);

			// �eOBB�̓��e���a
			float rA = 0, rB = 0;
			for (int i = 0; i < 3; i++)
			{
				XMVECTOR axi = XMLoadFloat3(&axesA[i]);
				rA += halfExtA[i] * fabsf(XMVectorGetX(XMVector3Dot(axis, axi)));
			}
			for (int i = 0; i < 3; i++)
			{
				XMVECTOR axi = XMLoadFloat3(&axesB[i]);
				rB += halfExtB[i] * fabsf(XMVectorGetX(XMVector3Dot(axis, axi)));
			}

			float dist = fabsf(XMVectorGetX(XMVector3Dot(vD, axis)));
			float pen = (rA + rB) - dist;

			if (pen < 0) return false;  // ���������� �� �Փ˂Ȃ�

			if (pen < minPen)
			{
				minPen = pen;
				XMStoreFloat3(&minAxis, axis);
			}
			return true;
		};

	// A ��3��
	if (!TestAxis(axA0)) return false;
	if (!TestAxis(axA1)) return false;
	if (!TestAxis(axA2)) return false;

	// B ��3��
	if (!TestAxis(axB0)) return false;
	if (!TestAxis(axB1)) return false;
	if (!TestAxis(axB2)) return false;

	// �O�ώ��i9���j
	if (!TestAxis(XMVector3Cross(axA0, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA0, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA1, axB2))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB0))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB1))) return false;
	if (!TestAxis(XMVector3Cross(axA2, axB2))) return false;

	// �����o������������iA��B���痣�������j
	XMVECTOR normAxis = XMLoadFloat3(&minAxis);
	if (XMVectorGetX(XMVector3Dot(vD, normAxis)) < 0)
	{
		normAxis = XMVectorNegate(normAxis);
	}

	XMVECTOR pushVec = normAxis * minPen;
	XMStoreFloat3(outPush, pushVec);
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
// 2D�p�g���K�[��������i�v���C���[�̉�]���l���j
//=========================================================================================================
static TRIGGER_SIDE CalcTriggerSide2D(const XMFLOAT3& playerPos, float playerRotZ, const XMFLOAT3& triggerPos)
{
	// �v���C���[����g���K�[�ւ̃x�N�g��
	float dx = triggerPos.x - playerPos.x;
	float dy = triggerPos.y - playerPos.y;

	// �v���C���[�̉�]���l�����ă��[�J�����W�ɕϊ�
	float radZ = XMConvertToRadians(playerRotZ);
	float cosZ = cosf(-radZ);  // �t��]�Ń��[�J�����W��
	float sinZ = sinf(-radZ);

	float localX = dx * cosZ - dy * sinZ;
	float localY = dx * sinZ + dy * cosZ;

	// ���[�J�����W�ł̕�������
	if (fabsf(localX) > fabsf(localY))
	{
		return (localX > 0) ? TRIGGER_SIDE_RIGHT : TRIGGER_SIDE_LEFT;
	}
	else
	{
		return (localY > 0) ? TRIGGER_SIDE_TOP : TRIGGER_SIDE_BOTTOM;
	}
}


// �v���C���[3D�ƃt�B�[���h�̓����蔻��
int Player3DField_Collision()
{
	int hit = HIT_NONE;

	PLAYER* player = GetPlayer3D();
	std::vector<MAPDATA>& map = GetFieldMap();
	if (!player || map.empty()) return hit;

	player->isGround = false;
	XMFLOAT3 ellR = Player3D_GetSolidHalfSize();
	XMFLOAT3 ellC = GetPlayerSolidCollider();

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsSolid(map[i].no)) continue;

		// �� �������ΰ�壬��������?
		if (map[i].no == FIELD_SEESAW_2) continue;

		float boxYaw = (map[i].no == FIELD_OBJ_1) ? map[i].rotate.y : 0.0f;

		XMFLOAT3 push, norm;
		// �� ʹ����ײ��ߴ�?
		if (!Resolve_Ellipsoid_OBB_Yaw(ellC, ellR, map[i].pos,
			Field_GetHalfSize(map[i]), boxYaw, &push, &norm))
			continue;

		ellC = ellC + push;

		float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

		if (ay >= ax && ay >= az)
		{
			if (norm.y > 0) { player->isGround = true; player->Velocity.y = 0; hit = HIT_GROUND; }
			else if (player->Velocity.y > 0) player->Velocity.y = 0;
		}
		else if (ax >= az)
		{
			player->Velocity.x = 0;
			hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
		}
		else
		{
			player->Velocity.z = 0;
			hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
		}
	}

	player->Position.x = ellC.x;
	player->Position.y = ellC.y - ellR.y;
	player->Position.z = ellC.z;

	return hit;
}

int Player2DField_Collision()
{
	int hit = HIT_NONE;

	PLAYER* player = GetPlayer2D();
	std::vector<MAPDATA>& map = GetFieldMap();
	if (!player || map.empty()) return hit;

	player->isGround = false;
	XMFLOAT3 halfSize = Player2D_GetSolidHalfSize();

	// 2D�v���C���[�̓����蔻�蒆�S�i��������璆�S��֕ϊ��j
	XMFLOAT3 colliderCenter = XMFLOAT3(
		player->Position.x,
		player->Position.y + halfSize.y,
		player->Position.z
	);

	// �v���C���[��Z����]�p�x�i�x�����W�A���j
	float playerZRot = XMConvertToRadians(player->Rotation.z);

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsSolid(map[i].no)) continue;

		XMFLOAT3 push, norm;

		// OBB vs OBB�i�v���C���[��Z��]�A�t�B�[���h��Y��]�j
		if (!Resolve_OBB_OBB_ZY(
			colliderCenter, halfSize, playerZRot,      // �v���C���[�iZ��]�j
			map[i].pos, Field_GetHalfSize(map[i]), map[i].rotate.y,  // �t�B�[���h�iY��]�j
			&push, &norm))
			continue;

		colliderCenter = XMFLOAT3(
			colliderCenter.x + push.x,
			colliderCenter.y + push.y,
			colliderCenter.z + push.z
		);

		float ax = fabsf(norm.x), ay = fabsf(norm.y), az = fabsf(norm.z);

		if (ay >= ax && ay >= az)
		{
			// �㉺�̏Փ�
			if (norm.y > 0)
			{
				player->isGround = true;
				player->Velocity.y = 0;
				hit = HIT_GROUND;
			}
			else if (player->Velocity.y > 0)
			{
				player->Velocity.y = 0;
			}
		}
		else if (ax >= az)
		{
			// ���E�̕ǏՓ�
			player->Velocity.x = 0;
			hit = (norm.x > 0) ? HIT_WALL_PlusX : HIT_WALL_NegX;
		}
		else
		{
			// �O��̕ǏՓ�?
			player->Velocity.z = 0;
			hit = (norm.z > 0) ? HIT_WALL_PlusZ : HIT_WALL_NegZ;
		}
	}

	// ���S���W���瑫�����W�֖߂�
	player->Position.x = colliderCenter.x;
	player->Position.y = colliderCenter.y - halfSize.y;
	player->Position.z = colliderCenter.z;

	return hit;
}



// �v���C���[�̃g���K�[�����蔻��
bool Collision_PlayerTrigger(TRIGGER_HIT* outHit, float extraRange)
{
	if (outHit) *outHit = TRIGGER_HIT{};

	PLAYER* p = GetPlayer3D();
	if (!p) return false;

	auto& map = GetFieldMap();
	if (map.empty()) return false;

	XMFLOAT3 pHalf = Player3D_GetTriggerHalfSize();
	XMFLOAT3 pC = GetPlayerTriggerCollider();
	float pYaw = p->Rotation.y;

	bool found = false;
	float bestD2 = 1e30f;
	TRIGGER_HIT best;

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;

		XMFLOAT3 tHalf = Field_GetHalfSize(map[i]);
		tHalf.x += extraRange; tHalf.y += extraRange; tHalf.z += extraRange;

		if (!OBB_Intersect_Yaw(pC, pHalf, pYaw, map[i].pos, tHalf, map[i].rotate.y))
			continue;

		float dx = map[i].pos.x - pC.x;
		float dz = map[i].pos.z - pC.z;
		float d2 = dx * dx + dz * dz;

		if (!found || d2 < bestD2)
		{
			found = true;
			bestD2 = d2;
			best.hit = true;
			best.mapIndex = i;
			best.type = map[i].no;
			best.side = CalcTriggerSide(pC, map[i].pos);
		}
	}

	if (!found) return false;
	if (outHit) *outHit = best;
	return true;
}

bool Collision_Player2DTrigger(TRIGGER_HIT* outHit, float extraRange)
{
	if (outHit) *outHit = TRIGGER_HIT{};

	PLAYER* p = GetPlayer2D();
	if (!p) return false;

	auto& map = GetFieldMap();
	if (map.empty()) return false;

	XMFLOAT3 pHalf = Player2D_GetSolidHalfSize();
	pHalf.x += extraRange;
	pHalf.y += extraRange;
	pHalf.z += extraRange;

	XMFLOAT3 pC = XMFLOAT3(
		p->Position.x,
		p->Position.y + Player2D_GetSolidHalfSize().y,
		p->Position.z
	);

	float pZRot = XMConvertToRadians(p->Rotation.z);

	bool found = false;
	float bestD2 = 1e30f;
	TRIGGER_HIT best;

	for (size_t i = 0; i < map.size(); ++i)
	{
		if (!Field_IsTrigger(map[i].no)) continue;

		XMFLOAT3 tHalf = Field_GetHalfSize(map[i]);
		tHalf.x += extraRange;
		tHalf.y += extraRange;
		tHalf.z += extraRange;

		// OBB vs OBB ��������
		if (!OBB_Intersect_ZY(pC, pHalf, pZRot,
			map[i].pos, tHalf, map[i].rotate.y))
			continue;

		float dx = map[i].pos.x - pC.x;
		float dy = map[i].pos.y - pC.y;
		float d2 = dx * dx + dy * dy;

		if (!found || d2 < bestD2)
		{
			found = true;
			bestD2 = d2;
			best.hit = true;
			best.mapIndex = (int)i;
			best.type = map[i].no;
			best.side = CalcTriggerSide2D(pC, p->Rotation.z, map[i].pos);
		}
	}

	if (!found) return false;
	if (outHit) *outHit = best;
	return true;
}


//=========================================================================================================
// �_���v���Y�����ɂ��邩�`�F�b�N�i2D���p�`����j
//=========================================================================================================
static bool PointInPolygon2D(float px, float py, const std::vector<XMFLOAT2>& poly)
{
	if (poly.size() < 3) return false;

	bool inside = false;
	int n = (int)poly.size();

	for (int i = 0, j = n - 1; i < n; j = i++)
	{
		float xi = poly[i].x, yi = poly[i].y;
		float xj = poly[j].x, yj = poly[j].y;

		if (((yi > py) != (yj > py)) &&
			(px < (xj - xi) * (py - yi) / (yj - yi) + xi))
		{
			inside = !inside;
		}
	}
	return inside;
}

//=========================================================================================================
// �v���C���[2D�Ɖe�v���Y���̓����蔻��
//=========================================================================================================
static float DotXZ(const XMFLOAT3& a, const XMFLOAT3& b)
{
	return a.x * b.x + a.z * b.z;
}

static float ProjectRadiusOnAxis_Yaw(const XMFLOAT3& axis, const XMFLOAT3& half, float yawDeg)
{
	const float yaw = XMConvertToRadians(yawDeg);
	const XMFLOAT3 right = { cosf(yaw), 0.0f, -sinf(yaw) };
	const XMFLOAT3 fwd = { sinf(yaw), 0.0f,  cosf(yaw) };
	const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };

	return fabsf(Dot(axis, right)) * half.x
		+ fabsf(Dot(axis, up)) * half.y
		+ fabsf(Dot(axis, fwd)) * half.z;
}


bool Player2DShadow_Collision()
{
	PLAYER* player = GetPlayer2D();
	if (!player) return false;

	const std::vector<const ShadowPrism*>& prisms = Collision_GetShadowPrisms();
	if (prisms.empty()) return false;

	const XMFLOAT3 half = Player2D_GetSolidHalfSize();

	const float skin = 0.01f;
	const float eps = 1e-6f;
	const float hitEps = 1e-4f;

	XMFLOAT3 dXZ = { player->Velocity.x, 0.0f, player->Velocity.z };

	bool hitAny = false;

	auto Area2 = [](const std::vector<XMFLOAT2>& p) -> float
		{
			float a = 0.0f;
			const int n = (int)p.size();
			for (int i = 0, j = n - 1; i < n; j = i++)
				a += p[j].x * p[i].y - p[i].x * p[j].y;
			return a;
		};

	auto Dot2 = [](const XMFLOAT2& a, const XMFLOAT2& b) -> float { return a.x * b.x + a.y * b.y; };
	auto Sub2 = [](const XMFLOAT2& a, const XMFLOAT2& b) -> XMFLOAT2 { return { a.x - b.x, a.y - b.y }; };
	auto Len2 = [](const XMFLOAT2& v) -> float { return sqrtf(v.x * v.x + v.y * v.y); };

	auto SweepEnterExpanded = [&](const ShadowPrism* prism,
		const std::vector<XMFLOAT2>& polyCCW,
		const XMFLOAT2& p0,
		const XMFLOAT2& dp,
		float yawDeg,
		float* outEnterT) -> bool
		{
			const int n = (int)polyCCW.size();
			float tEnter = 0.0f;
			float tExit = 1.0f;

			for (int i = 0; i < n; ++i)
			{
				const XMFLOAT2 a = polyCCW[i];
				const XMFLOAT2 b = polyCCW[(i + 1) % n];
				const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

				XMFLOAT2 nIn = { -e.y, e.x };
				float nl = Len2(nIn);
				if (nl < 1e-6f) continue;
				nIn.x /= nl; nIn.y /= nl;

				const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
				const float rEdge = ProjectRadiusOnAxis_Yaw(axisW, half, yawDeg) + skin;

				const float s0 = Dot2(nIn, Sub2(p0, a));
				const float sv = Dot2(nIn, dp);

				if (fabsf(sv) < 1e-6f)
				{
					if (s0 < -rEdge) return false;
					continue;
				}

				const float t = (-rEdge - s0) / sv;

				if (sv > 0.0f)
				{
					if (t > tEnter) tEnter = t;
				}
				else
				{
					if (t < tExit) tExit = t;
				}

				if (tEnter > tExit) return false;
			}

			if (outEnterT) *outEnterT = tEnter;
			return true;
		};

	auto GetCenter = [&]() -> XMFLOAT3
		{
			return { player->Position.x, player->Position.y + half.y, player->Position.z };
		};

	for (const ShadowPrism* prism : prisms)
	{
		if (!prism || !prism->isValid) continue;
		if (prism->poly.size() < 3) continue;

		const float moveLen = sqrtf(dXZ.x * dXZ.x + dXZ.z * dXZ.z);
		const float gate = half.x + half.z + moveLen + 0.10f;
		const XMFLOAT3 cW0 = GetCenter();
		if (cW0.x < prism->aabbMin.x - gate || cW0.x > prism->aabbMax.x + gate ||
			cW0.z < prism->aabbMin.z - gate || cW0.z > prism->aabbMax.z + gate)
		{
			continue;
		}

		std::vector<XMFLOAT2> poly = prism->poly;
		if (Area2(poly) < 0.0f)
			std::reverse(poly.begin(), poly.end());

		const XMFLOAT3 cW = GetCenter();
		const XMFLOAT3 rel = cW - prism->origin;
		const float u0 = Dot(rel, prism->u);
		const float v0 = Dot(rel, prism->v);
		const float n0 = Dot(rel, prism->n);
		const XMFLOAT2 p0 = { u0, v0 };

		const float rN = ProjectRadiusOnAxis_Yaw(prism->n, half, player->Rotation.y);
		const bool overlapN = (n0 >= -rN) && (n0 <= prism->thickness + rN);

		if (prism->n.y > 0.7f)
		{
			const bool insideFoot = PointInPolygon2D(p0.x, p0.y, poly);

			const float bottomN = n0 - rN;

			if (insideFoot && player->Velocity.y <= 0.0f)
			{
				const float targetBottomN = prism->thickness;
				const float distToTop = bottomN - targetBottomN;

				if (distToTop <= 0.05f && distToTop >= -0.25f)
				{
					player->Position = player->Position - prism->n * distToTop;

					const float vN = Dot(player->Velocity, prism->n);
					if (vN < 0.0f)
						player->Velocity = player->Velocity - prism->n * vN;

					player->isGround = true;
					hitAny = true;
				}
			}
		}

		if (!overlapN) continue;

		{
			const float bottomN = n0 - rN;
			if (prism->n.y > 0.7f && bottomN >= prism->thickness - 0.02f)
				continue;
		}

		if (moveLen < eps) continue;

		XMFLOAT2 dp = { DotXZ(dXZ, prism->u), DotXZ(dXZ, prism->v) };
		if (fabsf(dp.x) + fabsf(dp.y) < 1e-7f) continue;

		bool insideExp0 = true;
		for (int i = 0; i < (int)poly.size(); ++i)
		{
			const XMFLOAT2 a = poly[i];
			const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
			const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

			XMFLOAT2 nIn = { -e.y, e.x };
			float nl = Len2(nIn);
			if (nl < 1e-6f) continue;
			nIn.x /= nl; nIn.y /= nl;

			const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
			const float rEdge = ProjectRadiusOnAxis_Yaw(axisW, half, player->Rotation.y) + skin;

			const float s0 = Dot2(nIn, Sub2(p0, a));
			if (s0 < -rEdge) { insideExp0 = false; break; }
		}

		if (!insideExp0)
		{
			float tEnter = 0.0f;
			if (SweepEnterExpanded(prism, poly, p0, dp, player->Rotation.y, &tEnter))
			{
				if (tEnter <= 1.0f && tEnter >= 0.0f)
				{
					if (tEnter < 1.0f)
					{
						const float tStop = (std::max)(0.0f, (std::min)(1.0f, tEnter - hitEps));
						dXZ.x *= tStop;
						dXZ.z *= tStop;
						hitAny = true;
					}
				}
			}
		}
		else
		{
			for (int pass = 0; pass < 2; ++pass)
			{
				XMFLOAT2 dpNow = { DotXZ(dXZ, prism->u), DotXZ(dXZ, prism->v) };
				if (fabsf(dpNow.x) + fabsf(dpNow.y) < 1e-7f) break;

				bool changed = false;
				for (int i = 0; i < (int)poly.size(); ++i)
				{
					const XMFLOAT2 a = poly[i];
					const XMFLOAT2 b = poly[(i + 1) % (int)poly.size()];
					const XMFLOAT2 e = { b.x - a.x, b.y - a.y };

					XMFLOAT2 nIn = { -e.y, e.x };
					float nl = Len2(nIn);
					if (nl < 1e-6f) continue;
					nIn.x /= nl; nIn.y /= nl;

					const XMFLOAT3 axisW = prism->u * nIn.x + prism->v * nIn.y;
					const float rEdge = ProjectRadiusOnAxis_Yaw(axisW, half, player->Rotation.y) + skin;

					const float s0 = Dot2(nIn, Sub2(p0, a));
					const float slack = s0 + rEdge;

					if (slack > 0.03f) continue;

					const float vIn = Dot2(nIn, dpNow);
					if (vIn <= 0.0f) continue;

					XMFLOAT3 axisXZ = { axisW.x, 0.0f, axisW.z };
					const float len2xz = axisXZ.x * axisXZ.x + axisXZ.z * axisXZ.z;
					if (len2xz < 1e-6f) continue;

					const float proj = (dXZ.x * axisXZ.x + dXZ.z * axisXZ.z) / len2xz;
					if (proj > 0.0f)
					{
						dXZ.x -= axisXZ.x * proj;
						dXZ.z -= axisXZ.z * proj;
						changed = true;
						hitAny = true;
					}
				}
				if (!changed) break;
			}
		}
	}

	player->Velocity.x = dXZ.x;
	player->Velocity.z = dXZ.z;

	return hitAny;
}

bool Player2DShadow_TopContact()
{
	PLAYER* player = GetPlayer2D();
	if (!player) return false;

	const std::vector<const ShadowPrism*>& prisms = Collision_GetShadowPrisms();
	if (prisms.empty()) return false;

	XMFLOAT3 halfSize = Player2D_GetSolidHalfSize();

	bool hitAny = false;
	const int maxIterations = 4;

	static int s_LastStandingPrismIndex = -1;
	static XMFLOAT3 s_LastShadowTopPos = { 0, 0, 0 };

	int currentStandingPrism = -1;
	float bestContactDist = FLT_MAX;

	for (int iter = 0; iter < maxIterations; iter++)
	{
		bool hitThisIter = false;
		float footY = player->Position.y;

		XMFLOAT3 playerCenter = XMFLOAT3(
			player->Position.x,
			player->Position.y + halfSize.y,
			player->Position.z
		);

		for (int prismIdx = 0; prismIdx < (int)prisms.size(); ++prismIdx)
		{
			const ShadowPrism* prism = prisms[prismIdx];
			if (!prism || !prism->isValid) continue;
			if (prism->poly.size() < 3) continue;

			float verticalMargin = halfSize.y + 0.5f;
			if (footY > prism->aabbMax.y + verticalMargin ||
				footY + halfSize.y * 2.0f < prism->aabbMin.y - verticalMargin)
			{
				continue;
			}

			float shadowTopY = prism->aabbMax.y;

			float horizontalMargin = halfSize.x + 0.15f;
			bool inXZRange = (playerCenter.x >= prism->aabbMin.x - horizontalMargin &&
				playerCenter.x <= prism->aabbMax.x + horizontalMargin &&
				playerCenter.z >= prism->aabbMin.z - halfSize.z &&
				playerCenter.z <= prism->aabbMax.z + halfSize.z);

			if (!inXZRange) continue;

			float footToShadowTop = footY - shadowTopY;

			float contactToleranceDown = -halfSize.y - 0.3f;
			float contactToleranceUp = 0.25f;

			if (footToShadowTop >= contactToleranceDown && footToShadowTop <= contactToleranceUp)
			{
				if (player->Velocity.y <= 0.05f) 
				{
					float targetY = shadowTopY;
					float currentY = player->Position.y;
					float deltaY = targetY - currentY;

					const float maxAdjustPerFrame = 0.15f;
					if (fabsf(deltaY) > maxAdjustPerFrame)
					{
						deltaY = (deltaY > 0) ? maxAdjustPerFrame : -maxAdjustPerFrame;
					}

					player->Position.y += deltaY;
					player->Velocity.y = 0.0f;
					player->isGround = true;

					float dist = fabsf(footToShadowTop);
					if (dist < bestContactDist)
					{
						bestContactDist = dist;
						currentStandingPrism = prismIdx;
					}

					hitThisIter = true;
					hitAny = true;
					continue;
				}
			}

			XMFLOAT3 rel = playerCenter - prism->origin;
			float localV = Dot(rel, prism->v);

			float minV = FLT_MAX, maxV = -FLT_MAX;
			for (const auto& p : prism->poly)
			{
				minV = (std::min)(minV, p.y);
				maxV = (std::max)(maxV, p.y);
			}

			float playerRadiusV = halfSize.y;
			float penV_neg = localV - (minV - playerRadiusV);

			if (penV_neg <= 0.001f) continue;

			float targetLocalV = minV - playerRadiusV;
			float deltaV = targetLocalV - localV;

			float pushAmount = prism->v.y * deltaV;
			const float maxPush = 0.1f;
			if (fabsf(pushAmount) > maxPush)
			{
				pushAmount = (pushAmount > 0) ? maxPush : -maxPush;
			}

			player->Position.y += pushAmount;

			if (player->Velocity.y > 0)
			{
				player->Velocity.y = 0.0f;
			}

			hitThisIter = true;
			hitAny = true;
		}

		if (!hitThisIter) break;
	}

	static int s_GraceFrames = 0;
	const int GRACE_FRAME_COUNT = 5;

	if (s_LastStandingPrismIndex >= 0 && !hitAny)
	{
		s_GraceFrames++;
		if (s_GraceFrames <= GRACE_FRAME_COUNT)
		{
			if (s_LastStandingPrismIndex < (int)prisms.size())
			{
				const ShadowPrism* lastPrism = prisms[s_LastStandingPrismIndex];
				if (lastPrism && lastPrism->isValid)
				{
					float newShadowTopY = lastPrism->aabbMax.y;
					float deltaY = newShadowTopY - s_LastShadowTopPos.y;

					player->Position.y += deltaY;
					player->isGround = true;
					player->Velocity.y = 0.0f;
					hitAny = true;

					s_LastShadowTopPos.y = newShadowTopY;
				}
			}
		}
		else
		{
			s_LastStandingPrismIndex = -1;
			s_GraceFrames = 0;
		}
	}
	else if (hitAny)
	{
		s_GraceFrames = 0;
	}

	if (currentStandingPrism >= 0)
	{
		s_LastStandingPrismIndex = currentStandingPrism;
		s_LastShadowTopPos.y = prisms[currentStandingPrism]->aabbMax.y;
	}

	return hitAny;
}


// �f�o�b�O�`��
void Collision_DebugDraw()
{
	// 3D�v���C���[�R���C�_�[
	PLAYER* player3D = GetPlayer3D();
	if (player3D)
	{
		XMFLOAT3 pC = GetPlayerSolidCollider();
		XMFLOAT3 pH = Player3D_GetSolidHalfSize();
		DebugDrawEllipsoid(pC, pH, IM_COL32(0, 255, 0, 255));

		XMFLOAT3 tC = GetPlayerTriggerCollider();
		XMFLOAT3 tH = Player3D_GetTriggerHalfSize();
		DebugDrawOBB_Yaw(tC, tH, player3D->Rotation.y, IM_COL32(255, 255, 255, 255));
	}
	// 2D�v���C���[�R���C�_�[
	PLAYER* player2D = GetPlayer2D();
	if (player2D)
	{
		XMFLOAT3 pC = GetPlayer2DSolidCollider();
		XMFLOAT3 pH = Player2D_GetSolidHalfSize();
		DebugDrawOBB_Yaw(pC, pH, player2D->Rotation.y, IM_COL32(0, 255, 0, 255));
	}

	// �t�B�[���h�g���K�[
	std::vector<MAPDATA>& map = GetFieldMap();
	if (player3D && !map.empty())
	{
		XMFLOAT3 tC = GetPlayerTriggerCollider();
		XMFLOAT3 tH = Player3D_GetTriggerHalfSize();

		for (size_t i = 0; i < map.size(); ++i)
		{
			if (!Field_IsTrigger(map[i].no)) continue;
			{
				XMFLOAT3 boxH = Field_GetHalfSize(map[i]);
				bool hit = OBB_Intersect_Yaw(tC, tH, player3D->Rotation.y,
					map[i].pos, boxH, map[i].rotate.y);
				ImU32 col = hit ? IM_COL32(255, 0, 0, 255) : IM_COL32(0, 255, 255, 255);
				DebugDrawOBB_Yaw(map[i].pos, boxH, map[i].rotate.y, col);
			}
		}
	}

	if (!g_ShadowPrisms.empty())
	{
		static const ImU32 prismColors[] = {
			IM_COL32(255, 50, 50, 220), 
			IM_COL32(50, 255, 50, 220), 
			IM_COL32(50, 50, 255, 220), 
			IM_COL32(255, 255, 50, 220),
			IM_COL32(255, 50, 255, 220),
			IM_COL32(50, 255, 255, 220),
		};
		const int colorCount = sizeof(prismColors) / sizeof(prismColors[0]);

		for (size_t i = 0; i < g_ShadowPrisms.size(); ++i)
		{
			if (g_ShadowPrisms[i] && g_ShadowPrisms[i]->isValid)
			{
				ShadowDebugOptions opts = g_ShadowDebugOpts;
				opts.prismColor = prismColors[i % colorCount];
				DebugDrawShadowPrism(*g_ShadowPrisms[i], opts);
			}
		}
	}

	for (const auto& box : g_ExtraBoxes)
	{
		if (box.isOBB)
			DebugDrawOBB_Yaw(box.center, box.half, box.rotDeg.y, box.color);
		else
			DebugDrawAABB(box.center, box.half, box.color);
	}

}
