#include "player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
//=========================================================================================================
// 繝槭け繝ｭ螳夂ｾｩ
//=========================================================================================================
#define PLAYER3D_SPEEDMAX (2.0f)		//譛螟ｧ騾溷ｺｦ

//=========================================================================================================
// 繧ｰ繝ｭ繝ｼ繝舌Ν螟画焚
//=========================================================================================================
PLAYER3D g_player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;

float g_StopTime = 0.0f;

<<<<<<< HEAD
// 入力ベクトル
XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

//リセット用
=======
//繝ｪ繧ｻ繝・ヨ逕ｨ
>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a
XMFLOAT3		Firstposition;
XMFLOAT3		FirstRotation;
XMFLOAT3		FirstScaling;
XMFLOAT3		FirstVelocity;
XMFLOAT3		FirstAcceleration;
PLAYER3D_STATE	FirstState;
float			FirstStopTime;
XMVECTOR		FirstQuaternion;

<<<<<<< HEAD
//プレイヤーステータス
float moveSpeed =		 0.005f;			//移動速度
float maxMoveSpeed =	 1.0f;				//最大移動速度
float maxGravity =		-0.25f;				//最大落下速度
float jumpPower =		 0.175f;			//ジャンプ力
bool isGround = false;						//接地判定（明示的に初期化）

//キーボード定義
//移動
static const auto UpKey =		KK_W;		//前進
static const auto RightKey =	KK_D;		//右移動
static const auto DownKey =		KK_S;		//後退
static const auto LeftKey =		KK_A;		//左移動
//行動
static const auto JumpKey =		KK_SPACE;	//ジャンプ
static const auto ActionKey =	KK_F;		//アクション
static const auto ChangeKey =	KK_F;		//影変身
//その他
static const auto ResetKey =	KK_R;		//リセット
static const auto MenuKey =		KK_ESCAPE;	//終了

//繝励Ξ繧､繝､繝ｼ繧ｹ繝・・繧ｿ繧ｹ
float moveSpeed = 0.005f;				//遘ｻ蜍暮溷ｺｦ
float maxMoveSpeed = 1.0f;				//譛螟ｧ遘ｻ蜍暮溷ｺｦ
float maxGravity = -0.25f;				//譛螟ｧ關ｽ荳矩溷ｺｦ
float jumpPower = 0.25f;				//繧ｸ繝｣繝ｳ繝怜鴨
bool isGround = false;					//謗･蝨ｰ蛻､螳・

//繧ｭ繝ｼ繝懊・繝牙ｮ夂ｾｩ
//遘ｻ蜍・
static const auto UpKey = KK_W;			//蜑埼ｲ
static const auto RightKey = KK_D;		//蜿ｳ遘ｻ蜍・
static const auto DownKey = KK_S;		//蠕碁
static const auto LeftKey = KK_A;		//蟾ｦ遘ｻ蜍・
//陦悟虚
static const auto JumpKey = KK_SPACE;	//繧ｸ繝｣繝ｳ繝・
static const auto ActionKey = KK_F;		//繧｢繧ｯ繧ｷ繝ｧ繝ｳ
static const auto ChangeKey = KK_F;		//蠖ｱ螟芽ｺｫ
//縺昴・莉・
static const auto ResetKey = KK_R;		//繝ｪ繧ｻ繝・ヨ
static const auto MenuKey = KK_ESCAPE;	//邨ゆｺ・
=======
float g_StopTime=0.0f;

>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a

//=========================================================================================================
// 蛻晄悄蛹門・逅・
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	g_pDevice = pDevice;
	g_pContext = pContext;

