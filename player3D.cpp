#include "player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "debug.h"
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
float moveSpeed = 0.1f;				//移動速度
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

float g_StopTime=0.0f;
static bool debugMode = TRUE;
static XMFLOAT3 g_DetectHalfSize = XMFLOAT3(
	PLAYER3D_DETECT_HALF_X,
	PLAYER3D_DETECT_HALF_Y,
	PLAYER3D_DETECT_HALF_Z
);


static ImVec2 WorldToScreen(const XMFLOAT3& p)
{
	using namespace DirectX;

	// 1) ���������� backbuffer �ߴ磨�������һ�£�
	float bbWidth = (float)Direct3D_GetBackBufferWidth();
	float bbHeight = (float)Direct3D_GetBackBufferHeight();

	XMMATRIX view = GetViewMatrix();
	XMMATRIX proj = GetProjectionMatrix();
	XMMATRIX vp = XMMatrixMultiply(view, proj);


	XMVECTOR v = XMVectorSet(p.x, p.y, p.z, 1.0f);
	v = XMVector3TransformCoord(v, vp);

	XMFLOAT3 ndc;
	XMStoreFloat3(&ndc, v);


	float x_bb = (ndc.x * 0.5f + 0.5f) * bbWidth;
	float y_bb = (-ndc.y * 0.5f + 0.5f) * bbHeight;


	ImGuiIO& io = ImGui::GetIO();
	float x_imgui = x_bb / bbWidth * io.DisplaySize.x;
	float y_imgui = y_bb / bbHeight * io.DisplaySize.y;

	return ImVec2(x_imgui, y_imgui);
}


static void DebugDrawDetectBox()
{
	using namespace DirectX;

	ImDrawList* draw = ImGui::GetBackgroundDrawList();
	const XMFLOAT3& c = g_Player3D.Position;
	const XMFLOAT3& h = g_DetectHalfSize;


	XMFLOAT3 corners[8] =
	{
		{c.x - h.x, c.y - h.y, c.z - h.z},
		{c.x + h.x, c.y - h.y, c.z - h.z},
		{c.x + h.x, c.y + h.y, c.z - h.z},
		{c.x - h.x, c.y + h.y, c.z - h.z},
		{c.x - h.x, c.y - h.y, c.z + h.z},
		{c.x + h.x, c.y - h.y, c.z + h.z},
		{c.x + h.x, c.y + h.y, c.z + h.z},
		{c.x - h.x, c.y + h.y, c.z + h.z},
	};

	ImVec2 pts[8];
	for (int i = 0; i < 8; ++i)
		pts[i] = WorldToScreen(corners[i]);

	ImU32 col = IM_COL32(0, 255, 0, 255);

	auto Line = [&](int a, int b)
		{
			draw->AddLine(pts[a], pts[b], col, 1.0f);
		};


	Line(0, 1); Line(1, 2); Line(2, 3); Line(3, 0);

	Line(4, 5); Line(5, 6); Line(6, 7); Line(7, 4);

	Line(0, 4); Line(1, 5); Line(2, 6); Line(3, 7);
}



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
	if (debugMode)
	{


		ImGui::Begin("Debug - CHEN");
		if (ImGui::TreeNode("Player3D.cpp"))
		{
			ImGui::Text("PosX: %.2f", g_Player3D.Position.x);
			ImGui::Text("PosY: %.2f", g_Player3D.Position.y);
			ImGui::Text("PosZ: %.2f", g_Player3D.Position.z);
			ImGui::TreePop();
		}
		ImGui::End();

		DebugDrawDetectBox();
	}
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
	g_player3D.state = PLAYER3D_MOVE;
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
	
	XMFLOAT3 camPos = GetCameraPosition();
	XMFLOAT3 camAt = GetCameraAtPosition();

	// カメラの前方向ベクトル（正規化）
	XMFLOAT3 forward = { camAt.x - camPos.x,	0.0f, camAt.z - camPos.z };
	float flen = sqrtf(forward.x * forward.x + forward.z * forward.z);
	if (flen > 0.0001f) {
		forward.x /= flen;
		forward.z /= flen;
	}
	// カメラの右方向ベクトル
	XMFLOAT3 right = { forward.z, 0.0f, -forward.x };
	if (Keyboard_IsKeyDown(UpKey))
	{
		g_player3D.Position.x += forward.x * moveSpeed;
		g_player3D.Position.z += forward.z * moveSpeed;

	}
	if (Keyboard_IsKeyDown(RightKey))
	{
		g_player3D.Position.x += right.x * moveSpeed;
		g_player3D.Position.z += right.z * moveSpeed;
	}
	if (Keyboard_IsKeyDown(DownKey))
	{
		g_player3D.Position.x -= forward.x * moveSpeed;
		g_player3D.Position.z -= forward.z * moveSpeed;
	}
	if (Keyboard_IsKeyDown(LeftKey))
	{
		g_player3D.Position.x -= right.x * moveSpeed;
		g_player3D.Position.z -= right.z * moveSpeed;
	}
	float hit = Player3DField_Collision();
	//落下チェック
	if (g_player3D.Position.y < -10.0f)
	{
		g_player3D.state = PLAYER3D_RESPAWN;
		return;
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

XMFLOAT3 Player3D_GetDetectHalfSize()
{
	return g_DetectHalfSize;
}

bool Player3D_IsNearPoint(const XMFLOAT3& point)
{
	const XMFLOAT3& c = g_Player3D.Position;

	if (fabsf(point.x - c.x) > g_DetectHalfSize.x) return false;
	if (fabsf(point.y - c.y) > g_DetectHalfSize.y) return false;
	if (fabsf(point.z - c.z) > g_DetectHalfSize.z) return false;

	return true;
}