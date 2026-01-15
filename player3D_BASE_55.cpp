#include "Player3D.h"
#include "Controller.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "manager.h"
#include "Input.h"

#include "debug.h"


PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
static float g_StopTime = 0.0f;


extern Controller gPad;


static XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

static XMFLOAT3		Firstposition;
static XMFLOAT3		FirstRotation;
static XMFLOAT3		FirstScaling;
static XMFLOAT3		FirstVelocity;
static XMFLOAT3		FirstAcceleration;
static PLAYER3D_STATE	FirstState;
static float			FirstStopTime;
static XMVECTOR		FirstQuaternion;


static float moveSpeed = 0.005f;		
static float maxMoveSpeed = 1.0f;		
static float maxFallSpeed = -0.5f;		
static float  dampingXZ = 0.925f;		
//static float gravityPower = 1.0f;		
static float jumpPower = 0.175f;		


float FirstMaxMoveSpeed = maxMoveSpeed;


static const auto UpKey = KK_W;			
static const auto RightKey = KK_D;		
static const auto DownKey = KK_S;		
static const auto LeftKey = KK_A;		

static const auto JumpKey = KK_SPACE;	
static const auto ActionKey = KK_F;		
static const auto ChangeKey = KK_F;		

static const auto ResetKey = KK_R;		
static const auto MenuKey = KK_ESCAPE;	

static bool debugMode = true;
static bool isTrigger = false;


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
	FirstState = g_Player3D.state = PLAYER3D_STATE_MOVE;
	FirstStopTime = g_StopTime = 0.0f;
	FirstQuaternion = g_Player3D.Quaternion = XMQuaternionIdentity();

	FirstMaxMoveSpeed = maxMoveSpeed;

}


void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}


void Player3D_Update()
{
	if (!g_Player3DActive) return;

	Player3D_Respawn();

	Player3D_Gravity();


	Player3D_Move();	
	Player3D_Jump();	
	Player3D_Change();	
	Player3D_Action();	

	switch (g_Player3D.state)
	{
	case PLAYER3D_STATE_IDLE:
		//Idleアニメーション
		break;
	case PLAYER3D_STATE_MOVE:
		//Moveアニメーション
		break;
	case PLAYER3D_STATE_FALL:
		//Fallアニメーション
		break;
	case PLAYER3D_STATE_ACTION:
		//Actionアニメーション
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

	if (g_Player3D.Velocity.z >= maxMoveSpeed) g_Player3D.Velocity.z = maxMoveSpeed;
	else g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *= 0.925f;
	g_Player3D.Velocity.z *= 0.925f;

	if (!g_Player3D.isGround)
	{
		//if (g_Player3D.Velocity.y 
		// < maxGravity) g_Player3D.Velocity.y = maxGravity;
		//else g_Player3D.Velocity.y += g_Player3D.Acceleration.y;
	}
	else
	{
		if (g_Player3D.Velocity.y < 0.0f)
			g_Player3D.Velocity.y = 0.0f;
	}


	g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *=  dampingXZ;
	g_Player3D.Velocity.z *=  dampingXZ;

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
	if (Keyboard_IsKeyDownTrigger(ResetKey))
	{
		Player3D_Reset();
	}
}

void Player3D_Move()
{


	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (!gPad.IsConnected())
	{
		if (Keyboard_IsKeyDown(UpKey))    inputDir.z += +1.0f;
		if (Keyboard_IsKeyDown(DownKey))  inputDir.z += -1.0f;
		if (Keyboard_IsKeyDown(RightKey)) inputDir.x += +1.0f;
		if (Keyboard_IsKeyDown(LeftKey))  inputDir.x += -1.0f;
	}
	else
	{
		float lx = gPad.GetLeftStickX();
		float ly = gPad.GetLeftStickY();


		const float deadzone = 0.20f;
		if (fabsf(lx) < deadzone) lx = 0.0f;
		if (fabsf(ly) < deadzone) ly = 0.0f;

		// 左右が逆になる問題を修正（スティック右で x が正になるよう符号を反転）
		inputDir.x = -lx;      // 左右
		inputDir.y = 0.0f;     // 3Dなら Y は高さとして使わない
		inputDir.z = -ly;      // 前後
	}


	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 1e-6f)
	{

		inputDir.x /= len;
		inputDir.z /= len;


		XMFLOAT3 camPos = GetCameraPosition();
		XMFLOAT3 camAt = GetCameraAtPosition();
		XMFLOAT3 camFwd = XMFLOAT3(
			camAt.x - camPos.x,
			0.0f,
			camAt.z - camPos.z
		);
		float flen = sqrtf(camFwd.x * camFwd.x + camFwd.z * camFwd.z);
		if (flen > 1e-6f)
		{
			camFwd.x /= flen;
			camFwd.z /= flen;
		}
		else
		{
			camFwd = XMFLOAT3(0.0f, 0.0f, 1.0f);
		}


		XMFLOAT3 camRight = XMFLOAT3(
			camFwd.z,
			0.0f,
			-camFwd.x
		);
		float rlen = sqrtf(camRight.x * camRight.x + camRight.z * camRight.z);
		if (rlen > 1e-6f)
		{
			camRight.x /= rlen;
			camRight.z /= rlen;
		}


		XMFLOAT3 moveWorld = XMFLOAT3(
			camFwd.x * inputDir.z + camRight.x * inputDir.x,
			0.0f,
			camFwd.z * inputDir.z + camRight.z * inputDir.x
		);
		float mlen = sqrtf(moveWorld.x * moveWorld.x + moveWorld.z * moveWorld.z);
		if (mlen > 1e-6f)
		{
			moveWorld.x /= mlen;
			moveWorld.z /= mlen;
		}

		g_Player3D.Velocity.x += moveWorld.x * moveSpeed;
		g_Player3D.Velocity.z += moveWorld.z * moveSpeed;


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
		g_Player3D.Velocity.x = maxMoveSpeed;
	}
	else if (g_Player3D.Velocity.x <= -maxMoveSpeed)
	{
		g_Player3D.Velocity.x = -maxMoveSpeed;
	}
	if (g_Player3D.Velocity.z >= maxMoveSpeed)
	{
		g_Player3D.Velocity.z = maxMoveSpeed;
	}
	else if (g_Player3D.Velocity.z <= -maxMoveSpeed)
	{
		g_Player3D.Velocity.z = -maxMoveSpeed;
	}
}

