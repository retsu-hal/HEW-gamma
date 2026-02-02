//Player2D.cpp
#include "Player2D.h"
#include "PlayerStatus.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "sprite.h"

//=========================================================================================================
// 僨僶僢僌�?
#include "debug.h"
#include "MathUtil.h"
using namespace mu;

//=========================================================================================================
// 僨僶僢僌�?
//=========================================================================================================
#include "debug.h"

//=========================================================================================================
// 僌儘乕僶儖曄�?
//=========================================================================================================
PLAYER g_Player2D;
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static  ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11ShaderResourceView* g_Texture;		//僥僋僗僠儍曄�?

static float g_StopTime = 0.0f;
static bool debugMode = TRUE;


static Vertex3D Player2DVertex[4] = {
	{//捀�? LEFT-TOP
		XMFLOAT3(-1.0f, 1.0f, 0.0f),		//嵗昗
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朄慄
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僇儔�?
		XMFLOAT2(0.0f,0.0f)					//僥僋僗僠儍嵗�?
	},

	{//捀�? RIGHT-TOP
		XMFLOAT3(1.0f, 1.0f, 0.0f),		//嵗昗
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朄慄
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僇儔�?
		XMFLOAT2(1.0f,0.0f)					//僥僋僗僠儍嵗�?
	},

	{//捀�? LEFT-BOTTOM
		XMFLOAT3(-1.0f, 0.0f, 0.0f),		//嵗昗
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朄慄
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僇儔�?
		XMFLOAT2(0.0f,1.0f)					//僥僋僗僠儍嵗�?
	},

	{//捀�? RIGHT-BOTTOM
		XMFLOAT3(1.0f, 0.0f, 0.0f),		//嵗昗
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朄慄
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僇儔�?
		XMFLOAT2(1.0f,1.0f)					//僥僋僗僠儍嵗�?
	},
};

//僾儗僀儎乕摉偨傝敾掕僒僀�?
static XMFLOAT3 g_SolidHalfSize_2d = XMFLOAT3(
	PLAYER2D_SOLID_HALF_X,
	PLAYER2D_SOLID_HALF_Y,
	PLAYER2D_SOLID_HALF_Z
);

static bool g_Player2DActive = false;

//=========================================================================================================
// 弶婜壔張�?
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 僨僶僀僗偲僨僶僀僗僐儞僥僉僗僩偺曐懚
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 僥僋僗僠�?
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\Texture\\player2d.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	//捀揰僶僢僼傽偺惗惉
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));//0偱僋儕傾
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * 4;//奿擺偱偒傞捀揰悢*捀揰僒僀�?
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
// 廔椆張棟
//=========================================================================================================
void Player2D_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_Texture);
}

//=========================================================================================================
// 峏怴張�?
//=========================================================================================================
void Player2D_Update()
{
	if (!g_Player2DActive) return;

	Player2D_Respawn();	//儕僗億乕�?

	Player2D_Gravity();	//廳椡張棟

	// 僾儗僀儎乕憖嶌
	Player2D_Move();	//堏摦
	Player2D_Jump();	//僕儍儞僾
	Player2D_Change();	//塭曄�?


	switch (g_Player2D.state)
	{
	case PLAYER_STATE_IDLE:
		//Idle傾僯儊乕僔儑�?
		break;
	case PLAYER_STATE_MOVE:
		//Move傾僯儊乕僔儑�?
		break;
	case PLAYER_STATE_FALL:
		//Fall傾僯儊乕僔儑�?
		break;


	default:
		break;
	}
}

//=========================================================================================================
// 僎僢僞乕
//=========================================================================================================
XMFLOAT3 GetPlayer2DPosition()
{
	return g_Player2D.Position;
}


//=========================================================================================================
// �Z�b�^�[
//=========================================================================================================


