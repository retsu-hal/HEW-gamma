#include "Player3D.h"
#include "PlayerStatus.h"
#include "Keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "manager.h"
#include "Input.h"

#include "field.h"
#include "debug.h"
#include "MathUtil.h"
using namespace mu;

#include "Seesaw.h"
static bool debugMode = true;

//=========================================================================================================
// グローバル変数
//=========================================================================================================
PLAYER g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
static float g_StopTime = 0.0f;

// コントローラー
extern Controller gPad;
static XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);
float FirstMaxMoveSpeed = g_Player3D.maxMoveSpeed;
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

// 前方宣言: 状態管理のための追加関数
static bool Player3D_HasMoveInput();
static void Player3D_HandleStateTransitions();
static void Player3D_Idle();

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	for (int i = 0; i < PLAYER_ANIM_MAX; i++) {
		g_Player3D.Model[i] = NULL;
	}

	g_Player3D.FirstAnim = g_Player3D.CurrentAnimIndex = PLAYER_ANIM_IDLE;

	g_Player3D.Model[PLAYER_ANIM_IDLE] = ModelLoad("asset\\model\\Idle.fbx");
	g_Player3D.Model[PLAYER_ANIM_WALK] = ModelLoad("asset\\model\\Walking.fbx");
	g_Player3D.Model[PLAYER_ANIM_PUSH] = ModelLoad("asset\\model\\Pushing.fbx");

	g_Player3D.Firstposition = g_Player3D.Position;
	g_Player3D.FirstRotation = g_Player3D.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.FirstScaling = g_Player3D.Scaling = XMFLOAT3(0.01f, 0.01f, 0.01f);
	g_Player3D.FirstVelocity = g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.FirstAcceleration = g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	g_Player3D.FirstState = g_Player3D.state = PLAYER_STATE_MOVE;
	g_Player3D.FirstStopTime = g_StopTime = 0.0f;
	g_Player3D.FirstQuaternion = g_Player3D.Quaternion = XMQuaternionIdentity();

	FirstMaxMoveSpeed = g_Player3D.maxMoveSpeed;

	g_Player3D.FirstMaxMoveSpeed = g_Player3D.maxMoveSpeed;	//弶婜嵟戝堏摦懍搙
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Player3D_Finalize(void)
{
	for (int i = 0; i < PLAYER_ANIM_MAX; i++) {
		if (g_Player3D.Model[i] != NULL) {
			ModelRelease(g_Player3D.Model[i]);
			g_Player3D.Model[i] = NULL;
		}
	}
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void Player3D_Update()
{
	if (debugMode)
	{
		ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("Player3d.cpp"))
		{
			ImGui::Text("PosX: %.2f", g_Player3D.Position.x);
			ImGui::Text("PosY: %.2f", g_Player3D.Position.y);
			ImGui::Text("PosZ: %.2f", g_Player3D.Position.z);
			ImGui::TreePop();
		}
		ImGui::End();
	}

	if (!g_Player3DActive) return;
	if (g_Player3D.Position.y < -10.0f)
	{
		Player3D_Respawn();
	}
	

	Player3D_Gravity();

	// プレイヤー操作


	Player3D_Move();	//移動
	Player3D_Dash();	//ダッシュ
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

//=========================================================================================================
// 重力処理
//=========================================================================================================
void Player3D_Gravity()
{
	// 水平加速度・速度制御（このブロックは挙動次第で整理可能）
	if (g_Player3D.Velocity.x >= g_Player3D.maxMoveSpeed) g_Player3D.Velocity.x = g_Player3D.maxMoveSpeed;
	else g_Player3D.Velocity.x += g_Player3D.Acceleration.x;

	if (g_Player3D.Velocity.z >= g_Player3D.maxMoveSpeed) g_Player3D.Velocity.z = g_Player3D.maxMoveSpeed;
	else g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *= 0.925f;
	g_Player3D.Velocity.z *= 0.925f;

	// 垂直重力
	if (!g_Player3D.isGround)
	{
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y;

		if (g_Player3D.Velocity.y < g_Player3D.maxFallSpeed)
			g_Player3D.Velocity.y = g_Player3D.maxFallSpeed;
	}
	else
	{
		if (g_Player3D.Velocity.y < 0.0f)
			g_Player3D.Velocity.y = 0.0f;
	}

	// 再度水平に加速度と減衰を適用（必要なら整理）
	g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *= g_Player3D.dampingXZ;
	g_Player3D.Velocity.z *= g_Player3D.dampingXZ;

	// 位置更新
	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;

	int hit = Player3DField_Collision();

	Seesaw_PlayerCollision();
}

void Player3D_Respawn()
{
	g_Player3D.Position = g_Player3D.Firstposition;
	g_Player3D.Rotation = g_Player3D.FirstRotation;
	g_Player3D.Scaling = g_Player3D.FirstScaling;
	g_Player3D.Velocity = g_Player3D.FirstVelocity;
	g_Player3D.Acceleration = g_Player3D.FirstAcceleration;
	g_Player3D.state = g_Player3D.FirstState;
	g_Player3D.CurrentAnimIndex = g_Player3D.FirstAnim;
	g_StopTime = g_Player3D.FirstStopTime;
	g_Player3D.Quaternion = g_Player3D.FirstQuaternion;

	g_Player3D.maxMoveSpeed = g_Player3D.FirstMaxMoveSpeed;
	return;
}

void Player3D_Move()
{
	XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);
	bool isMoving = false;

	// 入力ベクトル
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (!gPad.IsConnected())
	{
		if (Keyboard_IsKeyDown(KK_W))
		{
			inputDir.z += +1.0f;
			isMoving = true;
		}
		if (Keyboard_IsKeyDown(KK_S))
		{
			inputDir.z += -1.0f;
			isMoving = true;
		}
		if (Keyboard_IsKeyDown(KK_D))
		{
			inputDir.x += +1.0f;
			isMoving = true;
		}
		if (Keyboard_IsKeyDown(KK_A))
		{
			inputDir.x += -1.0f;
			isMoving = true;
		}
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

	// アニメ切替
	if (isMoving) {
		g_Player3D.CurrentAnimIndex = PLAYER_ANIM_WALK;
	}
	else {
		g_Player3D.CurrentAnimIndex = PLAYER_ANIM_IDLE;
	}

	// カメラ基準移動
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

		if (g_Player3D.isDash)
		{
			g_Player3D.Velocity.x += moveWorld.x * (g_Player3D.moveSpeed * g_Player3D.dashMoveSpeed);
			g_Player3D.Velocity.z += moveWorld.z * (g_Player3D.moveSpeed * g_Player3D.dashMoveSpeed);
		}
		else
		{
			g_Player3D.Velocity.x += moveWorld.x * g_Player3D.moveSpeed;
			g_Player3D.Velocity.z += moveWorld.z * g_Player3D.moveSpeed;
		}

		// 向きの補間
		float targetYawRad = atan2f(moveWorld.x, moveWorld.z);
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		const float yawOffset = g_Player3D.FirstRotation.y;
		targetYawDeg += yawOffset;

		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f;
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}

	// 最大速度クランプ
	if (g_Player3D.Velocity.x >= g_Player3D.maxMoveSpeed)
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
	if (IsInputTrigger(JumpKey, gPad))
	{
		if (g_Player3D.isGround)
		{
			g_Player3D.Velocity.y = g_Player3D.jumpPower;
			g_Player3D.isGround = false;
			g_Player3D.state = PLAYER_STATE_FALL; // 上昇後はFALLへ
		}
	}

}

