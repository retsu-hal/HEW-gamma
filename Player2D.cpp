//Player2D.cpp
#include "Player2D.h"
#include "PlayerStatus.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "sprite.h"

//=========================================================================================================
// 蜒ｨ蜒ｶ蜒｢蜒梧｢?
#include "debug.h"
#include "MathUtil.h"
using namespace mu;

//=========================================================================================================
// 蜒ｨ蜒ｶ蜒｢蜒梧｢?
//=========================================================================================================

//=========================================================================================================
// 蜒悟・荵募Ω蜆匁寇謔?
//=========================================================================================================
PLAYER g_Player2D;
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static  ID3D11Buffer* g_VertexBuffer = NULL;
static ID3D11ShaderResourceView* g_Texture;		//蜒･蜒句・蜒蜆肴寇謔?

static float g_StopTime = 0.0f;
static bool debugMode;


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

static Vertex3D Player2DVertex[4] = {
	{//謐謠? LEFT-TOP
		XMFLOAT3(-1.0f, 1.0f, 0.0f),		//蠏玲・
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//譛・・
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//蜒・・荵?
		XMFLOAT2(0.0f,0.0f)					//蜒･蜒句・蜒蜆榊ｵ玲・
	},

	{//謐謠? RIGHT-TOP
		XMFLOAT3(1.0f, 1.0f, 0.0f),		//蠏玲・
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//譛・・
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//蜒・・荵?
		XMFLOAT2(1.0f,0.0f)					//蜒･蜒句・蜒蜆榊ｵ玲・
	},

	{//謐謠? LEFT-BOTTOM
		XMFLOAT3(-1.0f, 0.0f, 0.0f),		//蠏玲・
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//譛・・
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//蜒・・荵?
		XMFLOAT2(0.0f,1.0f)					//蜒･蜒句・蜒蜆榊ｵ玲・
	},

	{//謐謠? RIGHT-BOTTOM
		XMFLOAT3(1.0f, 0.0f, 0.0f),		//蠏玲・
		XMFLOAT3(0.0f, 1.0f, 0.0f),			//譛・・
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),	//蜒・・荵?
		XMFLOAT2(1.0f,1.0f)					//蜒･蜒句・蜒蜆榊ｵ玲・
	},
};

//蜒ｾ蜆怜ム蜆惹ｹ墓痩蛛ｨ蛯晄弊謗募ヲ蜒蜒?
static XMFLOAT3 g_SolidHalfSize_2d = XMFLOAT3(
	PLAYER2D_SOLID_HALF_X,
	PLAYER2D_SOLID_HALF_Y,
	PLAYER2D_SOLID_HALF_Z
);

static bool g_Player2DActive = false;

//=========================================================================================================
// 蠑ｶ蟀懷｣泌ｼｵ譽?
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 蜒ｨ蜒ｶ蜒蜒怜・蜒ｨ蜒ｶ蜒蜒怜ヰ蜆槫Η蜒牙・蜒ｩ蛛ｺ譖先∵
	g_pDevice = pDevice;
	g_pContext = pContext;

	// 蜒･蜒句・蜒蜆?
	TexMetadata metadata;
	ScratchImage image;
	LoadFromWICFile(L"asset\\Texture\\player2d.png", WIC_FLAGS_NONE, &metadata, image);
	CreateShaderResourceView(pDevice, image.GetImages(), image.GetImageCount(), metadata, &g_Texture);
	assert(g_Texture);

	//謐謠ｰ蜒ｶ蜒｢蜒ｼ蛯ｽ蛛ｺ諠玲ラ
	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(bd));//0蛛ｱ蜒句・蛯ｾ
	bd.Usage = D3D11_USAGE_DYNAMIC;
	bd.ByteWidth = sizeof(Vertex3D) * 4;//螂ｿ謫ｺ蛛ｱ蛛貞・謐謠ｰ謔｢*謐謠ｰ蜒貞ム蜒?
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
// 蟒疲､・ｼｵ譽・
//=========================================================================================================
void Player2D_Finalize(void)
{
	SAFE_RELEASE(g_VertexBuffer);
	SAFE_RELEASE(g_Texture);
}

//=========================================================================================================
// 蟲乗ｴ蠑ｵ譽?
//=========================================================================================================
void Player2D_Update()
{
	if (!g_Player2D.Active) return;

	

	Player2D_Gravity();	//蟒ｳ讀｡蠑ｵ譽・

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
// 蜒主Δ蜒樔ｹ・
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
// Иﾀ・
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
		// 諢呎刊諡槫⊂螢捺嵜蟯ｦ蛛ｺ諛肴杉蛯ｪ蜆募・蜒｢蜒ｩ荵ｮ蠢区嵜蟯ｦ蛛ｼ雖句｣應ｹ?
		if (g_Player2D.Velocity.y < 0.0f)
		{
			g_Player2D.Velocity.y = 0.0f;
		}
	}

	// X-Z證ｯ譟ｺ蛛ｺ譚ｸ蠍､蛛ｵ蛯帛・蟆ｭ諛・
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
	// 譽雁｣灘Β蜒・Δ蜒・
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
		// 蜒ｾ蜆怜ム蜆惹ｹ募⊆Y螟樊尚螯乗杉蛯ｪ蠎｢鞫ｼ荵ｮ證ｻ蛛ｺ蟯ｦ蛛貞が蟇帶歯荵ｯ
		// Rotation.y 蛛ｼ證ｻ蛛ｺ譛・・譖ｽ蟯ｦ蛯ｪ蟯ｦ蛛・・蛛・・
		float yawRad = XMConvertToRadians(g_Player2D.Rotation.y);

		// 證ｻ蛛ｵ蠅怜・蛛ｨ荵ｽ蝪・嵜蟯ｦ荵ｿ蜆蜒句Λ蜆門が蟇∝ｶｼ
		// 譛・・譖ｽ蟯ｦ:  (sin(yaw), 0, cos(yaw))
		// 蝪・嵜蟯? 譛・・蛯?Y蟷牙ｻ・・蛛ｵ -90謳吝､樊・= (cos(yaw), 0, -sin(yaw))
		// 蛯大→蛛ｼ謇ｨ蠑ｮ蛛ｵ:  蝪?= (cos(yaw), 0, -sin(yaw))
		float rightX = cosf(yawRad);
		float rightZ = -sinf(yawRad);

		// 謫匁､｡譖ｽ蟯ｦ荵ｮ蠏榊｡・ｹｯ蛯ｪ蜆壻ｹ募・蜒ｪ蠏玲・蛛ｵ譖・ｧｺ
		float worldX = inputDir.x * rightX;
		float worldZ = inputDir.x * rightZ;

		// 諛肴杉蛛ｵ螢帛ｶｼ荵ｮX蟷牙・Z蟷我ｸШ蟷牙⊂蟒ｳ讀｡蛛ｱ諠ょｱｼ荵ｯ
		g_Player2D.Velocity.x += worldX * g_Player2D.moveSpeed;
		g_Player2D.Velocity.z += worldZ * g_Player2D.moveSpeed;
	}

	// 諛肴杉諠ょｰｷ荵ｮX-Z證ｯ譟ｺ荵?
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
// ﾃ霆ｭИﾀ・
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

	// 我轍ﾐﾐﾁﾐ､鳩罕ｷ･ｧｩ`･ﾀ､ﾘ･ｻ･ﾃ･ﾈ
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
