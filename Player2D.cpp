//Player2D.cpp
#include "Player2D.h"
#include "PlayerStatus.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "sprite.h"
#include "MathUtil.h"
using namespace mu;


#include "debug.h"
static bool debugMode = TRUE;



PLAYER g_Player2D;
static ID3D11Device* g_pDevice = NULL;
static ID3D11DeviceContext* g_pContext = NULL;
static ID3D11Buffer* g_VertexBuffer = NULL;

static float g_StopTime = 0.0f;

static Player2DAnimDef g_AnimDefs[PLAYER2D_ANIM_MAX] = {
	//                  texturePath                               cols rows start count speed loop
	/* IDLE */ { L"asset\\Texture\\Player2D\\Taiki_2D.png",        5,   5,    0,    25,  1.5f, true  },

};

static ID3D11ShaderResourceView* g_AnimTextures[PLAYER2D_ANIM_MAX] = { NULL };

static PLAYER2D_ANIM g_CurrentAnim = PLAYER2D_ANIM_IDLE;
static int   g_AnimFrame = 0;
static float g_AnimTimer = 0.0f;
static bool  g_AnimFinished = false;
static bool  g_FacingRight = true;

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

static XMFLOAT3 g_SolidHalfSize_2d = XMFLOAT3(
	PLAYER2D_SOLID_HALF_X,
	PLAYER2D_SOLID_HALF_Y,
	PLAYER2D_SOLID_HALF_Z
);


void Player2D_SetAnim(PLAYER2D_ANIM anim)
{
	if (anim < 0 || anim >= PLAYER2D_ANIM_MAX) return;

	if (anim == g_CurrentAnim) return;

	if (g_AnimDefs[anim].cols <= 0 || g_AnimDefs[anim].rows <= 0)
		return;

	g_CurrentAnim = anim;
	g_AnimFrame = 0;
	g_AnimTimer = 0.0f;
	g_AnimFinished = false;
}

PLAYER2D_ANIM Player2D_GetAnim()
{
	return g_CurrentAnim;
}

static void Player2D_UpdateAnim()
{
	if (g_CurrentAnim < 0 || g_CurrentAnim >= PLAYER2D_ANIM_MAX)
	{
		g_CurrentAnim = PLAYER2D_ANIM_IDLE;
	}

	const Player2DAnimDef& def = g_AnimDefs[g_CurrentAnim];

	int frameCount = def.frameCount;
	if (frameCount <= 0) frameCount = 1;

	if (g_AnimFinished) return;

	g_AnimTimer += 1.0f;

	float speed = def.frameSpeed;
	if (speed <= 0.0f) speed = 1.0f;

	if (g_AnimTimer >= speed)
	{
		g_AnimTimer = 0.0f;
		g_AnimFrame++;

		if (g_AnimFrame >= frameCount)
		{
			if (def.loop)
			{
				g_AnimFrame = 0;
			}
			else
			{
				g_AnimFrame = frameCount - 1;
				g_AnimFinished = true;
			}
		}
	}

	if (g_Player2D.state == PLAYER_STATE_FALL && !g_Player2D.isGround)
	{
		if (g_Player2D.Velocity.y > 0.0f)
			Player2D_SetAnim(PLAYER2D_ANIM_JUMP);
		else
			Player2D_SetAnim(PLAYER2D_ANIM_FALL);
	}
	else if (g_Player2D.isGround)
	{
		float speedSq = g_Player2D.Velocity.x * g_Player2D.Velocity.x +
			g_Player2D.Velocity.z * g_Player2D.Velocity.z;

		if (speedSq > 0.0001f)
			Player2D_SetAnim(PLAYER2D_ANIM_WALK);
		else
			Player2D_SetAnim(PLAYER2D_ANIM_IDLE);
	}

	float yawRad = XMConvertToRadians(g_Player2D.Rotation.y);
	float rightX = cosf(yawRad);
	float rightZ = -sinf(yawRad);
	float rightDot = g_Player2D.Velocity.x * rightX + g_Player2D.Velocity.z * rightZ;

	if (rightDot > 0.001f)
		g_FacingRight = true;
	else if (rightDot < -0.001f)
		g_FacingRight = false;
}

static void Player2D_UpdateUV()
{
	if (g_CurrentAnim < 0 || g_CurrentAnim >= PLAYER2D_ANIM_MAX)
	{
		g_CurrentAnim = PLAYER2D_ANIM_IDLE;
	}

	const Player2DAnimDef& def = g_AnimDefs[g_CurrentAnim];

	int cols = def.cols;
	int rows = def.rows;
	if (cols <= 0) cols = 1;
	if (rows <= 0) rows = 1;

	float uvW = 1.0f / cols;
	float uvH = 1.0f / rows;


	int frameCount = def.frameCount;
	if (frameCount <= 0) frameCount = 1;

	int animFrame = g_AnimFrame;
	if (animFrame < 0) animFrame = 0;
	if (animFrame >= frameCount) animFrame = frameCount - 1;

	int sheetFrame = def.startFrame + animFrame;

	int totalFrames = cols * rows;
	if (totalFrames <= 0) totalFrames = 1;
	if (sheetFrame >= totalFrames) sheetFrame = totalFrames - 1;
	if (sheetFrame < 0) sheetFrame = 0;

	int col = sheetFrame % cols;
	int row = sheetFrame / cols;

	float u0 = col * uvW;
	float v0 = row * uvH;
	float u1 = u0 + uvW;
	float v1 = v0 + uvH;

	if (!g_FacingRight)
	{
		float temp = u0;
		u0 = u1;
		u1 = temp;
	}

	Player2DVertex[0].texCoord = XMFLOAT2(u0, v0);
	Player2DVertex[1].texCoord = XMFLOAT2(u1, v0);
	Player2DVertex[2].texCoord = XMFLOAT2(u0, v1);
	Player2DVertex[3].texCoord = XMFLOAT2(u1, v1);
}





