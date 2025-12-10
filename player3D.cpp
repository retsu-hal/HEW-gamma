#include "Player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"

#include "debug.h"


PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
float g_StopTime = 0.0f;


XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);


XMFLOAT3		Firstposition;
XMFLOAT3		FirstRotation;
XMFLOAT3		FirstScaling;
XMFLOAT3		FirstVelocity;
XMFLOAT3		FirstAcceleration;
PLAYER3D_STATE	FirstState;
float			FirstStopTime;
XMVECTOR		FirstQuaternion;

float moveSpeed = 0.005f;
float maxMoveSpeed = 1.0f;
float maxGravity = -0.25f;
float jumpPower = 0.175f;
bool isGround = false;

static const auto UpKey = KK_W;
static const auto RightKey = KK_D;
static const auto DownKey = KK_S;
static const auto LeftKey = KK_A;

static const auto JumpKey = KK_SPACE;
static const auto ActionKey = KK_F;	
static const auto ChangeKey = KK_F;

static const auto ResetKey = KK_R;
static const auto MenuKey = KK_ESCAPE;

static bool debugMode = TRUE;

static XMFLOAT3 g_DetectHalfSize = XMFLOAT3(
	PLAYER3D_DETECT_HALF_X,
	PLAYER3D_DETECT_HALF_Y,
	PLAYER3D_DETECT_HALF_Z
);


void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{

	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Player3D.Model = ModelLoad("asset\\model\\Test_man_stand.fbx");

	Firstposition = g_Player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	FirstRotation = g_Player3D.Rotation = XMFLOAT3(-90.0f, 180.0f, 0.0f);
	FirstScaling = g_Player3D.Scaling = XMFLOAT3(0.01f, 0.01f, 0.01f);
	FirstVelocity = g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstAcceleration = g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	FirstState = g_Player3D.state = PLAYER3D_STATE_MOVE;
	FirstStopTime = g_StopTime = 0.0f;
	FirstQuaternion = g_Player3D.Quaternion = XMQuaternionIdentity();

}

void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}

void Player3D_Update()
{
	Player3D_Respawn();

	Player3D_Move();
	Player3D_Jump();
	Player3D_Change();
	Player3D_Action();

	Player3D_Gravity();


	switch (g_Player3D.state)
	{
	case PLAYER3D_STATE_IDLE:

		break;

	case PLAYER3D_STATE_MOVE:

		break;

	case PLAYER3D_STATE_FALL:

		break;

	case PLAYER3D_STATE_ACTION:

		break;

	default:
		break;
	}
}

void Player3D_Draw(void)
{
	if (debugMode)
	{
		ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("Player3D.cpp"))
		{
			ImGui::Text("PosX: %.2f", g_Player3D.Position.x);
			ImGui::Text("PosY: %.2f", g_Player3D.Position.y);
			ImGui::Text("PosZ: %.2f", g_Player3D.Position.z);
			ImGui::TreePop();
		}
		ImGui::End();

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

	ModelDraw(g_Player3D.Model);
}

XMFLOAT3 GetPlayer3DPositon()
{
	return g_Player3D.Position;
}

void Player3D_Gravity()
{

	if (g_Player3D.Velocity.x >= maxMoveSpeed)
	{
		g_Player3D.Velocity.x = maxMoveSpeed;
	}
	else
	{
		g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	}

	if (g_Player3D.Velocity.y < maxGravity)
	{
		g_Player3D.Velocity.y = maxGravity;
	}
	else
	{
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y;
	}

	if (g_Player3D.Velocity.z >= maxMoveSpeed)
	{
		g_Player3D.Velocity.z = maxMoveSpeed;
	}
	else
	{
		g_Player3D.Velocity.z += g_Player3D.Acceleration.z;
	}


	g_Player3D.Velocity.x *= 0.925f;

	g_Player3D.Velocity.z *= 0.925f;


	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;




	float len = (g_Player3D.Velocity.x * g_Player3D.Velocity.x + g_Player3D.Velocity.y * g_Player3D.Velocity.y + g_Player3D.Velocity.z * g_Player3D.Velocity.z);
	if (len <= 0.0002f)
	{
		g_StopTime++;
		if (g_StopTime > (60.0f * 2))
		{
			g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
			g_Player3D.state = PLAYER3D_STATE_IDLE;
			g_StopTime = 0.0f;
		}
	}

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

	if (Keyboard_IsKeyDown(UpKey))    inputDir.z += +1.0f;
	if (Keyboard_IsKeyDown(DownKey))  inputDir.z += -1.0f;
	if (Keyboard_IsKeyDown(RightKey)) inputDir.x += +1.0f;
	if (Keyboard_IsKeyDown(LeftKey))  inputDir.x += -1.0f;


	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 0.0001f)
	{

		inputDir.x /= len;
		inputDir.z /= len;
		g_Player3D.Velocity.x += inputDir.x * moveSpeed;
		g_Player3D.Velocity.z += inputDir.z * moveSpeed;

		float targetYawRad = atan2f(inputDir.x, inputDir.z);
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

}

void Player3D_Jump()
{
	if (Keyboard_IsKeyDownTrigger(JumpKey))
	{
		g_Player3D.Velocity.y += jumpPower;
		if (isGround)
		{
			g_Player3D.Velocity.y += jumpPower;
		}
	}
}

void Player3D_Change()
{
	if (Keyboard_IsKeyDownTrigger(ChangeKey))
	{

	}
}

void Player3D_Action()
{
	if (Keyboard_IsKeyDownTrigger(ActionKey))
	{

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
}

PLAYER3D* GetPlayer3D()
{
	return &g_Player3D;
}

XMFLOAT3 Player3D_GetDetectHalfSize()
{
	return g_DetectHalfSize;
}