#include "camera.h"
#include"keyboard.h"
#include "mouse.h"
#include "player3D.h"
#include "debug.h"


//=========================================================================================================
// ?O???[?o?????
//=========================================================================================================
static CAMERA CameraObject;
XMFLOAT3 g_PlayerPosOld;
static bool debugMode = TRUE;


Mouse_State ms{};
float cSize = 1.0f;

static bool   gCamAnglesInit = false;
static float  gYawDeg = 0.0f;
static float  gPitchDeg = 0.0f;
static const float kPitchMin = -85.0f;
static const float kPitchMax = 85.0f;

//=========================================================================================================
// ??????????
//=========================================================================================================
void Camera_Initialize()
{
	CameraObject.Position = XMFLOAT3(0.0f, 5.0f, -5.0f);
	CameraObject.AtPosition = XMFLOAT3(0.0f, 0.0f, 0.0f);
	CameraObject.UpVector = XMFLOAT3(0.0f, 1.0f, 0.0f);

	CameraObject.Fov = 45.0f;
	float width = (float)Direct3D_GetBackBufferWidth();
	float height = (float)Direct3D_GetBackBufferHeight();
	CameraObject.Aspect = width / height;
	CameraObject.NearClip = 0.5f;
	CameraObject.FarClip = 1000.0f;

	g_PlayerPosOld = GetPlayer3DPositon();
}

//=========================================================================================================
// ?I??????
//=========================================================================================================
void Camera_Finalize()
{
	return;
}

//=========================================================================================================
// ?X?V????
//=========================================================================================================
void Camera_Update()
{
	Mose();
}

//=========================================================================================================
// ?`????
//=========================================================================================================
void Camera_Draw()
{
	if (debugMode)
	{
		ImGui::Begin("Debug - CHEN");
		if (ImGui::TreeNode("canera.cpp"))
		{
			ImGui::Text("PosX: %.2f", CameraObject.Position.x);
			ImGui::Text("PosY: %.2f", CameraObject.Position.y);
			ImGui::Text("PosZ: %.2f", CameraObject.Position.z);
			ImGui::TreePop();
		}
		ImGui::End();
	}

	//?v???W?F?N?V?????s???
	CameraObject.Projection = XMMatrixPerspectiveFovLH(XMConvertToRadians(CameraObject.Fov),CameraObject.Aspect,CameraObject.NearClip,CameraObject.FarClip);

	//?r???[?s???
	XMVECTOR vPos = XMVectorSet(CameraObject.Position.x,CameraObject.Position.y,CameraObject.Position.z,0.0f);
	XMVECTOR vAt = XMVectorSet(CameraObject.AtPosition.x, CameraObject.AtPosition.y, CameraObject.AtPosition.z, 0.0f);
	XMVECTOR vUp = XMVectorSet(CameraObject.UpVector.x, CameraObject.UpVector.y, CameraObject.UpVector.z, 0.0f);
	CameraObject.View = XMMatrixLookAtLH(vPos, vAt, vUp);

	return;
}

//=========================================================================================================
// ????p????
//=========================================================================================================
void SetCameraFov(float fov)
{
	CameraObject.Fov = fov;
}

//=========================================================================================================
// ?A?X?y?N?g?????
//=========================================================================================================
void SetCameraAspect(float asp)
{
	CameraObject.Aspect = asp;
}

//=========================================================================================================
// ?N???b?v????????
//=========================================================================================================
void SetCameraClip(float n, float f)
{
	CameraObject.NearClip = n;
	CameraObject.FarClip = f;
}

//=========================================================================================================
// ??u????
//=========================================================================================================
void SetCameraPosition(XMFLOAT3 pos)
{
	CameraObject.Position = pos;
}

//=========================================================================================================
// ?????_????
//=========================================================================================================
void SetCameraAtPosition(XMFLOAT3 atpos )
{
	CameraObject.AtPosition = atpos;
}

//=========================================================================================================
// ??????x?N?g??????
//=========================================================================================================
void SetCameraUpVector(XMFLOAT3 up)
{
	CameraObject.UpVector = up;
}

//=========================================================================================================
// ?r???[?s??èÔ
//=========================================================================================================
XMMATRIX GetViewMatrix()
{
	return CameraObject.View;
}

//=========================================================================================================
// ?v???W?F?N?V?????s???èÔ
//=========================================================================================================
XMMATRIX GetProjectionMatrix()
{
	return CameraObject.Projection;
}

//=========================================================================================================
// ?J??????????_???èÔ
//=========================================================================================================
XMFLOAT3 GetCameraAtPosition()
{
	return CameraObject.AtPosition;
}

//=========================================================================================================
// ?J???????u???èÔ
//=========================================================================================================
XMFLOAT3 GetCameraPosition()
{
	return CameraObject.Position;
}

