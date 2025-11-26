#include "Player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"

using namespace DirectX;

//=========================================================================================================
//グローバル変数
//=========================================================================================================
PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
float g_StopTime = 0.0f;
//XMFLOAT3 dir = { 0, 0, 0 };

//キーボード定義
//移動
static const auto UpKey = KK_W;			//前進
static const auto RightKey = KK_D;		//右移動
static const auto DownKey = KK_S;		//後退
static const auto LeftKey = KK_A;		//左移動
//行動
static const auto JumpKey = KK_SPACE;	//ジャンプ
static const auto ActionKey = KK_E;		//アクション
static const auto ChangeKey = KK_E;		//影変身

//デバッグ
static const auto RespawnKey = KK_R;	//リスポーン

//=========================================================================================================
//初期化処理
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Player3D.Model = ModelLoad("asset\\model\\test.fbx");
	g_Player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	g_Player3D.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.Scaling = XMFLOAT3(1.0f, 1.0f, 1.0f);
	g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	g_Player3D.state = PLAYER3D_STATE_MOVE;
	g_StopTime = 0.0f;
	g_Player3D.Quaternion = XMQuaternionIdentity();
	
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
	g_Player3D.Velocity.x += g_Player3D.Acceleration.x; //重力
	g_Player3D.Velocity.y += g_Player3D.Acceleration.y; //重力
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z; //重力

	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;

	g_Player3D.Velocity.x *= 0.98f;		//好みで減衰させる
	//g_Player3D.Velocity.y *= 0.98f;		//好みで減衰させる
	g_Player3D.Velocity.z *= 0.98f;		//好みで減衰させる

	//落下チェック
	if (g_Player3D.Position.y < -10.0f)
	{
		Player3D_Respawn();
		return;
	}
	//静止チェック
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

	//プレイヤー操作
	Player3D_Move();	//移動
	Player3D_Jump();	//ジャンプ
	Player3D_Change();	//影変身
	Player3D_Action();	//アクション

#if defined(_DEBUG)
	Player3D_Respawn();
#endif
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
	XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(g_Player3D.Quaternion);
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
//処理
//=========================================================================================================
void Player3D_Move()
{
	

	if (Keyboard_IsKeyDown(UpKey))
	{
		//dir.z += 1.0f;
	}
	if (Keyboard_IsKeyDown(RightKey))
	{
		//dir.x += 1.0f;
	}
	if (Keyboard_IsKeyDown(DownKey))
	{
		//dir.z -= 1.0f;
	}
	if (Keyboard_IsKeyDown(LeftKey))
	{
		//dir.x -= 1.0f;
	}

	//g_Player3D.Velocity.x += dir.x * 0.01f;
	//g_Player3D.Velocity.z += dir.z * 0.01f;

}

void Player3D_Jump()
{

}

void Player3D_Change()
{

}

void Player3D_Action()
{

}

void Player3D_Respawn()
{
	g_Player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	g_Player3D.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.Scaling = XMFLOAT3(1.0f, 1.0f, 1.0f);
	g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	g_Player3D.state = PLAYER3D_STATE_MOVE;
	g_StopTime = 0.0f;
	g_Player3D.Quaternion = XMQuaternionIdentity();
}

PLAYER3D* GetPlayer3D()
{
	return &g_Player3D;
}
