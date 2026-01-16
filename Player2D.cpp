#include "Player2D.h"
#include "PlayerStatus.h"
#include "KeyBind.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "sprite.h"

//=========================================================================================================
// デバッグ用
#include "debug.h"
#include "MathUtil.h"
using namespace mu;

//=========================================================================================================
// デバッグ用
//=========================================================================================================
#include "debug.h"

//=========================================================================================================
// グローバル変数
//=========================================================================================================
PLAYER g_Player2D;
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static  ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11ShaderResourceView* g_Texture;		//テクスチャ変数

static float g_StopTime = 0.0f;

static Vertex3D Player2DVertex[4] = {
	{//頂点0 LEFT-TOP
		XMFLOAT3(-1.0f, 1.0f, 0.0f),		//座標
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//法線
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(0.0f,0.0f)					//テクスチャ座標
	},

	{//頂点1 RIGHT-TOP
		XMFLOAT3(1.0f, 1.0f, 0.0f),		//座標
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//法線
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(1.0f,0.0f)					//テクスチャ座標
	},

	{//頂点2 LEFT-BOTTOM
		XMFLOAT3(-1.0f, 0.0f, 0.0f),		//座標
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//法線
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(0.0f,1.0f)					//テクスチャ座標
	},

	{//頂点3 RIGHT-BOTTOM
		XMFLOAT3(1.0f, 0.0f, 0.0f),		//座標
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//法線
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//カラー
		XMFLOAT2(1.0f,1.0f)					//テクスチャ座標
	},
};

//プレイヤー当たり判定サイズ
static XMFLOAT3 g_SolidHalfSize_2d = XMFLOAT3(
	PLAYER2D_SOLID_HALF_X,
	PLAYER2D_SOLID_HALF_Y,
	PLAYER2D_SOLID_HALF_Z
);

static bool g_Player2DActive = false;

//=========================================================================================================
// 初期化処理
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;

	// テクスチャ
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\Texture\\player2d.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	//頂点バッファの生成
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));//0でクリア
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * 4;//格納できる頂点数*頂点サイズ
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	g_pDevice->CreateBuffer(&bd, NULL, &g_VertexBuffer);

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex3D* vertex = (Vertex3D*)msr.pData;
	CopyMemory(&vertex[0], &Player2DVertex[0], sizeof(Vertex3D) * 4);
	g_pContext->Unmap(g_VertexBuffer, 0);
}

//=========================================================================================================
// 終了処理
//=========================================================================================================
void Player2D_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_Texture);
}

//=========================================================================================================
// 更新処理
//=========================================================================================================
void Player2D_Update()
{
	if (!g_Player2DActive) return;

	Player2D_Respawn();	//リスポーン

	Player2D_Gravity();	//重力処理

	// プレイヤー操作
	Player2D_Move();	//移動
	Player2D_Jump();	//ジャンプ
	Player2D_Change();	//影変身


	switch (g_Player2D.state)
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


	default:
		break;
	}
}

//=========================================================================================================
// ゲッター
//=========================================================================================================
XMFLOAT3 GetPlayer2DPosition()
{
	return g_Player2D.Position;
}



//=========================================================================================================
// セッター
//=========================================================================================================


//=========================================================================================================
// 処理
//=========================================================================================================
void Player2D_Gravity()
{
	// 横・縦・奥行きの加算
	if (g_Player2D.Velocity.x >= g_Player2D.maxMoveSpeed)
	{
		g_Player2D.Velocity.x = g_Player2D.maxMoveSpeed;
	}
	else
	{
		g_Player2D.Velocity.x += g_Player2D.Acceleration.x;
	}

	if (g_Player2D.Velocity.y < g_Player2D.gravityPower)
	{
		g_Player2D.Velocity.y = g_Player2D.gravityPower;
	}
	else
	{
		g_Player2D.Velocity.y += g_Player2D.Acceleration.y;
	}

	if (g_Player2D.Velocity.z >= g_Player2D.maxMoveSpeed)
	{
		g_Player2D.Velocity.z = g_Player2D.maxMoveSpeed;
	}
	else
	{
		g_Player2D.Velocity.z += g_Player2D.Acceleration.z;
	}

	// 摩擦による減速
	g_Player2D.Velocity.x *= 0.925f;
	//g_Player2D.Velocity.y *= 0.98f;

	// 座標に速度を加算
	g_Player2D.Position.x += g_Player2D.Velocity.x;
	g_Player2D.Position.y += g_Player2D.Velocity.y;



	// 静止チェック
	float len = LengthSq(g_Player2D.Velocity);
	if (len <= 0.0002f)
	{
		g_StopTime++;
		if (g_StopTime > (60.0f * 2))
		{
			g_Player2D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
			g_Player2D.state = PLAYER_STATE_IDLE;
			g_StopTime = 0.0f;
		}
	}

	int hit = Player3DField_Collision();
}

void Player2D_Respawn()
{
	// 落下チェック
	if (g_Player2D.Position.y < -10.0f)
	{
		Player2D_Reset();
		return;
	}
	if (IsInputDown(ResetKey, gPad))
	{
		Player2D_Reset();
	}
}