<<<<<<< HEAD
	g_Player3D.Model = ModelLoad("asset\\model\\Test_man_stand.fbx");

	Firstposition		= g_Player3D.Position		= XMFLOAT3(0.0f, 1.2f, 0.0f);
	FirstRotation		= g_Player3D.Rotation		= XMFLOAT3(-90.0f, 180.0f, 0.0f);
	FirstScaling		= g_Player3D.Scaling		= XMFLOAT3(0.01f, 0.01f, 0.01f);
	FirstVelocity		= g_Player3D.Velocity		= XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstAcceleration	= g_Player3D.Acceleration	= XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	FirstState			= g_Player3D.state			= PLAYER3D_STATE_MOVE;
	FirstStopTime		= g_StopTime				= 0.0f;
	FirstQuaternion		= g_Player3D.Quaternion		= XMQuaternionIdentity();
	
=======
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
>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a
}

//=========================================================================================================
// 邨ゆｺ・・逅・
//=========================================================================================================
void Player3D_Finalize()
{
	ModelRelease(g_player3D.Model);
}

//=========================================================================================================
// 譖ｴ譁ｰ蜃ｦ逅・
//=========================================================================================================
void Player3D_Update()
{
	Player3D_Respown();	//繝ｪ繧ｹ繝昴・繝ｳ

	//繝励Ξ繧､繝､繝ｼ謫堺ｽ・
	Player3D_Move();	//遘ｻ蜍・
	Player3D_Jump();	//繧ｸ繝｣繝ｳ繝・
	Player3D_Change();	//蠖ｱ螟芽ｺｫ
	Player3D_Action();	//繧｢繧ｯ繧ｷ繝ｧ繝ｳ

	Player3D_Gravity();	//驥榊鴨蜃ｦ逅・
	
	switch (g_player3D.state)

	{
	case PLAYER3D_IDLE:
		Player3D_Idle();
		break;
	case PLAYER3D_MOVE:
		Player3D_Move();
		break;
	case PLAYER3D_DIRECTION:
		Player3D_Direction();
		break;
	case PLAYER3D_POWER:
		Player3D_Power();
		break;
	case PLAYER3D_RESPAWN:
		Player3D_Respawn();
		Camera_Initialize();
		break;
	}

}


//=========================================================================================================
// 謠冗判蜃ｦ逅・
//=========================================================================================================
void Player3D_Draw()
{
<<<<<<< HEAD
	// ワールド行列の作成
	//スケーリング行列の作成
	XMMATRIX ScalingMatrix = XMMatrixScaling(g_Player3D.Scaling.x, g_Player3D.Scaling.y, g_Player3D.Scaling.z);
	//平行移動行列の作成
	XMMATRIX TranslationMatrix = XMMatrixTranslation(g_Player3D.Position.x, g_Player3D.Position.y, g_Player3D.Position.z);
	//回転行列の作成
	XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(g_Player3D.Rotation.x), XMConvertToRadians(g_Player3D.Rotation.y), XMConvertToRadians(g_Player3D.Rotation.z));
	//計算の順番「スケール*回転*平行移動」
	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;
=======
	//繝ｯ繝ｼ繝ｫ繝芽｡悟・菴懈・
	XMMATRIX scale = XMMatrixScaling
	(
		g_player3D.Scaling.x,
		g_player3D.Scaling.y,
		g_player3D.Scaling.z);
>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a

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

	//螟画鋤陦悟・菴懈・
	XMMATRIX view = GetViewMatrix();
	XMMATRIX projection = GetProjectionMatrix();
	XMMATRIX wvp = world * view * projection;

	//繧ｷ繧ｧ繝ｼ繝繝ｼ縺ｸ陦悟・繧偵そ繝・ヨ
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	//繝｢繝・Ν縺ｮ謠冗判繝ｪ繧ｯ繧ｨ繧ｹ繝・
	ModelDraw(g_player3D.Model);
}

//=========================================================================================================
// 繧ｲ繝・ち繝ｼ
//=========================================================================================================
XMFLOAT3 GetPlayer3DPositon()
{
	return g_player3D.Position;
}

