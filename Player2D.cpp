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

//=========================================================================================================
// 僌�E乕僶儖曄�?
//=========================================================================================================
PLAYER g_Player2D;
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static  ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11ShaderResourceView* g_Texture;		//僥僋�E僠儍曄�?

static float g_StopTime = 0.0f;
static bool debugMode;


static Vertex3D Player2DVertex[4] = {
	{//捀�? LEFT-TOP
		XMFLOAT3(-1.0f, 1.0f, 0.0f),		//嵗�E
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朁E�E
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僁E�E�?
		XMFLOAT2(0.0f,0.0f)					//僥僋�E僠儍嵗�E
	},

	{//捀�? RIGHT-TOP
		XMFLOAT3(1.0f, 1.0f, 0.0f),		//嵗�E
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朁E�E
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僁E�E�?
		XMFLOAT2(1.0f,0.0f)					//僥僋�E僠儍嵗�E
	},

	{//捀�? LEFT-BOTTOM
		XMFLOAT3(-1.0f, 0.0f, 0.0f),		//嵗�E
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朁E�E
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僁E�E�?
		XMFLOAT2(0.0f,1.0f)					//僥僋�E僠儍嵗�E
	},

	{//捀�? RIGHT-BOTTOM
		XMFLOAT3(1.0f, 0.0f, 0.0f),		//嵗�E
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//朁E�E
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//僁E�E�?
		XMFLOAT2(1.0f,1.0f)					//僥僋�E僠儍嵗�E
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
	// 僨僶僀僗�E僨僶僀僗僐儞僥僉�E僩偺曐��
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 僥僋�E僠�?
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\Texture\\player2d.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	//捀揰僶僢僼傽偺惗惉
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));//0偱僋�E傾
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * 4;//奿擺偱偒�E捀揰悢*捀揰僒僀�?
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
// 廔椁E��棁E
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
	if (!g_Player2D.Active) return;

	

	Player2D_Gravity();	//廳椡張棁E

	if (g_Player2D.Position.y < -10.0f)
	{
		Player2D_Respawn();	
	}

	Player2D_Move();
	switch (g_Player2D.state)
	{
	case PLAYER_STATE_IDLE:

		break;
	case PLAYER_STATE_MOVE:
		
		break;
	case PLAYER_STATE_FALL:

		break;
	case PLAYER_STATE_JUMP:
		Player2D_Jump();
		break;


	default:
		break;
	}
}

//=========================================================================================================
// 僎僢僞乁E
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
// �I��E
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
		// 愙抧拞偼壓曽岦偺懍搙傪儕�E僢僩乮忋曽岦偼嫋壜�?
		if (g_Player2D.Velocity.y < 0.0f)
		{
			g_Player2D.Velocity.y = 0.0f;
		}
	}

	// X-Z暯柺偺杸嶤偵傛�E尭懁E
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
	// 棊壓僠僁E��僁E
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
		// 僾儗僀儎乕偺Y夞揮妏搙傪庢摼乮暻偺岦偒傪寛掕乯
		// Rotation.y 偼暻偺朁E�E曽岦傪岦偁E�E偁E�E
		float yawRad = XMConvertToRadians(g_Player2D.Rotation.y);

		// 暻偵増�E偨乽塁E��岦乿儀僋僩儖傪寁嶼
		// 朁E�E曽岦:  (sin(yaw), 0, cos(yaw))
		// 塁E���? 朁E�E�?Y幉廁E�E偵 -90搙夞�E= (cos(yaw), 0, -sin(yaw))
		// 傑偨偼扨弮偵:  �?= (cos(yaw), 0, -sin(yaw))
		float rightX = cosf(yawRad);
		float rightZ = -sinf(yawRad);

		// 擖椡曽岦乮嵍塁E��傪儚乕�E僪嵗�E偵曁E��
		float worldX = inputDir.x * rightX;
		float worldZ = inputDir.x * rightZ;

		// 懍搙偵壛嶼乮X幉�EZ幉丄Y幉偼廳椡偱惂屼乯
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
// �軭�I��E
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
