#include	"title_manager.h"
#include	"sprite.h"
#include	"keyboard.h"
#include	"Player3D.h"
#include	"Player2D.h"
#include	"LightSource.h"
#include	"field.h"
#include	"Audio.h"
#include	"camera.h"
#include	"Collision.h"
#include	"PlayerModeSwitchManager.h"
#include	"ShadowColliderBox.h"
#include	"fade.h"
#include	"manager.h"
#include	"SkyDome.h"
#include "newKeyBind.h"

#include	 <map>

LIGHTOBJECT g_TitleLight;

static ID3D11Device* g_pDevice = nullptr;
static ID3D11DeviceContext* g_pContext = nullptr;

static std::map<int, ShadowPrism> g_ShadowPrisms;
static ShadowBuildConfig g_ShadowConfig;

static std::vector<const ShadowPrism*> g_ActiveShadowPrisms;

// ===== Title Action: Press Enter -> turn left & walk straight =====
static bool g_TitleActionStarted = false;
static int  g_TitleActionTimer = 0;

static const int kTitleTurnDuration = 30;    // ~0.5 seconds to turn left
static const int kTitleWalkDuration = 240;   // ~3 seconds walking before transition

static float g_TitleTargetYaw = 0.0f;        // target yaw after turning left
static float g_TitleStartYaw = 0.0f;         // yaw when action started

static const int kTitleStopDuration = 45;   // ~0.75 sec pause
static const int kTitleFadeDuration = 90;   // ~1.5 sec fade

static void TitleAction_Start()
{
	PLAYER* p = GetPlayer3D();
	if (!p) return;

	g_TitleActionStarted = true;
	g_TitleActionTimer = 0;

	// Remember current yaw, target is 90 degrees to the left
	g_TitleStartYaw = p->Rotation.y;
	XMMATRIX view = GetViewMatrix();

	XMMATRIX invView = XMMatrixInverse(nullptr, view);

	XMFLOAT4X4 invViewMat;
	XMStoreFloat4x4(&invViewMat, invView);

	// Camera right vector (world space)
	XMFLOAT3 camRight(
		invViewMat._11,
		invViewMat._21,
		invViewMat._31
	);

	// Screen-left
	float targetYawRad = atan2f(camRight.x, camRight.z);
	g_TitleTargetYaw = XMConvertToDegrees(targetYawRad) + 180.0f;

	// Block player input
	p->blockMovement = true;

	// Set walking animation
	p->isAuto = true;
	p->CurrentAnimIndex = PLAYER_ANIM_WALK;
	p->state = PLAYER_STATE_MOVE;
}

static void TitleAction_Update()
{
	if (!g_TitleActionStarted) return;

	PLAYER* p = GetPlayer3D();
	if (!p) return;

	g_TitleActionTimer++;

	// Keep player input blocked
	p->blockMovement = true;

	if (g_TitleActionTimer <= kTitleTurnDuration)
	{
		// Phase 1: Smoothly turn left over kTitleTurnDuration frames
		float t = (float)g_TitleActionTimer / (float)kTitleTurnDuration;
		// Ease-out interpolation for smoother rotation
		t = 1.0f - (1.0f - t) * (1.0f - t);
		p->Rotation.y = g_TitleStartYaw + (g_TitleTargetYaw - g_TitleStartYaw) * t;

		// Keep walk animation playing
		p->CurrentAnimIndex = PLAYER_ANIM_WALK;
	}
	else
	{
		// Phase 2: Walk straight in the direction the player is now facing
		p->Rotation.y = g_TitleTargetYaw;

		// Calculate forward direction based on current yaw
		// The model faces -Z by default with 180 offset, so forward = (sin(yaw), 0, cos(yaw))
		// But since Player3D uses yaw with a 180 degree offset for the model,
		// we compute the actual movement direction:
		float yawRad = XMConvertToRadians(p->Rotation.y - 180.0f);
		float fwdX = sinf(yawRad);
		float fwdZ = cosf(yawRad);

		// Apply walk velocity
		float walkSpeed = p->moveSpeed * 6.0f;
		p->Velocity.x = fwdX * walkSpeed;
		p->Velocity.z = fwdZ * walkSpeed;

		// Keep walk animation
		p->isAuto = true;
		p->CurrentAnimIndex = PLAYER_ANIM_WALK;

		// After walk duration, trigger scene transition
		int walkFrame = g_TitleActionTimer - kTitleTurnDuration;
		if (walkFrame < kTitleWalkDuration)
		{
			// keep walking
		}
		else
		{
			// ---- Phase 3: stop & hold ----
			p->Velocity.x = 0.0f;
			p->Velocity.z = 0.0f;
			p->isAuto = false;

			int stopFrame = walkFrame - kTitleWalkDuration;

			// ---- Phase 4: fade out ----
			if (stopFrame >= kTitleStopDuration)
			{
				if (GetFadeState() == FADE_NONE)
				{
					XMFLOAT4 color(0, 0, 0, 1);
					SetFade(kTitleFadeDuration, color, FADE_OUT, SCENE_GAME);
					SetCurrentStage(STAGE_SELECT);
				}
			}
		}
	}
}

