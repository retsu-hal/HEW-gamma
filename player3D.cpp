#include "Player3D.h"
#include "PlayerStatus.h"
#include "Keyboard.h"
#include "KeyBind.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "manager.h"
#include "Input.h"

#include "debug.h"
#include "MathUtil.h"
using namespace mu;


// デバッグ用
#include "debug.h"
static bool debugMode = TRUE;


PLAYER g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
static float g_StopTime = 0.0f;


extern Controller gPad;


static XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);


static XMFLOAT3			Firstposition;
static XMFLOAT3			FirstRotation;
static XMFLOAT3			FirstScaling;
static XMFLOAT3			FirstVelocity;
static XMFLOAT3			FirstAcceleration;
static PLAYER_STATE	FirstState;
static float			FirstStopTime;
static XMVECTOR			FirstQuaternion;


static float moveSpeed = 0.005f;	
static float maxMoveSpeed = 1.0f;	
static float maxFallSpeed = -0.5f;	
static float  dampingXZ = 0.925f;	
static float gravityPower = -1.0f;	
static float jumpPower = 0.175f;	

float FirstMaxMoveSpeed = maxMoveSpeed;

static XMFLOAT3 g_SolidHalfSize = XMFLOAT3(
	PLAYER3D_SOLID_HALF_X,
	PLAYER3D_SOLID_HALF_Y,
	PLAYER3D_SOLID_HALF_Z
);
static XMFLOAT3 g_TriggerHalfSize = XMFLOAT3(
	PLAYER3D_TRIGGER_HALF_X,
	PLAYER3D_TRIGGER_HALF_Y,
	PLAYER3D_TRIGGER_HALF_Z
);

static bool g_Player3DActive = true;

static bool isTrigger = false;


void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	g_pDevice = pDevice;
	g_pContext = pContext;
	
	g_Player3D.Model = ModelLoad("asset\\model\\Hip_Hop_Dancing.fbx");

	Firstposition = g_Player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	FirstRotation = g_Player3D.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstScaling = g_Player3D.Scaling = XMFLOAT3(0.01f, 0.01f, 0.01f);
	FirstVelocity = g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstAcceleration = g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	FirstState = g_Player3D.state = PLAYER_STATE_MOVE;
	FirstStopTime = g_StopTime = 0.0f;
	FirstQuaternion = g_Player3D.Quaternion = XMQuaternionIdentity();

	FirstMaxMoveSpeed = maxMoveSpeed;

	g_Player3D.FirstMaxMoveSpeed = g_Player3D.maxMoveSpeed;	//初期最大移動速度
}


void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}


void Player3D_Update()
{
	if (!g_Player3DActive) return;

	Player3D_Respawn();	//リスポーン

	Player3D_Gravity();	//重力処理

	// プレイヤー操作
	Player3D_Move();	//移動
	Player3D_Jump();	//ジャンプ
	Player3D_Change();	//影変身
	Player3D_Action();	//アクション

	switch (g_Player3D.state)
	{
	case PLAYER_STATE_IDLE:

		break;
	case PLAYER_STATE_MOVE:

		break;
	case PLAYER_STATE_FALL:

		break;
	case PLAYER_STATE_ACTION:

		break;
	default:
		break;
	}

}


XMFLOAT3 GetPlayer3DPosition()
{
	return g_Player3D.Position;
}


void Player3D_Gravity()
{

	if (g_Player3D.Velocity.x >= maxMoveSpeed) g_Player3D.Velocity.x = maxMoveSpeed;
	else g_Player3D.Velocity.x += g_Player3D.Acceleration.x;

	if (g_Player3D.Velocity.z >= g_Player3D.maxMoveSpeed) g_Player3D.Velocity.z = g_Player3D.maxMoveSpeed;
	else g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *= 0.925f;
	g_Player3D.Velocity.z *= 0.925f;

	if (!g_Player3D.isGround)
	{
		// 重力加速度を適用し、最大落下速度でクランプする（maxFallSpeed は負の値の想定）
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y;

		if (g_Player3D.Velocity.y < g_Player3D.maxFallSpeed)
			g_Player3D.Velocity.y = g_Player3D.maxFallSpeed;
	}
	else
	{
		// 地上では下向き速度をゼロにする
		if (g_Player3D.Velocity.y < 0.0f)
			g_Player3D.Velocity.y = 0.0f;
	}


	g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *= g_Player3D.dampingXZ;
	g_Player3D.Velocity.z *= g_Player3D.dampingXZ;

	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;

	int hit = Player3DField_Collision();
}


