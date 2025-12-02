#include "player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
//=========================================================================================================
// マクロ定義
//=========================================================================================================
#define PLAYER3D_SPEEDMAX (2.0f)		//最大速度

//=========================================================================================================
// グローバル変数
//=========================================================================================================
PLAYER3D g_player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;

float g_StopTime = 0.0f;

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
float moveSpeed = 0.005f;				//移動速度
float maxMoveSpeed = 1.0f;				//最大移動速度
float maxGravity = -0.25f;				//最大落下速度
float jumpPower = 0.25f;				//ジャンプ力
bool isGround = false;					//接地判定

//キーボード定義
//移動
static const auto UpKey = KK_W;			//前進
static const auto RightKey = KK_D;		//右移動
static const auto DownKey = KK_S;		//後退
static const auto LeftKey = KK_A;		//左移動
//行動
static const auto JumpKey = KK_SPACE;	//ジャンプ
static const auto ActionKey = KK_F;		//アクション
static const auto ChangeKey = KK_F;		//影変身
//その他
static const auto ResetKey = KK_R;		//リセット
static const auto MenuKey = KK_ESCAPE;	//終了
=======
float g_StopTime=0.0f;


//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_player3D.Model= ModelLoad("asset\\model\\Test_man_stand.fbx");
	g_player3D.Position = XMFLOAT3(0.0f,1.2f,0.0f);
	g_player3D.Rotation = XMFLOAT3(-90.0f,0.0f,0.0f);
	g_player3D.Scaling = XMFLOAT3(0.01f,0.01f,0.01f);
	g_player3D.Velocity = XMFLOAT3(0.0f,0.0f,0.0f);
	g_player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	g_player3D.state = PLAYER3D_MOVE;
	g_StopTime = 0.0f;
	g_player3D.Quaternion = XMQuaternionIdentity();
	g_player3D.Axis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Player3D_Finalize()
{
	ModelRelease(g_player3D.Model);
}

//=========================================================================================================
// 更新処理
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
	
	switch (g_player3D.state)

	{
	case PLAYER3D_IDLE:
		Player3D_Idle();
		break;
	case PLAYER3D_MOVE:
		Player3D_Move();
		break;
	case PLAYER3D_DIRECTION:
		Player3D_Direction();
		break;
	case PLAYER3D_POWER:
		Player3D_Power();
		break;
	case PLAYER3D_RESPAWN:
		Player3D_Respawn();
		Camera_Initialize();
		break;
	}

}


//=========================================================================================================
// 描画処理
//=========================================================================================================
void Player3D_Draw()
{
	//ワールド行列作成
	XMMATRIX scale = XMMatrixScaling
	(
		g_player3D.Scaling.x,
		g_player3D.Scaling.y,
		g_player3D.Scaling.z);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw
	(
		XMConvertToRadians(g_player3D.Rotation.x),
		XMConvertToRadians(g_player3D.Rotation.y),
		XMConvertToRadians(g_player3D.Rotation.z)
	);

	XMMATRIX translation = XMMatrixTranslation
	(
		g_player3D.Position.x,
		g_player3D.Position.y,
		g_player3D.Position.z
	);

	XMMATRIX world = scale * rotation * translation;

	//変換行列作成
	XMMATRIX view = GetViewMatrix();
	XMMATRIX projection = GetProjectionMatrix();
	XMMATRIX wvp = world * view * projection;

	//シェーダーへ行列をセット
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	//モデルの描画リクエスト
	ModelDraw(g_player3D.Model);
}

//=========================================================================================================
// ゲッター
//=========================================================================================================
XMFLOAT3 GetPlayer3DPositon()
{
	return g_player3D.Position;
}

//=========================================================================================================
// stateごとの処理（Idle状態）
//=========================================================================================================
void Player3D_Idle()
{

	g_Player3D.Velocity.x += g_Player3D.Acceleration.x; //重力
	if (g_Player3D.Velocity.y < maxGravity)
	{
		g_Player3D.Velocity.y = maxGravity;
	}
	else
	{
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y; //重力
	}
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z; //重力


	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;
=======

}