//=========================================================================================================
// ����
// �I��
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
		// 愙抧拞偼壓曽岦偺懍搙傪儕僙僢僩乮忋曽岦偼嫋壜�?
		if (g_Player2D.Velocity.y < 0.0f)
		{
			g_Player2D.Velocity.y = 0.0f;
		}
	}

	// X-Z暯柺偺杸嶤偵傛傞尭懍
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
	static int s_FallFrameCount = 0;
	const int FALL_FRAME_THRESHOLD = 30; 

	if (g_Player2D.Position.y < -10.0f)
	{
		s_FallFrameCount++;

		if (s_FallFrameCount >= FALL_FRAME_THRESHOLD)
		{
			Player2D_Reset();
			s_FallFrameCount = 0;
			return;
		}
	}
	else
	{
		s_FallFrameCount = 0;
	}

	if (IsInputTrigger(ResetKey, gPad))
	{
		Player2D_Reset();
		s_FallFrameCount = 0;
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
		// 僾儗僀儎乕偺Y夞揮妏搙傪庢摼乮暻偺岦偒傪寛掕乯
		// Rotation.y 偼暻偺朄慄曽岦傪岦偄偰偄�?
		float yawRad = XMConvertToRadians(g_Player2D.Rotation.y);

		// 暻偵増偭偨乽塃曽岦乿儀僋僩儖傪寁嶼
		// 朄慄曽岦:  (sin(yaw), 0, cos(yaw))
		// 塃曽�? 朄慄�?Y幉廃傝偵 -90搙夞�?= (cos(yaw), 0, -sin(yaw))
		// 傑偨偼扨弮偵:  �?= (cos(yaw), 0, -sin(yaw))
		float rightX = cosf(yawRad);
		float rightZ = -sinf(yawRad);

		// 擖椡曽岦乮嵍塃乯傪儚乕儖僪嵗昗偵曄姺
		float worldX = inputDir.x * rightX;
		float worldZ = inputDir.x * rightZ;

		// 懍搙偵壛嶼乮X幉偲Z幉丄Y幉偼廳椡偱惂屼乯
		g_Player2D.Velocity.x += worldX * g_Player2D.moveSpeed;
		g_Player2D.Velocity.z += worldZ * g_Player2D.moveSpeed;
	}

	// 懍搙惂尷乮X-Z暯柺�?
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


static bool g_IsJumping = false;      
static bool g_JumpKeyReleased = true; 
static float g_JumpHoldTime = 0.0f;   
static float g_CoyoteTime = 0.0f;     
static float g_JumpBufferTime = 0.0f; 

static const float JUMP_INITIAL_VELOCITY = 0.18f;  
static const float JUMP_HOLD_BONUS = 0.008f;       
static const float JUMP_HOLD_MAX_TIME = 12.0f;     
static const float COYOTE_TIME_MAX = 6.0f;         
static const float JUMP_BUFFER_MAX = 8.0f;         
static const float JUMP_CUT_MULTIPLIER = 0.5f;


void Player2D_Jump()
{
	if (g_Player2D.isGround)
	{
		g_CoyoteTime = COYOTE_TIME_MAX;
		g_IsJumping = false;
	}
	else
	{
		if (g_CoyoteTime > 0.0f)
		{
			g_CoyoteTime -= 1.0f;
		}
	}

	if (g_JumpBufferTime > 0.0f)
	{
		g_JumpBufferTime -= 1.0f;
	}

	bool jumpPressed = IsInputPress(JumpKey, gPad);
	bool jumpTriggered = IsInputTrigger(JumpKey, gPad);

	if (jumpTriggered)
	{
		g_JumpBufferTime = JUMP_BUFFER_MAX;
	}

	bool canJump = (g_Player2D.isGround || g_CoyoteTime > 0.0f) && !g_IsJumping;
	bool wantsToJump = jumpTriggered || (g_JumpBufferTime > 0.0f && g_JumpKeyReleased);

	if (canJump && wantsToJump && g_JumpKeyReleased)
	{
		g_Player2D.Velocity.y = JUMP_INITIAL_VELOCITY;
		g_Player2D.isGround = false;
		g_IsJumping = true;
		g_JumpKeyReleased = false;
		g_JumpHoldTime = 0.0f;
		g_CoyoteTime = 0.0f;      
		g_JumpBufferTime = 0.0f;  

		g_Player2D.state = PLAYER_STATE_FALL;

		// Audio_PlaySE(SE_JUMP);
	}

	if (g_IsJumping && jumpPressed && g_Player2D.Velocity.y > 0.0f)
	{
		g_JumpHoldTime += 1.0f;

		if (g_JumpHoldTime <= JUMP_HOLD_MAX_TIME)
		{
			float holdBonus = JUMP_HOLD_BONUS * (1.0f - g_JumpHoldTime / JUMP_HOLD_MAX_TIME);
			g_Player2D.Velocity.y += holdBonus;
		}
	}

	if (g_IsJumping && !jumpPressed && g_Player2D.Velocity.y > 0.0f)
	{
		g_Player2D.Velocity.y *= JUMP_CUT_MULTIPLIER;
		g_IsJumping = false;
	}

	if (!jumpPressed)
	{
		g_JumpKeyReleased = true;
	}

}

void Player2D_Change()
{
	if (IsInputTrigger(ChangeKey, gPad))
	{
		// ��3D�ˉ���
		// 
		// 3D����饯���`��2D�ץ쥤��`�����ˤ���դ�������
		// ��
		// 2D�ץ쥤��`����������
		//
	}
}

void Player2D_ResetJumpState()
{
	g_IsJumping = false;
	g_JumpKeyReleased = true;
	g_JumpHoldTime = 0.0f;
	g_CoyoteTime = 0.0f;
	g_JumpBufferTime = 0.0f;
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

	Player2D_ResetJumpState();
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
// �軭�I��
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

	// ��Q���Ф�픵㥷���`���إ��å�
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
