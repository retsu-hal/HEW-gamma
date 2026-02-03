//Player2D.cpp
#include "Player2D.h"
#include "PlayerStatus.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "sprite.h"

//=========================================================================================================
// åƒ¨åƒ¶åƒ¢åƒŒæ¢?
#include "debug.h"
#include "MathUtil.h"
using namespace mu;

//=========================================================================================================
// åƒ¨åƒ¶åƒ¢åƒŒæ¢?
//=========================================================================================================
#include "debug.h"

//=========================================================================================================
// åƒŒåEä¹•åƒ¶å„–æ›„æ‚?
//=========================================================================================================
PLAYER g_Player2D;
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static  ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11ShaderResourceView* g_Texture;		//åƒ¥åƒ‹åEåƒ å„æ›„æ‚?

static float g_StopTime = 0.0f;
static bool debugMode;


static Vertex3D Player2DVertex[4] = {
	{//æ€æ? LEFT-TOP
		XMFLOAT3(-1.0f, 1.0f, 0.0f),		//åµ—æE
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//æœEE
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//åƒEEä¹?
		XMFLOAT2(0.0f,0.0f)					//åƒ¥åƒ‹åEåƒ å„åµ—æE
	},

	{//æ€æ? RIGHT-TOP
		XMFLOAT3(1.0f, 1.0f, 0.0f),		//åµ—æE
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//æœEE
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//åƒEEä¹?
		XMFLOAT2(1.0f,0.0f)					//åƒ¥åƒ‹åEåƒ å„åµ—æE
	},

	{//æ€æ? LEFT-BOTTOM
		XMFLOAT3(-1.0f, 0.0f, 0.0f),		//åµ—æE
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//æœEE
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//åƒEEä¹?
		XMFLOAT2(0.0f,1.0f)					//åƒ¥åƒ‹åEåƒ å„åµ—æE
	},

	{//æ€æ? RIGHT-BOTTOM
		XMFLOAT3(1.0f, 0.0f, 0.0f),		//åµ—æE
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//æœEE
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//åƒEEä¹?
		XMFLOAT2(1.0f,1.0f)					//åƒ¥åƒ‹åEåƒ å„åµ—æE
	},
};

//åƒ¾å„—åƒ€å„ä¹•æ‘‰å¨å‚æ•¾æ•åƒ’åƒ€åƒ?
static XMFLOAT3 g_SolidHalfSize_2d = XMFLOAT3(
	PLAYER2D_SOLID_HALF_X,
	PLAYER2D_SOLID_HALF_Y,
	PLAYER2D_SOLID_HALF_Z
);

static bool g_Player2DActive = false;

//=========================================================================================================
// å¼¶å©œå£”å¼µæ£?
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// åƒ¨åƒ¶åƒ€åƒ—åEåƒ¨åƒ¶åƒ€åƒ—åƒå„åƒ¥åƒ‰åEåƒ©åºæ›ææ
	g_pDevice = pDevice;
	g_pContext = pContext;

	// åƒ¥åƒ‹åEåƒ å„?
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\Texture\\player2d.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	//æ€æ°åƒ¶åƒ¢åƒ¼å‚½åºæƒ—æƒ‰
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));//0å±åƒ‹åEå‚¾
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * 4;//å¥¿æ“ºå±å’åEæ€æ°æ‚¢*æ€æ°åƒ’åƒ€åƒ?
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
// å»”æ¤E¼µæ£E
//=========================================================================================================
void Player2D_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_Texture);
}

//=========================================================================================================
// å³æ€´å¼µæ£?
//=========================================================================================================
void Player2D_Update()
{
	if (!g_Player2DActive) return;

	Player2D_Respawn();	//å„•åEå„E¹•åE

	Player2D_Gravity();	//å»³æ¤¡å¼µæ£E

	// åƒ¾å„—åƒ€å„ä¹•æEå¶E
	Player2D_Move();	//å æ‘¦
	Player2D_Jump();	//åƒ•å„å„åE
	Player2D_Change();	//å¡­æ›EE


	switch (g_Player2D.state)
	{
	case PLAYER_STATE_IDLE:
		//Idleå‚¾åƒ¯å„Šä¹•åƒ”å„‘åE
		break;
	case PLAYER_STATE_MOVE:
		//Moveå‚¾åƒ¯å„Šä¹•åƒ”å„‘åE
		break;
	case PLAYER_STATE_FALL:
		//Fallå‚¾åƒ¯å„Šä¹•åƒ”å„‘åE
		break;


	default:
		break;
	}
}

//=========================================================================================================
// åƒåƒ¢åƒä¹E
//=========================================================================================================
XMFLOAT3 GetPlayer2DPosition()
{
	return g_Player2D.Position;
}


//=========================================================================================================
// ƒZƒbƒ^[
//=========================================================================================================