void Player3D_Jump()
{
	if (Keyboard_IsKeyDownTrigger(JumpKey) || gPad.IsButtonPressed(JumpKey))
	{
		if (g_Player3D.isGround)
		{
			g_Player3D.Velocity.y = jumpPower;
			g_Player3D.isGround = false;
			g_Player3D.state = PLAYER3D_STATE_FALL;
		}
	}
}

void Player3D_Change()
{
	if (Keyboard_IsKeyDownTrigger(ChangeKey))
	{
		// ◆影に変身
		// 
		// 壁に近づいたときに反応
		// ↓
		// 2Dキャラクターを3Dプレイヤーの座標を参照して生成
		// ↓
		// 3Dプレイヤーを削除する
		// 
	}
}

void Player3D_Action()
{
	if (Keyboard_IsKeyDownTrigger(ActionKey))
	{
		// ◆箱を持つ
		//
		// アニメーション
		// ↓
		// 箱をプレイヤーの座標に追従させる
		// ↓
		// 箱持ち状態をtrueにする
		// 
		// 
		// ◆箱を離す
		// 
		// アニメーション
		// ↓
		// 箱をプレイヤーに追従させるのをやめる
		// ↓
		// 箱持ち状態をfalseにする
		// 
		// 
		// ◆照明操作を始める
		// 
		// アニメーション
		// ↓
		// 照明操作状態をtrueにする
		// ↓
		// 移動入力を受け付けなくする
		// 
		//  
		// ◆照明操作をやめる
		// 
		// アニメーション
		// ↓
		// 照明操作状態をfalseにする
		// ↓
		// 移動入力を受け付ける
		// 
		// 

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
	g_Player3D.Position = Firstposition;
	g_Player3D.Rotation = FirstRotation;
	g_Player3D.Scaling = FirstScaling;
	g_Player3D.Velocity = FirstVelocity;
	g_Player3D.Acceleration = FirstAcceleration;
	g_Player3D.state = FirstState;
	g_StopTime = FirstStopTime;
	g_Player3D.Quaternion = FirstQuaternion;

	maxMoveSpeed = FirstMaxMoveSpeed;
}

PLAYER3D* GetPlayer3D()
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

	g_Player3D.state = PLAYER3D_STATE_MOVE;
	g_Player3D.isGround = false;
	g_StopTime = 0.0f;
}

void Player3D_SetActive(bool active)
{
	g_Player3DActive = active;
}