void Title_Manager_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

	// Reset title action state
	g_TitleActionStarted = false;
	g_TitleActionTimer = 0;

	PlayerModeSwitchManager_Init();
	Player3D_Initialize(pDevice, pContext);
	Player2D_Initialize(pDevice, pContext);

	field_Initialize(pDevice, pContext);
	Light_Initialize(pDevice, pContext);
	SkyDome_Initialize(pDevice, pContext);
	Camera_Initialize();

	g_TitleLight.SetEnable(false);
	g_TitleLight.SetAmbient(XMFLOAT4(0.08f, 0.08f, 0.1f, 1.0f));
	g_TitleLight.SetDiffuse(XMFLOAT4(1.0f, 0.95f, 0.85f, 1.0f));

	g_ShadowConfig.edgeSamples = 4;
	g_ShadowConfig.thickness = 1.0f;
	g_ShadowConfig.maxCastDist = 100.0f;

	// グローバル変数を直接定義せず、Player3D のエクスポート関数を通じてフラグを設定
	PLAYER* p = GetPlayer3D();
	if (p) p->isTitleScene = true;
}

void Title_Manager_Finalize()
{
	// Reset title action state on finalize
	g_TitleActionStarted = false;
	g_TitleActionTimer = 0;

	g_ShadowPrisms.clear();
	g_ActiveShadowPrisms.clear();

	field_Finalize();
	Light_Finalize();
	SkyDome_Finalize();
	Camera_Finalize();

	Player3D_Finalize();
	Player2D_Finalize();
}

