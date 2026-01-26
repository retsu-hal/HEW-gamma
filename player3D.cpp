//Player3D.cpp
#include "Player3D.h"
#include "PlayerStatus.h"
#include "Keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"

//=========================================================================================================
// デバッグ用
//=========================================================================================================
#include "debug.h"
static bool debugMode = TRUE;

//=========================================================================================================
// グローバル変数
//=========================================================================================================

PLAYER g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
static float g_StopTime = 0.0f;

// プレイヤー当たり判定サイズ
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

bool isTrigger = false;

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;
	
	g_Player3D.Model = ModelLoad("asset\\model\\Hip_Hop_Dancing.fbx");

	g_Player3D.Firstposition = g_Player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	g_Player3D.FirstRotation = g_Player3D.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.FirstScaling = g_Player3D.Scaling = XMFLOAT3(0.01f, 0.01f, 0.01f);
	g_Player3D.FirstVelocity = g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.FirstAcceleration = g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	g_Player3D.FirstState = g_Player3D.state = PLAYER_STATE_MOVE;
	g_Player3D.FirstStopTime = g_StopTime = 0.0f;
	g_Player3D.FirstQuaternion = g_Player3D.Quaternion = XMQuaternionIdentity();

	g_Player3D.FirstMaxMoveSpeed = g_Player3D.maxMoveSpeed;	//初期最大移動速度
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
	if (!g_Player3DActive) return;

	Player3D_Respawn();	//リスポーン

	Player3D_Gravity();	//重力処理

	// プレイヤー操作
	
	Player3D_Move();	//移動
	Player3D_Dash();	//ダッシュ
	Player3D_Jump();	//ジャンプ
	Player3D_Change();	//影変身
	Player3D_Action();	//アクション
	

	switch (g_Player3D.state)
	{
	case PLAYER_STATE_IDLE:
		//Idleアニメーション
		break;
	case PLAYER_STATE_MOVE:
		//Moveアニメーション
		break;
	case PLAYER_STATE_FALL:
		//Fallアニメーション
		break;
	case PLAYER_STATE_ACTION:
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
	if (g_Player3D.Velocity.x >= g_Player3D.maxMoveSpeed) g_Player3D.Velocity.x = g_Player3D.maxMoveSpeed;
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

	// --- XZ移動（従来どおり） ---
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
	// 落下チェック
	if (g_Player3D.Position.y < -10.0f)
	{
		Player3D_Reset();
		return;
	}
	if (IsInputTrigger(ResetKey, gPad))
	{
		Player3D_Reset();
	}
}

void Player3D_Move()
{
	// 入力ベクトル
	XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

	// 前フレームの入力をリセット（キーを離したときに以前の入力が残らないようにする）
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (!gPad.IsConnected())
	{// キーボード入力
		if (Keyboard_IsKeyDown(KK_W)) inputDir.z += +1.0f;
		if (Keyboard_IsKeyDown(KK_S)) inputDir.z += -1.0f;
		if (Keyboard_IsKeyDown(KK_D)) inputDir.x += +1.0f;
		if (Keyboard_IsKeyDown(KK_A)) inputDir.x += -1.0f;
	}
	else
	{// コントローラー入力
		float lx = gPad.GetLeftStickX();
		float ly = gPad.GetLeftStickY();

		// デッドゾーンを入れて微小入力を無視
		const float deadzone = 0.20f;
		if (fabsf(lx) < deadzone) lx = 0.0f;
		if (fabsf(ly) < deadzone) ly = 0.0f;

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
		

		// 入力がある場合にのみ向きを更新
		float targetYawRad = atan2f(moveWorld.x, moveWorld.z);
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// モデルの初期向きに合わせるオフセット（必要なら調整）
		const float yawOffset = g_Player3D.FirstRotation.y;
		targetYawDeg += yawOffset;

		// スムーズ回転（角度差を最短経路で求めて補間）

		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f;
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}
	// 入力無しのときは回転を変更しない（最後に向いていた方向を保持）

	//速度制限
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
			g_Player3D.state = PLAYER_STATE_FALL;
		}
	}
}

void Player3D_Change()
{
	if (IsInputTrigger(ChangeKey, gPad))
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

void Player3D_Dash()
{
	if (IsInputPress(DashKey, gPad))
	{
		// ダッシュ処理
		// 最大移動速度を一時的に上げる
		g_Player3D.isDash = true;
		g_Player3D.maxMoveSpeed = g_Player3D.FirstMaxMoveSpeed * g_Player3D.dashMoveSpeed;
	}
	else
	{
		g_Player3D.isDash = false;
		g_Player3D.maxMoveSpeed = g_Player3D.FirstMaxMoveSpeed;
	}
}

void Player3D_Action()
{
	if (IsInputTrigger(ActionKey, gPad))
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
	if (hit.side != TRIGGER_SIDE_FRONT) return;// 前面以外は無視
	switch (hit.type)// 当たったオブジェクトの種類で処理分岐
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

//=========================================================================================================
// 描画処理
//=========================================================================================================
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

	// 変換行列を頂点シェーダへセット
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	ModelUpdateAnimation(g_Player3D.Model, 10.0f / 600.0f);   
	Shader_SetBones(g_Player3D.Model);	


	// モデルの描画リクエスト
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
