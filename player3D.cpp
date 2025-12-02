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

}

//=========================================================================================================
// stateごとの処理（Move状態）
//=========================================================================================================
void Player3D_Move()
{
	g_player3D.Velocity.x += g_player3D.Acceleration.x; //重力
	g_player3D.Velocity.y += g_player3D.Acceleration.y; //重力
	g_player3D.Velocity.z += g_player3D.Acceleration.z; //重力

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
