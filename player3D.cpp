#include "Player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"

//=========================================================================================================
//グローバル変数
//=========================================================================================================
PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
float g_StopTime = 0.0f;

// 入力ベクトル
XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

//リセット用
XMFLOAT3		Firstposition;
XMFLOAT3		FirstRotation;
XMFLOAT3		FirstScaling;
XMFLOAT3		FirstVelocity;
XMFLOAT3		FirstAcceleration;
PLAYER3D_STATE	FirstState;
float			FirstStopTime;
XMVECTOR		FirstQuaternion;

//プレイヤーステータス
float moveSpeed =		 0.005f;			//移動速度
float maxMoveSpeed =	 1.0f;				//最大移動速度
float maxGravity =		-0.25f;				//最大落下速度
float jumpPower =		 0.175f;			//ジャンプ力
bool isGround = false;						//接地判定（明示的に初期化）

//キーボード定義
//移動
static const auto UpKey =		KK_W;		//前進
static const auto RightKey =	KK_D;		//右移動
static const auto DownKey =		KK_S;		//後退
static const auto LeftKey =		KK_A;		//左移動
//行動
static const auto JumpKey =		KK_SPACE;	//ジャンプ
static const auto ActionKey =	KK_F;		//アクション
static const auto ChangeKey =	KK_F;		//影変身
//その他
static const auto ResetKey =	KK_R;		//リセット
static const auto MenuKey =		KK_ESCAPE;	//終了

//=========================================================================================================
//初期化処理
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Player3D.Model = ModelLoad("asset\\model\\Test_man_stand.fbx");

	Firstposition		= g_Player3D.Position		= XMFLOAT3(0.0f, 1.2f, 0.0f);
	FirstRotation		= g_Player3D.Rotation		= XMFLOAT3(-90.0f, 180.0f, 0.0f);
	FirstScaling		= g_Player3D.Scaling		= XMFLOAT3(0.01f, 0.01f, 0.01f);
	FirstVelocity		= g_Player3D.Velocity		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstAcceleration	= g_Player3D.Acceleration	= XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	FirstState			= g_Player3D.state			= PLAYER3D_STATE_MOVE;
	FirstStopTime		= g_StopTime				= 0.0f;
	FirstQuaternion		= g_Player3D.Quaternion		= XMQuaternionIdentity();
	
}

//=========================================================================================================
//終了処理
//=========================================================================================================
void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}

//=========================================================================================================
//更新処理
//=========================================================================================================
void Player3D_Update()
{
	Player3D_Respown();	//リスポーン

	//プレイヤー操作
	Player3D_Move();	//移動
	Player3D_Jump();	//ジャンプ
	Player3D_Change();	//影変身
	Player3D_Action();	//アクション

	Player3D_Gravity();	//重力処理
	

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
//描画処理
//=========================================================================================================
void Player3D_Draw(void)
{
	// ワールド行列の作成
	//スケーリング行列の作成
	XMMATRIX ScalingMatrix = XMMatrixScaling(g_Player3D.Scaling.x, g_Player3D.Scaling.y, g_Player3D.Scaling.z);
	//平行移動行列の作成
	XMMATRIX TranslationMatrix = XMMatrixTranslation(g_Player3D.Position.x, g_Player3D.Position.y, g_Player3D.Position.z);
	//回転行列の作成
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(g_Player3D.Rotation.x), XMConvertToRadians(g_Player3D.Rotation.y), XMConvertToRadians(g_Player3D.Rotation.z));
	//計算の順番「スケール*回転*平行移動」
	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;

	//プロジェクション行列作成
	XMMATRIX Projection = GetProjectionMatrix();

	//ビュー行列作成
	XMMATRIX View = GetViewMatrix();

	//最終的な変換行列を作成	順番に注意！！
	XMMATRIX WVP = WorldMatrix * View * Projection;

	//変換行列を頂点シェーダへセット
	Shader_SetWorldMatrix(WorldMatrix);
	Shader_SetMatrix(WVP);

	//描画リクエスト
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
//処理
//=========================================================================================================

void Player3D_Gravity()
{
	// 横・縦・奥行きの加算（既存ロジック保持）
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

	//摩擦による減速
	g_Player3D.Velocity.x *= 0.925f;
	g_Player3D.Velocity.y *= 0.98f;
	g_Player3D.Velocity.z *= 0.925f;

	// 座標に速度を加算
	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;



	// 静止チェック
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

void Player3D_Respown()
{
	//落下チェック
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

	if (Keyboard_IsKeyDown(UpKey))    inputDir.z += +1.0f;
	if (Keyboard_IsKeyDown(DownKey))  inputDir.z += -1.0f;
	if (Keyboard_IsKeyDown(RightKey)) inputDir.x += +1.0f;
	if (Keyboard_IsKeyDown(LeftKey))  inputDir.x += -1.0f;
	

	// 長さ計算
	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 0.0001f)
	{
		// 正規化ベクトル × 加速
		inputDir.x /= len;
		inputDir.z /= len;
		g_Player3D.Velocity.x += inputDir.x * moveSpeed;
		g_Player3D.Velocity.z += inputDir.z * moveSpeed;

		// --- 入力がある場合にのみ向きを更新 ---
		float targetYawRad = atan2f(inputDir.x, inputDir.z); // (x,z) -> 前方を Z とした角度
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// モデルの初期向きに合わせるオフセット（必要なら調整）
		const float yawOffset = FirstRotation.y;
		targetYawDeg += yawOffset;

		// スムーズ回転（角度差を最短経路で求めて補間）
		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f; // 0..1（1で即時回転）
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}
	// 入力無しのときは回転を変更しない（最後に向いていた方向を保持）
}

void Player3D_Jump()
{
	if (Keyboard_IsKeyDownTrigger(JumpKey))
	{
		g_Player3D.Velocity.y += jumpPower;// テスト
		if(isGround)
		{
			g_Player3D.Velocity.y += jumpPower;// 上向きに初速を与える
			// 空中にいる状態へ
			//g_Player3D.state = PLAYER3D_STATE_FALL;
		}
	}
}

void Player3D_Change()
{

}

void Player3D_Action()
{

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