//=========================================================================================================
// state縺斐→縺ｮ蜃ｦ逅・ｼ・dle迥ｶ諷具ｼ・
//=========================================================================================================
void Player3D_Idle()
{
<<<<<<< HEAD
	// 横・縦・奥行きの加算（既存ロジック保持）
	if (g_Player3D.Velocity.x >= maxMoveSpeed)
	{
		g_Player3D.Velocity.x = maxMoveSpeed;
	}
	else
	{
		g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	}

=======

	g_Player3D.Velocity.x += g_Player3D.Acceleration.x; //驥榊鴨
>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a
	if (g_Player3D.Velocity.y < maxGravity)
	{
		g_Player3D.Velocity.y = maxGravity;
	}
	else
	{
<<<<<<< HEAD
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y;
	}
=======
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y; //驥榊鴨
	}
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z; //驥榊鴨
>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a

	if (g_Player3D.Velocity.z >= maxMoveSpeed)
	{
		g_Player3D.Velocity.z = maxMoveSpeed;
	}
	else
	{
		g_Player3D.Velocity.z += g_Player3D.Acceleration.z;
	}

	//摩擦による減速
	g_Player3D.Velocity.x *= 0.925f;
	g_Player3D.Velocity.y *= 0.98f;
	g_Player3D.Velocity.z *= 0.925f;

	// 座標に速度を加算
	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;
=======

<<<<<<< HEAD

=======
}
>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a


//=========================================================================================================
// state縺斐→縺ｮ蜃ｦ逅・ｼ・ove迥ｶ諷具ｼ・
//=========================================================================================================
void Player3D_Move()
{
	g_player3D.Velocity.x += g_player3D.Acceleration.x; //驥榊鴨
	g_player3D.Velocity.y += g_player3D.Acceleration.y; //驥榊鴨
	g_player3D.Velocity.z += g_player3D.Acceleration.z; //驥榊鴨


	// 髱呎ｭ｢繝√ぉ繝・け
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

	int hit = Player3DField_Collision();
}

void Player3D_Respown()
{
=======
	g_player3D.Position.x += g_player3D.Velocity.x;
	g_player3D.Position.y += g_player3D.Velocity.y;
	g_player3D.Position.z += g_player3D.Velocity.z;

	g_player3D.Velocity.x *= 0.98f;		//螂ｽ縺ｿ縺ｧ貂幄｡ｰ縺輔○繧・
	//g_player3D.Velocity.y *= 0.98f;		//螂ｽ縺ｿ縺ｧ貂幄｡ｰ縺輔○繧・
	g_player3D.Velocity.z *= 0.98f;		//螂ｽ縺ｿ縺ｧ貂幄｡ｰ縺輔○繧・


	//關ｽ荳九メ繧ｧ繝・け
	if (g_player3D.Position.y < -10.0f)
	{
		g_player3D.state = PLAYER3D_RESPAWN;
		return;
	}
	
	//蜑咲ｧｻ蜍・
	if (Keyboard_IsKeyDown(KK_W))
	{
		g_player3D.Position.z += 0.1f;
	}

void Player3D_Move()
{
	// 前フレームの入力をリセット（キーを離したときに以前の入力が残らないようにする）
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (Keyboard_IsKeyDown(UpKey))    inputDir.z += +1.0f;
	if (Keyboard_IsKeyDown(DownKey))  inputDir.z += -1.0f;
	if (Keyboard_IsKeyDown(RightKey)) inputDir.x += +1.0f;
	if (Keyboard_IsKeyDown(LeftKey))  inputDir.x += -1.0f;
	

	// 長さ計算
	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 0.0001f)
	{
<<<<<<< HEAD
		// 正規化ベクトル × 加速
		inputDir.x /= len;
		inputDir.z /= len;
		g_Player3D.Velocity.x += inputDir.x * moveSpeed;
		g_Player3D.Velocity.z += inputDir.z * moveSpeed;

		// --- 入力がある場合にのみ向きを更新 ---
		float targetYawRad = atan2f(inputDir.x, inputDir.z); // (x,z) -> 前方を Z とした角度
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// モデルの初期向きに合わせるオフセット（必要なら調整）
		const float yawOffset = FirstRotation.y;
		targetYawDeg += yawOffset;

		// スムーズ回転（角度差を最短経路で求めて補間）
		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f; // 0..1（1で即時回転）
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}
	// 入力無しのときは回転を変更しない（最後に向いていた方向を保持）
}