void Player3D_Change()
{
	// 変身処理があるならここに
}

void Player3D_Dash()
{
	if (IsInputPress(DashKey, gPad))
	{
		// ダッシュ開始/継続
		g_Player3D.isDash = true;
		g_Player3D.maxMoveSpeed = g_Player3D.FirstMaxMoveSpeed * g_Player3D.dashMoveSpeed;
	}
	else
	{
		// ダッシュ解除
		g_Player3D.isDash = false;
		g_Player3D.maxMoveSpeed = g_Player3D.FirstMaxMoveSpeed;

		// 状態復帰（地上/空中で適切に戻す）
		if (g_Player3D.isGround)
		{
			g_Player3D.state = Player3D_HasMoveInput() ? PLAYER_STATE_MOVE : PLAYER_STATE_IDLE;
		}
		else
		{
			g_Player3D.state = PLAYER_STATE_FALL;
		}
	}
}

void Player3D_Action()
{
	if (IsInputTrigger(ActionKey, gPad))
	{
		// アクションのトリガー処理
	}
	isTrigger = false;

	TRIGGER_HIT hit;
	if (!Collision_PlayerTrigger(&hit, 0.2f)) return;
	if (hit.side != TRIGGER_SIDE_FRONT) return;
	switch (hit.type)
	{
	case FIELD_GOAL:
		// ゴール処理
		break;
	case FIELD_OBJ_1:
		isTrigger = true;
		break;
	case FIELD_OBJ_2:
		// 何かの処理
		break;

	default:
		break;
	}
}