//=========================================================================================================
// 蠑ｶ蟀懷｣泌ｼｵ譽?
//=========================================================================================================
void Player2D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// 蜒ｨ蜒ｶ蜒蜒怜・蜒ｨ蜒ｶ蜒蜒怜ヰ蜆槫Η蜒牙・蜒ｩ蛛ｺ譖先∵
	g_pDevice = pDevice;
	g_pContext = pContext;

	for (int i = 0; i < PLAYER2D_ANIM_MAX; i++)
	{
		if (g_AnimDefs[i].texturePath == NULL) continue;

		TexMetadata metadata;
		ScratchImage image;
		HRESULT hr = LoadFromWICFile(g_AnimDefs[i].texturePath, WIC_FLAGS_NONE, &metadata, image);

		if (SUCCEEDED(hr))
		{
			CreateShaderResourceView(pDevice, image.GetImages(),
				image.GetImageCount(), metadata, &g_AnimTextures[i]);
		}
		else
		{
			char msg[256];
			sprintf_s(msg, "Warning: Failed to load texture for anim %d\n", i);
			OutputDebugStringA(msg);
			g_AnimTextures[i] = NULL;
		}
	}

	for (int i = 0; i < PLAYER2D_ANIM_MAX; i++)
	{
		if (g_AnimTextures[i] == NULL && g_AnimTextures[PLAYER2D_ANIM_IDLE] != NULL)
		{
			g_AnimTextures[i] = g_AnimTextures[PLAYER2D_ANIM_IDLE];
			g_AnimTextures[i]->AddRef();
		}
	}

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
	for (int i = 0; i < PLAYER2D_ANIM_MAX; i++)
	{
		if (g_AnimTextures[i] != NULL)
		{
			for (int j = i + 1; j < PLAYER2D_ANIM_MAX; j++)
			{
				if (g_AnimTextures[j] == g_AnimTextures[i])
				{
					g_AnimTextures[j] = NULL;
				}
			}
			g_AnimTextures[i]->Release();
			g_AnimTextures[i] = NULL;
		}
	}
}

//=========================================================================================================
// 蟲乗ｴ蠑ｵ譽?
//=========================================================================================================
void Player2D_Update()
{
	if (!g_Player2D.Active) return;

	

	Player2D_Gravity();

	if (g_Player2D.Position.y < -10.0f)
	{
		Player2D_Respawn();	
	}

	Player2D_Move();
	Player2D_Jump();


	Player2D_UpdateAnim();
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

	if (g_Player2D.isGround)
	{
		if (g_Player2D.state == PLAYER_STATE_FALL)
		{
			g_Player2D.state = PLAYER_STATE_MOVE;
		}
	}
	else
	{
		if (g_Player2D.state != PLAYER_STATE_FALL)
		{
			g_Player2D.state = PLAYER_STATE_FALL;
		}
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

	g_IsJumping = false;
	g_JumpKeyReleased = true;
	g_JumpHoldTime = 0.0f;
	g_CoyoteTime = 0.0f;
	g_JumpBufferTime = 0.0f;

	g_CurrentAnim = PLAYER2D_ANIM_IDLE;
	g_AnimFrame = 0;
	g_AnimTimer = 0.0f;
	g_AnimFinished = false;
	g_FacingRight = true;
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

	if (!g_Player2D.Active) return;

	ID3D11ShaderResourceView* currentTexture = g_AnimTextures[g_CurrentAnim];
	if (!currentTexture) return;

	Player2D_UpdateUV();

	D3D11_MAPPED_SUBRESOURCE msr;
	g_pContext->Map(g_VertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	Vertex3D* vertex = (Vertex3D*)msr.pData;
	CopyMemory(&vertex[0], &Player2DVertex[0], sizeof(Vertex3D) * 4);
	g_pContext->Unmap(g_VertexBuffer, 0);

	XMMATRIX scale = XMMatrixScaling
	(
		g_Player2D.Scaling.x,
		g_Player2D.Scaling.y,
		g_Player2D.Scaling.z
	);

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
	g_pContext->PSSetShaderResources(0, 1, &currentTexture);
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

	g_Player2D.Active = true;
}

void Player2D_Uninit()
{
	g_Player2D.Active = false;

	g_Player2D.Velocity = XMFLOAT3(0, 0, 0);
	g_Player2D.state = PLAYER_STATE_IDLE;
	g_StopTime = 0.0f;
}

void Player2D_SetActive(bool active)
{
	g_Player2D.Active = active;
}