void Player3D_Jump()
{
	if (Keyboard_IsKeyDownTrigger(JumpKey))
	{
		g_Player3D.Velocity.y += jumpPower;// テスト
		if(isGround)
		{
			g_Player3D.Velocity.y += jumpPower;// 上向きに初速を与える
			// 空中にいる状態へ
			//g_Player3D.state = PLAYER3D_STATE_FALL;
		}
=======
		//g_Player3D.Position.z += moveSpeed;
		g_Player3D.Velocity.z += +moveSpeed;
	}
	if (Keyboard_IsKeyDown(RightKey))
	{
		//g_Player3D.Position.x += moveSpeed;
		g_Player3D.Velocity.x += +moveSpeed;
	}
	if (Keyboard_IsKeyDown(DownKey))
	{
		//g_Player3D.Position.z += -moveSpeed;
		g_Player3D.Velocity.z += -moveSpeed;
	}
	if (Keyboard_IsKeyDown(LeftKey))
	{
		//g_Player3D.Position.x += -moveSpeed;
		g_Player3D.Velocity.x += -moveSpeed;

	//蠕後ｍ遘ｻ蜍・
	if (Keyboard_IsKeyDown(KK_S))
	{
		g_player3D.Position.z -= 0.1f;
	}

	//蜿ｳ遘ｻ蜍・
	if (Keyboard_IsKeyDown(KK_D))
	{
		g_player3D.Position.x += 0.1f;

	}

	//蟾ｦ遘ｻ蜍・
	if (Keyboard_IsKeyDown(KK_A))
	{
		g_player3D.Position.x -= 0.1f;
	}

	float hit = Player3DField_Collision();

	
}

//=========================================================================================================
// state縺斐→縺ｮ蜃ｦ逅・ｼ・ower迥ｶ諷具ｼ・
//=========================================================================================================
void Player3D_Power()
{
	/*float power = PLAYER3D_SPEEDMAX * 0.12f;

	g_player3D.Velocity.x *= power;
	g_player3D.Velocity.y *= power;
	g_player3D.Velocity.z *= power;*/



	g_player3D.state = PLAYER3D_MOVE;
}

//=========================================================================================================
// state縺斐→縺ｮ蜃ｦ逅・ｼ・irection迥ｶ諷具ｼ・
//=========================================================================================================
void Player3D_Direction()
{
	//繧ｭ繝ｼ繧呈款縺励◆繧芽ｻ｢縺後ｋ
	if (Keyboard_IsKeyDownTrigger(KK_F))
	{
		//繧ｫ繝｡繝ｩ縺ｮ蜷代″繧貞叙蠕・
		XMFLOAT3 Cap = GetCameraAtPosition();
		XMFLOAT3 Cp = GetCameraPosition();
		XMFLOAT3 Direction;
		Direction.x = Cap.x - Cp.x;
		Direction.y = 0.0f;
		Direction.z = Cap.z - Cp.z;

		//豁｣隕丞喧
		float len = sqrtf(Direction.x * Direction.x + Direction.y * Direction.y + Direction.z * Direction.z);
		Direction.x /= len;
		Direction.y /= len;
		Direction.z /= len;

		g_player3D.Velocity = Direction;
		g_player3D.state =PLAYER3D_MOVE;
>>>>>>> 693a74ade764a6e3125aecf28c8f9589aa595c8a
	}
}

//=========================================================================================================
// state縺斐→縺ｮ蜃ｦ逅・ｼ・espawn迥ｶ諷具ｼ・
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
