#include "Player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"

#include "debug.h"

//=========================================================================================================
//僌儘乕僶儖曄悢
//=========================================================================================================
PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
float g_StopTime = 0.0f;

//儕僙僢僩梡
XMFLOAT3		Firstposition;
XMFLOAT3		FirstRotation;
XMFLOAT3		FirstScaling;
XMFLOAT3		FirstVelocity;
XMFLOAT3		FirstAcceleration;
PLAYER3D_STATE	FirstState;
float			FirstStopTime;
XMVECTOR		FirstQuaternion;

//僾儗僀儎乕僗僥乕僞僗
float moveSpeed = 0.02f;				//堏摦懍搙
float maxMoveSpeed = 0.1f;				//嵟戝堏摦懍搙
float jumpPower = 0.15f;				//僕儍儞僾椡
bool isGround = false;					//愙抧敾掕

//僉乕儃乕僪掕媊
//堏摦
static const auto UpKey = KK_W;			//慜恑
static const auto RightKey = KK_D;		//塃堏摦
static const auto DownKey = KK_S;		//屻戅
static const auto LeftKey = KK_A;		//嵍堏摦
//峴摦
static const auto JumpKey = KK_SPACE;	//僕儍儞僾
static const auto ActionKey = KK_F;		//傾僋僔儑儞
static const auto ChangeKey = KK_F;		//塭曄恎
//偦偺懠
static const auto ResetKey = KK_R;		//儕僙僢僩
static const auto MenuKey = KK_ESCAPE;	//廔椆


static bool debugMode = TRUE;
static XMFLOAT3 g_DetectHalfSize = XMFLOAT3(
	PLAYER3D_DETECT_HALF_X,
	PLAYER3D_DETECT_HALF_Y,
	PLAYER3D_DETECT_HALF_Z
);


static ImVec2 WorldToScreen(const XMFLOAT3& p)
{
	using namespace DirectX;

	// 1) 先用真正的 backbuffer 尺寸（和摄像机一致）
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
//弶婜壔張棟
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 僨僶僀僗偲僨僶僀僗僐儞僥僉僗僩偺曐懚
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Player3D.Model = ModelLoad("asset\\model\\Test_man_stand.fbx");
	Firstposition		= g_Player3D.Position		= XMFLOAT3(0.0f, 1.6f, 0.0f);
	FirstRotation		= g_Player3D.Rotation		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstScaling		= g_Player3D.Scaling		= XMFLOAT3(0.01f, 0.01f, 0.01f);
	FirstVelocity		= g_Player3D.Velocity		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstAcceleration	= g_Player3D.Acceleration	= XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	FirstState			= g_Player3D.state			= PLAYER3D_STATE_MOVE;
	FirstStopTime		= g_StopTime				= 0.0f;
	FirstQuaternion		= g_Player3D.Quaternion		= XMQuaternionIdentity();
	
}

//=========================================================================================================
//廔椆張棟
//=========================================================================================================
void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}

//=========================================================================================================
//峏怴張棟
//=========================================================================================================
void Player3D_Update()
{
	Player3D_Gravity();

	//僾儗僀儎乕憖嶌
	Player3D_Move();	//堏摦
	Player3D_Jump();	//僕儍儞僾
	Player3D_Change();	//塭曄恎
	Player3D_Action();	//傾僋僔儑儞
	Player3D_Reset();	//儕僙僢僩
	Player3D_Respown();	//儕僗億乕儞

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
//昤夋張棟
//=========================================================================================================
void Player3D_Draw(void)
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


	

	// 儚乕儖僪峴楍偺嶌惉
	//僗働乕儕儞僌峴楍偺嶌惉
	XMMATRIX ScalingMatrix = XMMatrixScaling(g_Player3D.Scaling.x, g_Player3D.Scaling.y, g_Player3D.Scaling.z);
	//暯峴堏摦峴楍偺嶌惉
	XMMATRIX TranslationMatrix = XMMatrixTranslation(g_Player3D.Position.x, g_Player3D.Position.y, g_Player3D.Position.z);
	//夞揮峴楍偺嶌惉
	XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(g_Player3D.Quaternion);
	//寁嶼偺弴斣乽僗働乕儖*夞揮*暯峴堏摦乿
	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;

	//僾儘僕僃僋僔儑儞峴楍嶌惉
	XMMATRIX Projection = GetProjectionMatrix();

	//價儏乕峴楍嶌惉
	XMMATRIX View = GetViewMatrix();

	//嵟廔揑側曄姺峴楍傪嶌惉	弴斣偵拲堄両両
	XMMATRIX WVP = WorldMatrix * View * Projection;

	//曄姺峴楍傪捀揰僔僃乕僟傊僙僢僩
	Shader_SetWorldMatrix(WorldMatrix);
	Shader_SetMatrix(WVP);

	//昤夋儕僋僄僗僩
	ModelDraw(g_Player3D.Model);
	
}
//=========================================================================================================
// 僎僢僞乕
//=========================================================================================================
XMFLOAT3 GetPlayer3DPositon()
{
	return g_Player3D.Position;
}

//=========================================================================================================
//張棟
//=========================================================================================================

void Player3D_Gravity()
{
	g_Player3D.Velocity.x += g_Player3D.Acceleration.x; //廳椡
	g_Player3D.Velocity.y += g_Player3D.Acceleration.y; //廳椡
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z; //廳椡

	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;

	g_Player3D.Velocity.x *= 0.98f;		//岲傒偱尭悐偝偣傞
	//g_Player3D.Velocity.y *= 0.98f;	//岲傒偱尭悐偝偣傞
	g_Player3D.Velocity.z *= 0.98f;		//岲傒偱尭悐偝偣傞

	//惷巭僠僃僢僋
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
}

void Player3D_Respown()
{
	//棊壓僠僃僢僋
	if (g_Player3D.Position.y < -10.0f)
	{
		Player3D_Reset();
		return;
	}
}

void Player3D_Move()
{
	if (Keyboard_IsKeyDown(UpKey))
	{

	}
	if (Keyboard_IsKeyDown(RightKey))
	{

	}
	if (Keyboard_IsKeyDown(DownKey))
	{

	}
	if (Keyboard_IsKeyDown(LeftKey))
	{

	}
}

void Player3D_Jump()
{
	if (Keyboard_IsKeyDown(JumpKey))
	{
		// 抧柺偵偄傞偐偳偆偐傪敾掕
		if (!PLAYER3D_STATE_FALL)
		{
			// 忋岦偒偵弶懍傪梌偊傞乮抣偼挷惍偟偰偔偩偝偄乯
			g_Player3D.Velocity.y += jumpPower;
			// 嬻拞偵偄傞忬懺傊
			g_Player3D.state = PLAYER3D_STATE_FALL;
			// 拝抧僞僀儅乕儕僙僢僩
			g_StopTime = 0.0f;
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
	g_Player3D.Position		= Firstposition;
	g_Player3D.Rotation		= FirstRotation;
	g_Player3D.Scaling		= FirstScaling; 
	g_Player3D.Velocity		= FirstVelocity;
	g_Player3D.Acceleration = FirstAcceleration;
	g_Player3D.state		= FirstState;
	g_StopTime				= FirstStopTime;
	g_Player3D.Quaternion	= FirstQuaternion;
}

PLAYER3D* GetPlayer3D()
{
	return &g_Player3D;
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