void Player2D_Move()
{
	XMFLOAT3 inputDir;
	
	// 前フレームの入力をリセット（キーを離したときに以前の入力が残らないようにする）
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (IsInputDown(RightKey, gPad)) inputDir.x += +1.0f;
	if (IsInputDown(LeftKey, gPad))  inputDir.x += -1.0f;


	// 長さ計算
	float len = Length2D(inputDir);
	if (len > 0.0001f)
	{
		// 正規化ベクトル × 加速
		inputDir = Normalize2D(inputDir);
		g_Player2D.Velocity.x += inputDir.x * g_Player2D.moveSpeed;
		g_Player2D.Velocity.z += inputDir.z * g_Player2D.moveSpeed;

		// 入力がある場合にのみ向きを更新
		float targetYawRad = atan2f(inputDir.x, inputDir.z); //(x,z) -> 前方を Z とした角度
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// モデルの初期向きに合わせるオフセット（必要なら調整）
		const float yawOffset = g_Player2D.FirstRotation.y;
		targetYawDeg += yawOffset;

		// スムーズ回転（角度差を最短経路で求めて補間）
		float currentYaw = g_Player2D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f; //0..1（1で即時回転）
	}
	// 入力無しのときは回転を変更しない（最後に向いていた方向を保持）
}

void Player2D_Jump()
{
	if (IsInputDown(JumpKey, gPad))
	{
		g_Player2D.Velocity.y += g_Player2D.jumpPower;//テスト
		if (g_Player2D.isGround)
		{
			g_Player2D.Velocity.y += g_Player2D.jumpPower;//上向きに初速を与える
			// 空中にいる状態へ
			// g_Player2D.state = PLAYER2D_STATE_FALL;
		}
	}
}

void Player2D_Change()
{
	if (IsInputDown(ChangeKey, gPad))
	{
		// ◆3Dに変身
		// 
		// 3Dキャラクターを2Dプレイヤーの座標を参照して生成
		// ↓
		// 2Dプレイヤーを削除する
		//
	}
}

void Player2D_Reset()
{
	g_Player2D.Position = g_Player2D.Firstposition;
	g_Player2D.Rotation = g_Player2D.FirstRotation;
	g_Player2D.Scaling = g_Player2D.FirstScaling;
	g_Player2D.Velocity = g_Player2D.FirstVelocity;
	g_Player2D.Acceleration = g_Player2D.FirstAcceleration;
	g_Player2D.state = g_Player2D.FirstState;
	g_StopTime = g_Player2D.FirstStopTime;
	g_Player2D.Quaternion = g_Player2D.FirstQuaternion;
}

PLAYER* GetPlayer2D()
{
	return &g_Player2D;
}

XMFLOAT3 Player2D_GetSolidHalfSize()
{
	return g_SolidHalfSize_2d;
}

//=========================================================================================================
// 描画処理
//=========================================================================================================
void Player2D_Draw(void)
{

	if (!g_Player2DActive) return;

	XMMATRIX scale = XMMatrixScaling
	(
		g_Player2D.Scaling.x,
		g_Player2D.Scaling.y,
		g_Player2D.Scaling.z);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw
	(
		XMConvertToRadians(g_Player2D.Rotation.x),
		XMConvertToRadians(g_Player2D.Rotation.y),
		XMConvertToRadians(g_Player2D.Rotation.z)
	);

	XMMATRIX translation = XMMatrixTranslation
	(
		g_Player2D.Position.x,
		g_Player2D.Position.y,
		g_Player2D.Position.z
	);

	XMMATRIX world = scale * rotation * translation;

	XMMATRIX view = GetViewMatrix();
	XMMATRIX projection = GetProjectionMatrix();
	XMMATRIX wvp = world * view * projection;

	// 変換行列を頂点シェーダへセット
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);
	g_pContext->PSSetShaderResources(0, 1, &g_Texture);
	UINT stride = sizeof(Vertex3D);
	UINT offset = 0;
	g_pContext->IASetVertexBuffers(0, 1, &g_VertexBuffer, &stride, &offset);
	g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	SetBlendState(BLENDSTATE_ALFA);
	g_pContext->Draw(4, 0);

}

void Player2D_InitAt(const XMFLOAT3& pos, const XMFLOAT3& rot)
{
	g_Player2D.Firstposition = g_Player2D.Position = pos;
	g_Player2D.FirstRotation = g_Player2D.Rotation = rot;

	g_Player2D.Scaling = XMFLOAT3(1.0f, 2.0f, 1.0f);

	g_Player2D.Velocity = XMFLOAT3(0, 0, 0);
	g_Player2D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);

	g_Player2D.state = PLAYER_STATE_MOVE;
	g_StopTime = 0.0f;

	g_Player2D.Quaternion = XMQuaternionIdentity();

	g_Player2DActive = true;
}

void Player2D_Uninit()
{
	g_Player2DActive = false;

	g_Player2D.Velocity = XMFLOAT3(0, 0, 0);
	g_Player2D.state = PLAYER_STATE_IDLE;
	g_StopTime = 0.0f;
}

void Player2D_SetActive(bool active)
{
	g_Player2DActive = active;
}