void Player3D_Respawn()
{

	if (g_Player3D.Position.y < -10.0f)
	{
		Player3D_Reset();
		return;
	}
	if (IsInputDown(ResetKey, gPad))
	{
		Player3D_Reset();
	}
}

void Player3D_Move()
{
	// 入力ベクトル
	XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);


	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (!gPad.IsConnected())
	{
		if (IsInputDown(UpKey, gPad))    inputDir.z += +1.0f;
		if (IsInputDown(DownKey, gPad))  inputDir.z += -1.0f;
		if (IsInputDown(RightKey, gPad)) inputDir.x += +1.0f;
		if (IsInputDown(LeftKey, gPad))  inputDir.x += -1.0f;
	}
	else
	{
		float lx = gPad.GetLeftStickX();
		float ly = gPad.GetLeftStickY();


		const float deadzone = 0.20f;
		if (fabsf(lx) < deadzone) lx = 0.0f;
		if (fabsf(ly) < deadzone) ly = 0.0f;


		inputDir.x = -lx;  
		inputDir.y = 0.0f; 
		inputDir.z = -ly;  
	}


	float len = Length2D(inputDir);
	if (len > 1e-6f)
	{

		inputDir = Normalize2D(inputDir);


		XMFLOAT3 camPos = GetCameraPosition();
		XMFLOAT3 camAt = GetCameraAtPosition();

		XMFLOAT3 camFwd = XMFLOAT3(
			camAt.x - camPos.x,
			0.0f,
			camAt.z - camPos.z
		);
		camFwd = Normalize2D(camFwd);
		XMFLOAT3 camRight = XMFLOAT3(
			camFwd.z,
			0.0f,
			-camFwd.x
		);
		XMFLOAT3 moveWorld = XMFLOAT3(
			camFwd.x * inputDir.z + camRight.x * inputDir.x,
			0.0f,
			camFwd.z * inputDir.z + camRight.z * inputDir.x
		);
		moveWorld = Normalize2D(moveWorld);

		g_Player3D.Velocity.x += moveWorld.x * g_Player3D.moveSpeed;
		g_Player3D.Velocity.z += moveWorld.z * g_Player3D.moveSpeed;


		float targetYawRad = atan2f(moveWorld.x, moveWorld.z);
		float targetYawDeg = XMConvertToDegrees(targetYawRad);


		const float yawOffset = FirstRotation.y;
		targetYawDeg += yawOffset;



		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f;
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}



	if (g_Player3D.Velocity.x >= maxMoveSpeed)
	{
		g_Player3D.Velocity.x = g_Player3D.maxMoveSpeed;
	}
	else if (g_Player3D.Velocity.x <= -g_Player3D.maxMoveSpeed)
	{
		g_Player3D.Velocity.x = -g_Player3D.maxMoveSpeed;
	}
	if (g_Player3D.Velocity.z >= g_Player3D.maxMoveSpeed)
	{
		g_Player3D.Velocity.z = g_Player3D.maxMoveSpeed;
	}
	else if (g_Player3D.Velocity.z <= -g_Player3D.maxMoveSpeed)
	{
		g_Player3D.Velocity.z = -g_Player3D.maxMoveSpeed;
	}
}


void Player3D_Jump()
{
	if (IsInputDown(JumpKey, gPad)) 
	{
		if (g_Player3D.isGround)
		{
			g_Player3D.Velocity.y = g_Player3D.jumpPower;
			g_Player3D.isGround = false;
			g_Player3D.state = PLAYER_STATE_FALL;
		}
	}
}

