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
static float g_StopTime = 0.0f;

// コントローラー
extern Controller gPad;

// 入力ベクトル
static XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

// リセット用
static XMFLOAT3		Firstposition;
static XMFLOAT3		FirstRotation;
static XMFLOAT3		FirstScaling;
static XMFLOAT3		FirstVelocity;
static XMFLOAT3		FirstAcceleration;
static PLAYER3D_STATE	FirstState;
static float			FirstStopTime;
static XMVECTOR		FirstQuaternion;

// プレイヤーステータス
float moveSpeed = 0.005f;			//移動速度
float maxMoveSpeed = 1.0f;			//最大移動速度
float maxFallSpeed = -0.5f;			//最大落下速度
float  dampingXZ = 0.925f;			//摩擦係数
//float gravityPower = 1.0f;		//重力加速度（もしかしたら使う予定）
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
static const auto MenuKey = KK_ESCAPE;	//メニュー

static bool debugMode = true;

// プレイヤー当たり判定半分の大きさ
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

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Player3D.Model = ModelLoad("asset\\model\\Hip_Hop_Dancing.fbx");

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




//=========================================================================================================
// ゲッター
//=========================================================================================================
XMFLOAT3 GetPlayer3DPosition()
{
	return g_Player3D.Position;
}


//=========================================================================================================
// 処理
//=========================================================================================================

void Player3D_Gravity()
{
	// --- 重力加算（空中のみ） ---
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

	// --- XZ移動（従来どおり） ---
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

		// 左右が逆になる問題を修正（スティック右で x が正になるよう符号を反転）
		inputDir.x = -lx;      // 左右
		inputDir.y = 0.0f;     // 3Dなら Y は高さとして使わない
		inputDir.z = -ly;      // 前後
	}

	// 長さ計算
	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 1e-6f)
	{
		// 正規化ベクトル × 加速
		inputDir.x /= len;
		inputDir.z /= len;

		// カメラの向きに合わせて移動方向を変換
		XMFLOAT3 camPos = GetCameraPosition();
		XMFLOAT3 camAt = GetCameraAtPosition();
		XMFLOAT3 camFwd = XMFLOAT3(
			camAt.x - camPos.x,
			0.0f,
			camAt.z - camPos.z
		);// カメラの前方向ベクトル
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

		// カメラの右方向ベクトル
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

		// 移動方向をワールド座標に変換
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

		// 入力がある場合にのみ向きを更新
		float targetYawRad = atan2f(moveWorld.x, moveWorld.z);
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
	if (Keyboard_IsKeyDownTrigger(JumpKey) || gPad.IsButtonPressed(JumpKey))
	{
		if (isGround)
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
	//isTrigger = false;

	TRIGGER_HIT hit;
	if (!Collision_PlayerTrigger(&hit, 0.2f)) return;
	if (hit.side != TRIGGER_SIDE_FRONT) return;// 前面以外は無視
	switch (hit.type)// 当たったオブジェクトの種類で処理分岐
	{
	case FIELD_GOAL:

		break;
	case FIELD_OBJ_1:
		//isTrigger = true;
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

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Player3D_Draw(void)
{

	if (debugMode)
	{
		//ImGui::Begin("Debug - han");
		//if (ImGui::TreeNode("Player3D.cpp"))
		//{
		//	ImGui::Text("Pos: %.2f,%.2f,%.2f", g_Player3D.Position.x, g_Player3D.Position.y, g_Player3D.Position.z);
		//	ImGui::Text("Trigger: %s", isTrigger ? "true" : "false");
		//	ImGui::TreePop();
		//}
		//ImGui::End();
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

	// 変換行列を頂点シェーダへセット
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	ModelUpdateAnimation(g_Player3D.Model, 10.0f / 600.0f);   // CPU animation
	Shader_SetBones(g_Player3D.Model);	// upload bones

	// モデルの描画リクエスト
	ModelDraw(g_Player3D.Model);
}