//=========================================================================================================
// ˆ—
// „IÀE
//=========================================================================================================
void Player2D_Gravity()
{
	g_Player2D.blockMovement = false;

	bool wasGround = g_Player2D.isGround;
	g_Player2D.isGround = false;

	if (!wasGround)
	{
		g_Player2D.Velocity.y += g_Player2D.Acceleration.y;

		if (g_Player2D.Velocity.y < g_Player2D.gravityPower)
		{
			g_Player2D.Velocity.y = g_Player2D.gravityPower;
		}
	}
	else
	{
		// æ„™æŠ§æ‹å¼å£“æ›½å²¦åºæ‡æ™å‚ªå„•åEåƒ¢åƒ©ä¹®å¿‹æ›½å²¦å¼å«‹å£œä¹?
		if (g_Player2D.Velocity.y < 0.0f)
		{
			g_Player2D.Velocity.y = 0.0f;
		}
	}

	// X-Zæš¯æŸºåºæ¸å¶¤åµå‚›åEå°­æ‡E
	g_Player2D.Velocity.x *= 0.925f;
	g_Player2D.Velocity.z *= 0.925f;

	Player2DShadow_Collision();

	g_Player2D.Position.x += g_Player2D.Velocity.x;
	g_Player2D.Position.y += g_Player2D.Velocity.y;
	g_Player2D.Position.z += g_Player2D.Velocity.z;

	int hit = Player2DField_Collision();
	Player2DShadow_TopContact();

	if (g_Player2D.isGround && g_Player2D.Velocity.y < 0.0f)
	{
		g_Player2D.Velocity.y = 0.0f;
	}

	float len = g_Player2D.Velocity.x * g_Player2D.Velocity.x +
		g_Player2D.Velocity.y * g_Player2D.Velocity.y +
		g_Player2D.Velocity.z * g_Player2D.Velocity.z;

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
	else
	{
		g_StopTime = 0.0f;
	}
}

void Player2D_Respawn()
{
	// æ£Šå£“åƒ åƒEƒ¢åƒE
	if (g_Player2D.Position.y < -10.0f)
	{
		Player2D_Reset();
		return;
	}
	if (IsInputTrigger(ResetKey, gPad))
	{
		Player2D_Reset();
	}
}

void Player2D_Move()
{
	if (g_Player2D.blockMovement)
	{
		return;
	}

	XMFLOAT3 inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (IsInputPress(RightKey, gPad)) inputDir.x += +1.0f;
	if (IsInputPress(LeftKey, gPad))  inputDir.x += -1.0f;

	if (fabsf(inputDir.x) > 0.0001f)
	{
		// åƒ¾å„—åƒ€å„ä¹•åºYå¤æ®å¦æ™å‚ªåº¢æ‘¼ä¹®æš»åºå²¦å’å‚ªå¯›æ•ä¹¯
		// Rotation.y å¼æš»åºæœEEæ›½å²¦å‚ªå²¦åEEåEE
		float yawRad = XMConvertToRadians(g_Player2D.Rotation.y);

		// æš»åµå¢—åEå¨ä¹½å¡E›½å²¦ä¹¿å„€åƒ‹åƒ©å„–å‚ªå¯å¶¼
		// æœEEæ›½å²¦:  (sin(yaw), 0, cos(yaw))
		// å¡E›½å²? æœEEå‚?Yå¹‰å»EEåµ -90æ™å¤æE= (cos(yaw), 0, -sin(yaw))
		// å‚‘å¨å¼æ‰¨å¼®åµ:  å¡?= (cos(yaw), 0, -sin(yaw))
		float rightX = cosf(yawRad);
		float rightZ = -sinf(yawRad);

		// æ“–æ¤¡æ›½å²¦ä¹®åµå¡E¹¯å‚ªå„šä¹•åEåƒªåµ—æEåµæ›E§º
		float worldX = inputDir.x * rightX;
		float worldZ = inputDir.x * rightZ;

		// æ‡æ™åµå£›å¶¼ä¹®Xå¹‰åEZå¹‰ä¸„Yå¹‰å¼å»³æ¤¡å±æƒ‚å±¼ä¹¯
		g_Player2D.Velocity.x += worldX * g_Player2D.moveSpeed;
		g_Player2D.Velocity.z += worldZ * g_Player2D.moveSpeed;
	}

	// æ‡æ™æƒ‚å°·ä¹®X-Zæš¯æŸºä¹?
	float speedSq = g_Player2D.Velocity.x * g_Player2D.Velocity.x +
		g_Player2D.Velocity.z * g_Player2D.Velocity.z;
	float maxSpeed = g_Player2D.maxMoveSpeed;
	if (speedSq > maxSpeed * maxSpeed)
	{
		float speed = sqrtf(speedSq);
		g_Player2D.Velocity.x = (g_Player2D.Velocity.x / speed) * maxSpeed;
		g_Player2D.Velocity.z = (g_Player2D.Velocity.z / speed) * maxSpeed;
	}

	if (debugMode)
	{
		ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("pla2DMo.cpp"))
		{
			ImGui::Text("g_Player2DblockMovement: %s", g_Player2D.blockMovement ? "true" : "false");
			ImGui::Text("g_Player2DVelocityX: %.2f", g_Player2D.Velocity.x);
			ImGui::Text("g_Player2DVelocityZ: %.2f", g_Player2D.Velocity.z);
			ImGui::TreePop();
		}
		ImGui::End();
	}
}

void Player2D_Jump()
{
	if (IsInputTrigger(JumpKey, gPad))
	{
		if (g_Player2D.isGround)
		{
			//g_Player2D.Velocity.y += g_Player2D.jumpPower;//????????????^????
			// ??????????
			// g_Player2D.state = PLAYER2D_STATE_FALL;
		}
	}
}

void Player2D_Change()
{
	if (IsInputTrigger(ChangeKey, gPad))
	{
		// ¡ED¤Ë‰äÉE
		// 
		// 3D¥­¥ã¥é¥¯¥¿©`¤ED¥×¥E¤¥ä©`¤Î×ù˜Ë¤ò²ÎÕÕ¤·¤ÆÉú³É
		// ¡ı
		// 2D¥×¥E¤¥ä©`¤òÏ÷³ı¤¹¤E
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
// Ãè»­„IÀE
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

	// ‰ä“QĞĞÁĞ¤òí”µã¥·¥§©`¥À¤Ø¥»¥Ã¥È
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