//=========================================================================================================
// stateごとの処理（Move状態）
//=========================================================================================================
void Player3D_Move()
{
	g_player3D.Velocity.x += g_player3D.Acceleration.x; //重力
	g_player3D.Velocity.y += g_player3D.Acceleration.y; //重力
	g_player3D.Velocity.z += g_player3D.Acceleration.z; //重力


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
	float hit = Player3DField_Collision();
}

void Player3D_Respown()
{
=======
	g_player3D.Position.x += g_player3D.Velocity.x;
	g_player3D.Position.y += g_player3D.Velocity.y;
	g_player3D.Position.z += g_player3D.Velocity.z;

	g_player3D.Velocity.x *= 0.98f;		//好みで減衰させる
	//g_player3D.Velocity.y *= 0.98f;		//好みで減衰させる
	g_player3D.Velocity.z *= 0.98f;		//好みで減衰させる


	//落下チェック
	if (g_player3D.Position.y < -10.0f)
	{
		g_player3D.state = PLAYER3D_RESPAWN;
		return;
	}
	
	//前移動
	if (Keyboard_IsKeyDown(KK_W))
	{
		g_player3D.Position.z += 0.1f;
	}

void Player3D_Move()
{
	if (Keyboard_IsKeyDown(UpKey))
	{
		//g_Player3D.Position.z += moveSpeed;
		g_Player3D.Velocity.z += +moveSpeed;
	}
	if (Keyboard_IsKeyDown(RightKey))
	{
		//g_Player3D.Position.x += moveSpeed;
		g_Player3D.Velocity.x += +moveSpeed;
	}
	if (Keyboard_IsKeyDown(DownKey))
	{
		//g_Player3D.Position.z += -moveSpeed;
		g_Player3D.Velocity.z += -moveSpeed;
	}
	if (Keyboard_IsKeyDown(LeftKey))
	{
		//g_Player3D.Position.x += -moveSpeed;
		g_Player3D.Velocity.x += -moveSpeed;

	//後ろ移動
	if (Keyboard_IsKeyDown(KK_S))
	{
		g_player3D.Position.z -= 0.1f;
	}

	//右移動
	if (Keyboard_IsKeyDown(KK_D))
	{
		g_player3D.Position.x += 0.1f;

	}

	//左移動
	if (Keyboard_IsKeyDown(KK_A))
	{
		g_player3D.Position.x -= 0.1f;
	}

	float hit = Player3DField_Collision();

	
}

//=========================================================================================================
// stateごとの処理（Power状態）
//=========================================================================================================
void Player3D_Power()
{
	/*float power = PLAYER3D_SPEEDMAX * 0.12f;

	g_player3D.Velocity.x *= power;
	g_player3D.Velocity.y *= power;
	g_player3D.Velocity.z *= power;*/



	g_player3D.state = PLAYER3D_MOVE;
}

//=========================================================================================================
// stateごとの処理（Direction状態）
//=========================================================================================================
void Player3D_Direction()
{
	//キーを押したら転がる
	if (Keyboard_IsKeyDownTrigger(KK_F))
	{
		//カメラの向きを取得
		XMFLOAT3 Cap = GetCameraAtPosition();
		XMFLOAT3 Cp = GetCameraPosition();
		XMFLOAT3 Direction;
		Direction.x = Cap.x - Cp.x;
		Direction.y = 0.0f;
		Direction.z = Cap.z - Cp.z;

		//正規化
		float len = sqrtf(Direction.x * Direction.x + Direction.y * Direction.y + Direction.z * Direction.z);
		Direction.x /= len;
		Direction.y /= len;
		Direction.z /= len;

		g_player3D.Velocity = Direction;
		g_player3D.state =PLAYER3D_MOVE;
	}
}

//=========================================================================================================
// stateごとの処理（Respawn状態）
//=========================================================================================================
void Player3D_Respawn()
{
	g_player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	g_player3D.Rotation = XMFLOAT3(-90.0f, 0.0f, 0.0f);
	g_player3D.Scaling = XMFLOAT3(0.01f, 0.01f, 0.01f);
	g_player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	g_player3D.state =PLAYER3D_MOVE;
	g_StopTime = 0.0f;
	g_player3D.Quaternion = XMQuaternionIdentity();
	g_player3D.Axis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
}

PLAYER3D* GetPlayer3D()
{
	return &g_player3D;
}