void Mose()
{
        Mouse_GetState(&ms);

        const float sensitivityYaw = 0.5f;   // x  ?}?E?X???x?????p
        const float sensitivityPitch = 0.5f; // y  ?}?E?X???x?????p
        const float moveSpeedBase = 0.1f;

        // ????????[?h?????
        Mouse_SetMode(MOUSE_POSITION_MODE_RELATIVE);
       
        // FOV????
        if (Keyboard_IsKeyDown(KK_Q)) {
            CameraObject.Fov += 0.5f;
            if (CameraObject.Fov > 160.0f) CameraObject.Fov = 160.0f;
        }
        if (Keyboard_IsKeyDown(KK_E)) {
            CameraObject.Fov -= 0.5f;
            if (CameraObject.Fov < 5.0f) CameraObject.Fov = 5.0f;
        }

        // ?v???C???[??????èÔ
        XMFLOAT3 playerDelta = g_PlayerPosOld;
        g_PlayerPosOld = GetPlayer3DPositon();
        playerDelta.x = g_PlayerPosOld.x - playerDelta.x;
        playerDelta.y = g_PlayerPosOld.y - playerDelta.y;
        playerDelta.z = g_PlayerPosOld.z - playerDelta.z;

        XMFLOAT3 pos = CameraObject.Position;
        XMFLOAT3 at = CameraObject.AtPosition;

        // ?????p?x?v?Z
        if (!gCamAnglesInit) {
            float rx = pos.x - at.x;
            float ry = pos.y - at.y;
            float rz = pos.z - at.z;
            float r = sqrtf(rx * rx + ry * ry + rz * rz);
            if (r < 1e-6f) r = 1e-6f;

            gYawDeg = XMConvertToDegrees(atan2f(rz, rx));
            gPitchDeg = XMConvertToDegrees(asinf(ry / r));
            XMVECTOR v = XMVectorSet(gPitchDeg, 0, 0, 0);
            XMVECTOR lo = XMVectorReplicate(kPitchMin);
            XMVECTOR hi = XMVectorReplicate(kPitchMax);
            v = XMVectorClamp(v, lo, hi);
            gPitchDeg = XMVectorGetX(v);

            gCamAnglesInit = true;
        }

        // ?}?E?X?????]
        if (ms.positionMode == MOUSE_POSITION_MODE_RELATIVE) {
            gYawDeg -= ms.x * sensitivityYaw;
            gPitchDeg += ms.y * sensitivityPitch;
            XMVECTOR v = XMVectorSet(gPitchDeg, 0, 0, 0);
            XMVECTOR lo = XMVectorReplicate(kPitchMin);
            XMVECTOR hi = XMVectorReplicate(kPitchMax);
            v = XMVectorClamp(v, lo, hi);
            gPitchDeg = XMVectorGetX(v);
        }

        // ?J???????a???u?v?Z
        float relX = pos.x - at.x;
        float relY = pos.y - at.y;
        float relZ = pos.z - at.z;
        float radius = sqrtf(relX * relX + relY * relY + relZ * relZ);
        if (radius < 1e-6f) radius = 1e-6f;

        float yawRad = XMConvertToRadians(gYawDeg);
        float pitchRad = XMConvertToRadians(gPitchDeg);

        float cp = cosf(pitchRad);
        float sp = sinf(pitchRad);
        float cy = cosf(yawRad);
        float sy = sinf(yawRad);

        float rx = radius * cp * cy;
        float ry = radius * sp;
        float rz = radius * cp * sy;

        pos.x = at.x + rx;
        pos.y = at.y + ry;
        pos.z = at.z + rz;

        // ???K???????????x?N?g??
        XMFLOAT3 fwd = { at.x - pos.x, at.y - pos.y, at.z - pos.z };
        float flen = sqrtf(fwd.x * fwd.x + fwd.z * fwd.z);
        if (flen > 1e-6f) {
            fwd.x /= flen;
            fwd.y /= flen;
            fwd.z /= flen;
        }
        else {
            fwd = { 0.0f, 0.0f, 1.0f };
        }
        XMFLOAT3 right = { fwd.z, 0.0f, -fwd.x };
        const XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };

        // ?v???C???[????????J????????f
        pos.x += playerDelta.x;
        pos.y += playerDelta.y;
        pos.z += playerDelta.z;
        at.x += playerDelta.x;
        at.y += playerDelta.y;
        at.z += playerDelta.z;

        CameraObject.Position = pos;
        CameraObject.AtPosition = at;
    }

void Keyb()
{
	//?v???C???[????W???èÔ
	XMFLOAT3 pos = g_PlayerPosOld;
	g_PlayerPosOld = GetPlayer3DPositon();

	//?O???v???C???[??????v???C???[????W?????
	pos.x = g_PlayerPosOld.x - pos.x;
	pos.y = g_PlayerPosOld.y - pos.y;
	pos.z = g_PlayerPosOld.z - pos.z;
	//?J?????????
	CameraObject.Position.x += pos.x;
	CameraObject.Position.y += pos.y;
	CameraObject.Position.z += pos.z;

	//?{?[??????W????_?????Z?b?g
	CameraObject.AtPosition.x = g_PlayerPosOld.x;
	CameraObject.AtPosition.y = g_PlayerPosOld.y;
	CameraObject.AtPosition.z = g_PlayerPosOld.z;

	//?????_??S??J???????](Y????])
	float Rotation = 0.0f;

	//?????_??S??J???????]
	XMFLOAT2 vec;
	vec.x = CameraObject.Position.x - CameraObject.AtPosition.x;
	vec.y = CameraObject.Position.z - CameraObject.AtPosition.z;
	//?????_????J???????x?N?g??
	float co = cosf(XMConvertToRadians(Rotation));
	float si = sinf(XMConvertToRadians(Rotation));
	CameraObject.Position.x = (vec.x * co - vec.y * si);
	CameraObject.Position.z = (vec.x * si + vec.y * co);
	CameraObject.Position.x += CameraObject.AtPosition.x;
	CameraObject.Position.z += CameraObject.AtPosition.z;

}