void Player3D_Change()
{
	if (IsInputDown(ChangeKey, gPad))
	{
	
	}
}

void Player3D_Action()
{
	if (IsInputDown(ActionKey, gPad))
	{
		

	}
	isTrigger = false;

	TRIGGER_HIT hit;
	if (!Collision_PlayerTrigger(&hit, 0.2f)) return;
	if (hit.side != TRIGGER_SIDE_FRONT) return;
	switch (hit.type)
	{
	case FIELD_GOAL:

		break;
	case FIELD_OBJ_1:
		isTrigger = true;
		break;
	case FIELD_OBJ_2:

		break;

	default:
		break;
	}
}

void Player3D_Reset()
{
	g_Player3D.Position = g_Player3D.Firstposition;
	g_Player3D.Rotation = g_Player3D.FirstRotation;
	g_Player3D.Scaling = g_Player3D.FirstScaling;
	g_Player3D.Velocity = g_Player3D.FirstVelocity;
	g_Player3D.Acceleration = g_Player3D.FirstAcceleration;
	g_Player3D.state = g_Player3D.FirstState;
	g_StopTime = g_Player3D.FirstStopTime;
	g_Player3D.Quaternion = g_Player3D.FirstQuaternion;

	g_Player3D.maxMoveSpeed = g_Player3D.FirstMaxMoveSpeed;
}

PLAYER* GetPlayer3D()
{
	return &g_Player3D;
}

XMFLOAT3 Player3D_GetSolidHalfSize()
{
	return g_SolidHalfSize;
}

XMFLOAT3 Player3D_GetTriggerHalfSize()
{
	return g_TriggerHalfSize;
}


void Player3D_Draw(void)
{

	if (!g_Player3DActive) return;

	if (debugMode)
	{
		/*ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("Player3D.cpp"))
		{
			ImGui::Text("Trigger: %s", isTrigger ? "true" : "false");
			ImGui::TreePop();
		}
		ImGui::End();*/
	}



	XMMATRIX scale = XMMatrixScaling
	(
		g_Player3D.Scaling.x,
		g_Player3D.Scaling.y,
		g_Player3D.Scaling.z);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw
	(
		XMConvertToRadians(g_Player3D.Rotation.x),
		XMConvertToRadians(g_Player3D.Rotation.y),
		XMConvertToRadians(g_Player3D.Rotation.z)

	);

	XMMATRIX translation = XMMatrixTranslation
	(
		g_Player3D.Position.x,
		g_Player3D.Position.y,
		g_Player3D.Position.z
	);

	XMMATRIX world = scale * rotation * translation;

	XMMATRIX view = GetViewMatrix();
	XMMATRIX projection = GetProjectionMatrix();
	XMMATRIX wvp = world * view * projection;


	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	ModelUpdateAnimation(g_Player3D.Model, 10.0f / 600.0f);   
	Shader_SetBones(g_Player3D.Model);	



	ModelDraw(g_Player3D.Model);
}

XMFLOAT3 Player3D_GetForward()
{
	const float pitch = XMConvertToRadians(g_Player3D.Rotation.x);
	const float yaw = XMConvertToRadians(g_Player3D.Rotation.y);
	const float roll = XMConvertToRadians(g_Player3D.Rotation.z);

	XMMATRIX R = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	XMVECTOR f = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), R);
	f = XMVector3Normalize(f);

	XMFLOAT3 out;
	XMStoreFloat3(&out, f);
	return out;
}

void Player3D_InitAt(const XMFLOAT3& pos, const XMFLOAT3& rot)
{
	g_Player3D.Position = pos;
	g_Player3D.Rotation = rot;

	g_Player3D.Velocity = XMFLOAT3(0, 0, 0);
	g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);

	g_Player3D.state = PLAYER_STATE_MOVE;
	g_Player3D.isGround = false;
	g_StopTime = 0.0f;
}

void Player3D_SetActive(bool active)
{
	g_Player3DActive = active;
}