void Title_Manager_Update()
{
	Title_Camera_Update();
	Light_Update();
	field_Update();
	SkyDome_Update();

	// Check for Enter key to start title action (only if not already started)
	if (!g_TitleActionStarted)
	{
		if(IsInputTrigger(EnterKey, gPad))
		{
			TitleAction_Start();
			PLAYER* p = GetPlayer3D();
			if (p) p->isTitleScene = false;
		}

		// Normal mode switching only when action hasn't started
		PlayerModeSwitchManager_Update();
	}

	// Update the title action (turn + walk)
	TitleAction_Update();

	XMFLOAT3 LightPos = GetLight_Position();
	g_TitleLight.SetDirection(XMFLOAT4(LightPos.x, LightPos.y, LightPos.z, 1.0f));
	g_ShadowLightPos = LightPos;

	float shadowIntensity = 1.0f;
	Shader_SetShadowLightData(LightPos, g_ShadowRadius, shadowIntensity);

	auto& map = GetFieldMap();

	g_ActiveShadowPrisms.clear();
	std::vector<int> casterIndices;
	for (int i = 0; i < (int)map.size(); ++i)
	{
		if (map[i].no == FIELD_OBJ_2)
		{
			casterIndices.push_back(i);
		}
	}

	std::vector<int> keysToRemove;
	for (auto& pair : g_ShadowPrisms)
	{
		bool found = false;
		for (int idx : casterIndices)
		{
			if (pair.first == idx)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			keysToRemove.push_back(pair.first);
		}
	}
	for (int key : keysToRemove)
	{
		g_ShadowPrisms.erase(key);
	}

	for (int casterIdx : casterIndices)
	{
		if (g_ShadowPrisms.find(casterIdx) == g_ShadowPrisms.end())
		{
			g_ShadowPrisms[casterIdx] = ShadowPrism();
		}

		ShadowPrism& prism = g_ShadowPrisms[casterIdx];
		const MAPDATA& caster = map[casterIdx];

		bool needsRebuild = Shadow_NeedsRebuild(prism, LightPos, caster, 0.01f);

		if (needsRebuild)
		{
			bool buildSuccess = Shadow_Build(
				prism,
				caster,
				LightPos,
				map,
				g_ShadowConfig
			);

			if (!buildSuccess)
			{
				prism.isValid = false;
			}
		}

		if (prism.isValid)
		{
			g_ActiveShadowPrisms.push_back(&prism);
		}
	}

	Collision_SetShadowPrisms(g_ActiveShadowPrisms);

	if (PlayerModeSwitchManager_GetMode() == MODE_3D)
	{
		Player3D_Update();
	}
	else
	{
		Player2D_Update();
	}
}

void Title_Manager_Draw()
{
	Camera_Draw();
	Light_Draw();

	g_TitleLight.SetEnable(TRUE);
	Shader_SetLight(g_TitleLight.Light);
	SetDepthTest(TRUE);

	XMFLOAT3 lightPos = GetLight_Position();
	float lightRadius = g_ShadowRadius;

	for (int face = 0; face < 6; face++)
	{
		Direct3D_BeginShadowPass(face);
		Shader_Begin();
		Shader_SetShadowLightData(lightPos, lightRadius, 1.0f, 1.0f);

		XMMATRIX lightViewProj = Direct3D_GetCubemapFaceViewProj(face, lightPos, lightRadius);
		Shader_SetShadowMatrix(lightViewProj);

		{
			XMMATRIX world = Light_GetWorldMatrix();
			Light_DrawRaw(world, world * lightViewProj);
		}

		{
			std::vector<MAPDATA>& Map = GetFieldMap();
			float maxShadowDist = g_ShadowRadius;

			for (size_t i = 0; i < Map.size(); ++i)
			{
				XMVECTOR v = XMLoadFloat3(&Map[i].pos) - XMLoadFloat3(&lightPos);
				if (XMVectorGetX(XMVector3LengthSq(v)) > maxShadowDist * maxShadowDist)
					continue;

				XMMATRIX world = Field_GetWorldMatrix((int)i);
				Field_DrawShadowMap(world, world * lightViewProj, (int)i);
			}
		}
	}

	Direct3D_EndShadowPass();

	Shader_Begin();
	Shader_SetLight(g_TitleLight.Light);
	Shader_SetShadowMap(g_pShadowCubemapSRV);
	Shader_SetShadowSampler(g_pShadowSamplerState);
	Shader_SetShadowLightData(lightPos, lightRadius, 1.0f, 0.0f);

	{
		field_Draw();
	}
	{
		Light_Draw();
	}
	{
		SkyDome_Draw();
	}

	if (PlayerModeSwitchManager_GetMode() == MODE_3D)
	{
		Player3D_Draw();
	}
	else
	{
		Player2D_Draw();
	}

	g_TitleLight.SetEnable(FALSE);
	Shader_SetLight(g_TitleLight.Light);

	SetDepthTest(FALSE);
}