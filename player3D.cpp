#include "Player3D.h"
#include "Controller.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "manager.h"
#include "Input.h"

#include "debug.h"// デバッグ用

//=========================================================================================================
// マクロ定義
//=========================================================================================================

//=========================================================================================================
// グローバル変数
//=========================================================================================================
PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
float g_StopTime = 0.0f;

// コントローラー
extern Controller gPad;

// 入力ベクトル
XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

// リセット用
XMFLOAT3		Firstposition;
XMFLOAT3		FirstRotation;
XMFLOAT3		FirstScaling;
XMFLOAT3		FirstVelocity;
XMFLOAT3		FirstAcceleration;
PLAYER3D_STATE	FirstState;
float			FirstStopTime;
XMVECTOR		FirstQuaternion;

// プレイヤーステータス
float moveSpeed = 0.005f;			//移動速度
float maxMoveSpeed = 1.0f;			//最大移動速度
float maxFallSpeed = -0.5f;			//最大落下速度
float gravityPower = 0.925f;		//重力加速度
float jumpPower = 0.175f;			//ジャンプ力
bool isGround = false;				//接地判定

float FirstMaxMoveSpeed = maxMoveSpeed;



// キーボード定義
// 移動
static const auto UpKey = KK_W;			//前進
static const auto RightKey = KK_D;		//右移動
static const auto DownKey = KK_S;		//後退
static const auto LeftKey = KK_A;		//左移動
// 行動
static const auto JumpKey = KK_SPACE;	//ジャンプ
static const auto ActionKey = KK_F;		//アクション
static const auto ChangeKey = KK_F;		//影変身
// その他
static const auto ResetKey = KK_R;		//リセット
static const auto MenuKey = KK_ESCAPE;	//終了

static bool debugMode = TRUE;

static XMFLOAT3 g_DetectHalfSize = XMFLOAT3(
	PLAYER3D_DETECT_HALF_X,
	PLAYER3D_DETECT_HALF_Y,
	PLAYER3D_DETECT_HALF_Z
);

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
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

	FirstMaxMoveSpeed = maxMoveSpeed;			//初期最大移動速度

}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void Player3D_Update()
{
	Player3D_Respawn();	//リスポーン

	Player3D_Gravity();	//重力処理


	// プレイヤー操作
	Player3D_Move();	//移動
	Player3D_Jump();	//ジャンプ
	Player3D_Change();	//影変身
	Player3D_Action();	//アクション

	Player3D_Fall();	//空中処理

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

//=========================================================================================================
// 描画処理
//=========================================================================================================
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

	//????s???
	XMMATRIX view = GetViewMatrix();
	XMMATRIX projection = GetProjectionMatrix();
	XMMATRIX wvp = world * view * projection;

	// 変換行列を頂点シェーダへセット
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	// モデルの描画リクエスト
	ModelDraw(g_Player3D.Model);
}

//=========================================================================================================
// ゲッター
//=========================================================================================================
XMFLOAT3 GetPlayer3DPositon()
{
	return g_Player3D.Position;
}

//=========================================================================================================
// 処理
//=========================================================================================================

void Player3D_Gravity()
{
	// --- 重力加算（空中のみ） ---
	if (!isGround)
	{
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y;

		if (g_Player3D.Velocity.y < maxFallSpeed)
			g_Player3D.Velocity.y = maxFallSpeed;
	}

	// --- Y移動 ---
	float nextY = g_Player3D.Position.y + g_Player3D.Velocity.y;

	// --- レイ開始点（現在位置基準） ---
	XMFLOAT3 rayStart = g_Player3D.Position;
	rayStart.y += 0.1f;

	float rayLength = fabsf(g_Player3D.Velocity.y) + PLAYER3D_RADIUS + 0.3f;
	float hitY = 0.0f;

	bool hitGround = Collision_RayToField(
		rayStart,
		XMFLOAT3(0, -1, 0),
		rayLength,
		&hitY
	);

	if (hitGround && g_Player3D.Velocity.y <= 0.0f &&
		nextY <= hitY + PLAYER3D_RADIUS)
	{
		// --- 着地 ---
		g_Player3D.Position.y = hitY + PLAYER3D_RADIUS;
		g_Player3D.Velocity.y = 0.0f;
		isGround = true;
		g_Player3D.state = PLAYER3D_STATE_MOVE;
	}
	else
	{
		// --- 空中 ---
		g_Player3D.Position.y = nextY;
		isGround = false;
		g_Player3D.state = PLAYER3D_STATE_FALL;
	}

	// --- XZ移動（従来どおり） ---
	g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *= gravityPower;
	g_Player3D.Velocity.z *= gravityPower;

	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.z += g_Player3D.Velocity.z;
}