void Player3D_Draw(void)
{
	if (!g_Player3DActive) return;

	if (debugMode)
	{
		ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("Player3D.cpp"))
		{
			ImGui::Text("Trigger: %s", isTrigger ? "true" : "false");
			ImGui::TreePop();
		}
		ImGui::End();
	}

	XMMATRIX scale = XMMatrixScaling
	(
		g_Player3D.Scaling.x,
		g_Player3D.Scaling.y,
		g_Player3D.Scaling.z
	);

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

	MODEL* currentModel = g_Player3D.Model[g_Player3D.CurrentAnimIndex];

	if (currentModel != NULL)
	{
		ModelUpdateAnimation(currentModel, 10.0f / 600.0f);
		Shader_SetBones(currentModel);
	}

	ModelDraw(currentModel);
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

//=========================================================================================================
// mapの読み込み時の初期位置設定用
//=========================================================================================================
void Playe3D_setposition(XMFLOAT3 pos)
{
	g_Player3D.Firstposition = pos;

	if (PLAYER* p = GetPlayer3D())
	{
		g_Player3D.Firstposition = p->Position;
		// もし player3D.cpp 内に Firstposition 変数が既にある場合はそちらにも代入してください。
		// 例: Firstposition = p->Position;
	}
}

//=========================================================================================================
// ゲッター
//=========================================================================================================
XMFLOAT3 GetPlayer3DPosition()
{
	return g_Player3D.Position;
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

//=========================================================================================================
// 追加: 状態管理のためのヘルパー
//=========================================================================================================
static bool Player3D_HasMoveInput()
{
	if (!gPad.IsConnected())
	{
		if (Keyboard_IsKeyDown(KK_W)) return true;
		if (Keyboard_IsKeyDown(KK_S)) return true;
		if (Keyboard_IsKeyDown(KK_A)) return true;
		if (Keyboard_IsKeyDown(KK_D)) return true;
		return false;
	}
	else
	{
		float lx = gPad.GetLeftStickX();
		float ly = gPad.GetLeftStickY();
		const float deadzone = 0.20f;
		return (fabsf(lx) >= deadzone) || (fabsf(ly) >= deadzone);
	}
}

static void Player3D_HandleStateTransitions()
{
	// ジャンプ開始（地上で入力があればJUMPへ）
	if (g_Player3D.isGround && IsInputPress(JumpKey, gPad))
	{
		g_Player3D.state = PLAYER_STATE_JUMP;
	}

	// DASH開始（IDLE/MOVE時に入力があればDASHへ）
	if ((g_Player3D.state == PLAYER_STATE_IDLE || g_Player3D.state == PLAYER_STATE_MOVE) && IsInputPress(DashKey, gPad))
	{
		g_Player3D.state = PLAYER_STATE_DASH;
	}

	// 着地時の復帰
	if (g_Player3D.isGround)
	{
		// FALL/JUMPから地上へ戻ったら入力有無でIDLE/MOVEへ
		if (g_Player3D.state == PLAYER_STATE_FALL || g_Player3D.state == PLAYER_STATE_JUMP)
		{
			g_Player3D.state = Player3D_HasMoveInput() ? PLAYER_STATE_MOVE : PLAYER_STATE_IDLE;
		}

		// IDLE中に入力が来たらMOVEへ
		if (g_Player3D.state == PLAYER_STATE_IDLE && Player3D_HasMoveInput())
		{
			g_Player3D.state = PLAYER_STATE_MOVE;
		}
	}
	else
	{
		// 空中で下降中ならFALLへ（JUMP直後の上昇中は除く）
		if (g_Player3D.Velocity.y <= 0.0f && g_Player3D.state != PLAYER_STATE_JUMP)
		{
			g_Player3D.state = PLAYER_STATE_FALL;
		}
	}

	// DASH中で入力が離れたら解除（詳細は Player3D_Dash() 内でも保険）
	if (g_Player3D.state == PLAYER_STATE_DASH && !IsInputPress(DashKey, gPad))
	{
		g_Player3D.isDash = false;
		g_Player3D.maxMoveSpeed = g_Player3D.FirstMaxMoveSpeed;

		if (g_Player3D.isGround)
		{
			g_Player3D.state = Player3D_HasMoveInput() ? PLAYER_STATE_MOVE : PLAYER_STATE_IDLE;
		}
		else
		{
			g_Player3D.state = PLAYER_STATE_FALL;
		}
	}
}

static void Player3D_Idle()
{
	g_Player3D.CurrentAnimIndex = PLAYER_ANIM_IDLE;

	// 横方向は緩やかに減速
	g_Player3D.Velocity.x *= g_Player3D.dampingXZ;
	g_Player3D.Velocity.z *= g_Player3D.dampingXZ;
}