void Player3D_Respawn()
{
	// 落下チェック
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
	// 前フレームの入力をリセット（キーを離したときに以前の入力が残らないようにする）
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (!gPad.IsConnected())
	{// キーボード入力
		if (Keyboard_IsKeyDown(UpKey))    inputDir.z += +1.0f;
		if (Keyboard_IsKeyDown(DownKey))  inputDir.z += -1.0f;
		if (Keyboard_IsKeyDown(RightKey)) inputDir.x += +1.0f;
		if (Keyboard_IsKeyDown(LeftKey))  inputDir.x += -1.0f;
	}
	else
	{// コントローラー入力
		float lx = gPad.GetLeftStickX(); // -1～1
		float ly = gPad.GetLeftStickY(); // -1～1

		// デッドゾーンを入れて微小入力を無視
		const float deadzone = 0.20f;
		if (fabsf(lx) < deadzone) lx = 0.0f;
		if (fabsf(ly) < deadzone) ly = 0.0f;

		inputDir.x = lx;       // 左右
		inputDir.y = 0.0f;     // 3Dなら Y は高さとして使わない
		inputDir.z = -ly;      // 前後
	}

	// 長さ計算
	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 0.0001f)
	{
		// 正規化ベクトル × 加速
		inputDir.x /= len;
		inputDir.z /= len;
		g_Player3D.Velocity.x += inputDir.x * moveSpeed;
		g_Player3D.Velocity.z += inputDir.z * moveSpeed;

		// 入力がある場合にのみ向きを更新
		float targetYawRad = atan2f(inputDir.x, inputDir.z); //(x,z) -> 前方を Z とした角度
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// モデルの初期向きに合わせるオフセット（必要なら調整）
		const float yawOffset = FirstRotation.y;
		targetYawDeg += yawOffset;

		// スムーズ回転（角度差を最短経路で求めて補間）
		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f; //0..1（1で即時回転）
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}
	// 入力無しのときは回転を変更しない（最後に向いていた方向を保持）

	//速度制限
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
	if (Keyboard_IsKeyDownTrigger(JumpKey))
	{
		if(isGround)
		{
			g_Player3D.Velocity.y = jumpPower;
			isGround = false;
			g_Player3D.state = PLAYER3D_STATE_FALL;
		}
	}
}

void Player3D_Change()
{
	if (Keyboard_IsKeyDownTrigger(ChangeKey))
	{
		//壁に近づいたときに反応
		//影に変身
	}
}

void Player3D_Action()
{
	if(Keyboard_IsKeyDownTrigger(ActionKey))
	{
		//箱を持つ


		//照明操作

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

void Player3D_Fall()
{
	// レイ開始点（少し上から）
	XMFLOAT3 rayStart = g_Player3D.Position;
	rayStart.y += 0.3f;

	XMFLOAT3 rayDir(0.0f, -1.0f, 0.0f);

	// 落下速度連動レイ
	float rayLength = fabsf(g_Player3D.Velocity.y) + PLAYER3D_RADIUS + 0.2f;

	float hitY = 0.0f;
	bool hitGround = Collision_RayToField(
		rayStart,
		rayDir,
		rayLength,
		&hitY
	);
	const float groundSnap = 0.01f;

	if (hitGround && g_Player3D.Velocity.y <= 0.0f)
	{
		if (fabsf(g_Player3D.Position.y - (hitY + PLAYER3D_RADIUS)) > groundSnap)
		{
			g_Player3D.Position.y = hitY + PLAYER3D_RADIUS;
		}

		g_Player3D.Velocity.y = 0.0f;
		isGround = true;
		g_Player3D.state = PLAYER3D_STATE_MOVE;
	}
	else
	{
		isGround = false;
		g_Player3D.state = PLAYER3D_STATE_FALL;

		maxMoveSpeed -= 0.05f;
		if (maxMoveSpeed < 0.1f)
			maxMoveSpeed = 0.1f;
	}
}




PLAYER3D* GetPlayer3D()
{
	return &g_Player3D;
}

XMFLOAT3 Player3D_GetDetectHalfSize()
{
	return g_DetectHalfSize;
}