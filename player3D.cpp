#include "Player3D.h"
#include "Controller.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"
#include "manager.h"
#include "Input.h"

<<<<<<< HEAD
#include "debug.h"// ÉfÉoÉbÉOóp

//=========================================================================================================
// É}ÉNÉçíËã`
//=========================================================================================================

//=========================================================================================================
// ÉOÉçÅ[ÉoÉãïœêî
=======
#include "debug.h"// ÔøΩfÔøΩoÔøΩbÔøΩOÔøΩp

//=========================================================================================================
// ÔøΩ}ÔøΩNÔøΩÔøΩÔøΩÔøΩ`
//=========================================================================================================

//=========================================================================================================
// ÔøΩOÔøΩÔøΩÔøΩ[ÔøΩoÔøΩÔøΩÔøΩœêÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
//=========================================================================================================
PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
static float g_StopTime = 0.0f;

<<<<<<< HEAD
// ÉRÉìÉgÉçÅ[ÉâÅ[
extern Controller gPad;

// ì¸óÕÉxÉNÉgÉã
static XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

// ÉäÉZÉbÉgóp
=======
// ÔøΩRÔøΩÔøΩÔøΩgÔøΩÔøΩÔøΩ[ÔøΩÔøΩÔøΩ[
extern Controller gPad;

// ÔøΩÔøΩÔøΩÕÉxÔøΩNÔøΩgÔøΩÔøΩ
static XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

// ÔøΩÔøΩÔøΩZÔøΩbÔøΩgÔøΩp
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
static XMFLOAT3		Firstposition;
static XMFLOAT3		FirstRotation;
static XMFLOAT3		FirstScaling;
static XMFLOAT3		FirstVelocity;
static XMFLOAT3		FirstAcceleration;
static PLAYER3D_STATE	FirstState;
static float			FirstStopTime;
static XMVECTOR		FirstQuaternion;

<<<<<<< HEAD
// ÉvÉåÉCÉÑÅ[ÉXÉeÅ[É^ÉX
float moveSpeed = 0.005f;			//à⁄ìÆë¨ìx
float maxMoveSpeed = 1.0f;			//ç≈ëÂà⁄ìÆë¨ìx
float maxFallSpeed = -0.5f;			//ç≈ëÂóéâ∫ë¨ìx
float  dampingXZ = 0.925f;			//ñÄéCåWêî
//float gravityPower = 1.0f;		//èdóÕâ¡ë¨ìxÅiÇ‡ÇµÇ©ÇµÇΩÇÁégÇ§ó\íËÅj
float jumpPower = 0.175f;			//ÉWÉÉÉìÉvóÕ
bool isGround = false;				//ê⁄ínîªíË

float FirstMaxMoveSpeed = maxMoveSpeed;

// ÉLÅ[É{Å[ÉhíËã`
// à⁄ìÆ
static const auto UpKey = KK_W;			//ëOêi
static const auto RightKey = KK_D;		//âEà⁄ìÆ
static const auto DownKey = KK_S;		//å„ëﬁ
static const auto LeftKey = KK_A;		//ç∂à⁄ìÆ
// çsìÆ
static const auto JumpKey = KK_SPACE;	//ÉWÉÉÉìÉv
static const auto ActionKey = KK_F;		//ÉAÉNÉVÉáÉì
static const auto ChangeKey = KK_F;		//âeïœêg
// ÇªÇÃëº
static const auto ResetKey = KK_R;		//ÉäÉZÉbÉg
static const auto MenuKey = KK_ESCAPE;	//ÉÅÉjÉÖÅ[
=======
// ÔøΩvÔøΩÔøΩÔøΩCÔøΩÔøΩÔøΩ[ÔøΩXÔøΩeÔøΩ[ÔøΩ^ÔøΩX
static float moveSpeed = 0.005f;			//ÔøΩ⁄ìÔøΩÔøΩÔøΩÔøΩx
static float maxMoveSpeed = 1.0f;			//ÔøΩ≈ëÔøΩ⁄ìÔøΩÔøΩÔøΩÔøΩx
static float maxFallSpeed = -0.5f;			//ÔøΩ≈ëÂóéÔøΩÔøΩÔøΩÔøΩÔøΩx
static float  dampingXZ = 0.925f;			//ÔøΩÔøΩÔøΩCÔøΩWÔøΩÔøΩ
//static float gravityPower = 1.0f;		//ÔøΩdÔøΩÕâÔøΩÔøΩÔøΩÔøΩxÔøΩiÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩgÔøΩÔøΩÔøΩ\ÔøΩÔøΩj
static float jumpPower = 0.175f;			//ÔøΩWÔøΩÔøΩÔøΩÔøΩÔøΩvÔøΩÔøΩ


float FirstMaxMoveSpeed = maxMoveSpeed;

// ÔøΩLÔøΩ[ÔøΩ{ÔøΩ[ÔøΩhÔøΩÔøΩ`
// ÔøΩ⁄ìÔøΩ
static const auto UpKey = KK_W;			//ÔøΩOÔøΩi
static const auto RightKey = KK_D;		//ÔøΩEÔøΩ⁄ìÔøΩ
static const auto DownKey = KK_S;		//ÔøΩÔøΩÔøΩ
static const auto LeftKey = KK_A;		//ÔøΩÔøΩÔøΩ⁄ìÔøΩ
// ÔøΩsÔøΩÔøΩ
static const auto JumpKey = KK_SPACE;	//ÔøΩWÔøΩÔøΩÔøΩÔøΩÔøΩv
static const auto ActionKey = KK_F;		//ÔøΩAÔøΩNÔøΩVÔøΩÔøΩÔøΩÔøΩ
static const auto ChangeKey = KK_F;		//ÔøΩeÔøΩœêg
// ÔøΩÔøΩÔøΩÃëÔøΩ
static const auto ResetKey = KK_R;		//ÔøΩÔøΩÔøΩZÔøΩbÔøΩg
static const auto MenuKey = KK_ESCAPE;	//ÔøΩÔøΩÔøΩjÔøΩÔøΩÔøΩ[
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c

static bool debugMode = true;
static bool isTrigger = false;

<<<<<<< HEAD
// ÉvÉåÉCÉÑÅ[ìñÇΩÇËîªíËîºï™ÇÃëÂÇ´Ç≥
=======
// ÔøΩvÔøΩÔøΩÔøΩCÔøΩÔøΩÔøΩ[ÔøΩÔøΩÔøΩÔøΩÔøΩËîªÔøΩËîºÔøΩÔøΩÔøΩÃëÂÇ´ÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
static XMFLOAT3 g_SolidHalfSize = XMFLOAT3(
	PLAYER3D_SOLID_HALF_X,
	PLAYER3D_SOLID_HALF_Y,
	PLAYER3D_SOLID_HALF_Z
);
static XMFLOAT3 g_TriggerHalfSize = XMFLOAT3(
	PLAYER3D_TRIGGER_HALF_X,
	PLAYER3D_TRIGGER_HALF_Y,
	PLAYER3D_TRIGGER_HALF_Z
);

static bool g_Player3DActive = true;

//=========================================================================================================
<<<<<<< HEAD
// èâä˙âªèàóù
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ÉfÉoÉCÉXÇ∆ÉfÉoÉCÉXÉRÉìÉeÉLÉXÉgÇÃï€ë∂
=======
// ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// ÔøΩfÔøΩoÔøΩCÔøΩXÔøΩ∆ÉfÔøΩoÔøΩCÔøΩXÔøΩRÔøΩÔøΩÔøΩeÔøΩLÔøΩXÔøΩgÔøΩÃï€ëÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Player3D.Model = ModelLoad("asset\\model\\Hip_Hop_Dancing.fbx");

	Firstposition = g_Player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	FirstRotation = g_Player3D.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstScaling = g_Player3D.Scaling = XMFLOAT3(0.01f, 0.01f, 0.01f);
	FirstVelocity = g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstAcceleration = g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	FirstState = g_Player3D.state = PLAYER3D_STATE_MOVE;
	FirstStopTime = g_StopTime = 0.0f;
	FirstQuaternion = g_Player3D.Quaternion = XMQuaternionIdentity();

<<<<<<< HEAD
	FirstMaxMoveSpeed = maxMoveSpeed;			//èâä˙ç≈ëÂà⁄ìÆë¨ìx
=======
	FirstMaxMoveSpeed = maxMoveSpeed;			//ÔøΩÔøΩÔøΩÔøΩÔøΩ≈ëÔøΩ⁄ìÔøΩÔøΩÔøΩÔøΩx
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c

}

//=========================================================================================================
<<<<<<< HEAD
// èIóπèàóù
=======
// ÔøΩIÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
//=========================================================================================================
void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}

//=========================================================================================================
<<<<<<< HEAD
// çXêVèàóù
//=========================================================================================================
void Player3D_Update()
{
	Player3D_Respawn();	//ÉäÉXÉ|Å[Éì

	Player3D_Gravity();	//èdóÕèàóù

	// ÉvÉåÉCÉÑÅ[ëÄçÏ
	Player3D_Move();	//à⁄ìÆ
	Player3D_Jump();	//ÉWÉÉÉìÉv
	Player3D_Change();	//âeïœêg
	Player3D_Action();	//ÉAÉNÉVÉáÉì
=======
// ÔøΩXÔøΩVÔøΩÔøΩÔøΩÔøΩ
//=========================================================================================================
void Player3D_Update()
{
	if (!g_Player3DActive) return;

	Player3D_Respawn();	//ÔøΩÔøΩÔøΩXÔøΩ|ÔøΩ[ÔøΩÔøΩ

	Player3D_Gravity();	//ÔøΩdÔøΩÕèÔøΩÔøΩÔøΩ

	// ÔøΩvÔøΩÔøΩÔøΩCÔøΩÔøΩÔøΩ[ÔøΩÔøΩÔøΩÔøΩ
	Player3D_Move();	//ÔøΩ⁄ìÔøΩ
	Player3D_Jump();	//ÔøΩWÔøΩÔøΩÔøΩÔøΩÔøΩv
	Player3D_Change();	//ÔøΩeÔøΩœêg
	Player3D_Action();	//ÔøΩAÔøΩNÔøΩVÔøΩÔøΩÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c

	switch (g_Player3D.state)
	{
	case PLAYER3D_STATE_IDLE:
		//IdleÉAÉjÉÅÅ[ÉVÉáÉì
		break;
	case PLAYER3D_STATE_MOVE:
		//MoveÉAÉjÉÅÅ[ÉVÉáÉì
		break;
	case PLAYER3D_STATE_FALL:
		//FallÉAÉjÉÅÅ[ÉVÉáÉì
		break;
	case PLAYER3D_STATE_ACTION:
		//ActionÉAÉjÉÅÅ[ÉVÉáÉì
		break;
	default:
		break;
	}
}




//=========================================================================================================
<<<<<<< HEAD
// ÉQÉbÉ^Å[
=======
// ÔøΩQÔøΩbÔøΩ^ÔøΩ[
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
//=========================================================================================================
XMFLOAT3 GetPlayer3DPosition()
{
	return g_Player3D.Position;
}


//=========================================================================================================
<<<<<<< HEAD
// èàóù
=======
// ÔøΩÔøΩÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
//=========================================================================================================

void Player3D_Gravity()
{
<<<<<<< HEAD
	// --- èdóÕâ¡éZÅiãÛíÜÇÃÇ›Åj ---
=======
	// --- ÔøΩdÔøΩÕâÔøΩÔøΩZÔøΩiÔøΩÛíÜÇÃÇ›Åj ---
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	if (g_Player3D.Velocity.x >= maxMoveSpeed) g_Player3D.Velocity.x = maxMoveSpeed;
	else g_Player3D.Velocity.x += g_Player3D.Acceleration.x;

	if (g_Player3D.Velocity.z >= maxMoveSpeed) g_Player3D.Velocity.z = maxMoveSpeed;
	else g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *= 0.925f;
	g_Player3D.Velocity.z *= 0.925f;

	if (!g_Player3D.isGround)
	{
		//if (g_Player3D.Velocity.y 
		// < maxGravity) g_Player3D.Velocity.y = maxGravity;
		//else g_Player3D.Velocity.y += g_Player3D.Acceleration.y;
	}
	else
	{
		if (g_Player3D.Velocity.y < 0.0f)
			g_Player3D.Velocity.y = 0.0f;
	}

<<<<<<< HEAD
	// --- XZà⁄ìÆÅiè]óàÇ«Ç®ÇËÅj ---
=======
	// --- XZÔøΩ⁄ìÔøΩÔøΩiÔøΩ]ÔøΩÔøΩÔøΩ«ÇÔøΩÔøΩÔøΩj ---
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	g_Player3D.Velocity.z += g_Player3D.Acceleration.z;

	g_Player3D.Velocity.x *=  dampingXZ;
	g_Player3D.Velocity.z *=  dampingXZ;

	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;

	int hit = Player3DField_Collision();
}


void Player3D_Respawn()
{
<<<<<<< HEAD
	// óéâ∫É`ÉFÉbÉN
=======
	// ÔøΩÔøΩÔøΩÔøΩÔøΩ`ÔøΩFÔøΩbÔøΩN
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	if (g_Player3D.Position.y < -10.0f)
	{
		Player3D_Reset();
		return;
	}
	if (Keyboard_IsKeyDownTrigger(ResetKey))
	{
		Player3D_Reset();
	}
}

void Player3D_Move()
{

<<<<<<< HEAD
	// ëOÉtÉåÅ[ÉÄÇÃì¸óÕÇÉäÉZÉbÉgÅiÉLÅ[Çó£ÇµÇΩÇ∆Ç´Ç…à»ëOÇÃì¸óÕÇ™écÇÁÇ»Ç¢ÇÊÇ§Ç…Ç∑ÇÈÅj
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (!gPad.IsConnected())
	{// ÉLÅ[É{Å[Éhì¸óÕ
=======
	// ÔøΩOÔøΩtÔøΩÔøΩÔøΩ[ÔøΩÔøΩÔøΩÃìÔøΩÔøΩÕÇÔøΩÔøΩÔøΩÔøΩZÔøΩbÔøΩgÔøΩiÔøΩLÔøΩ[ÔøΩó£ÇÔøΩÔøΩÔøΩÔøΩ∆ÇÔøΩÔøΩ…à»ëOÔøΩÃìÔøΩÔøΩÕÇÔøΩÔøΩcÔøΩÔøΩ»ÇÔøΩÔøΩÊÇ§ÔøΩ…ÇÔøΩÔøΩÔøΩj
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (!gPad.IsConnected())
	{// ÔøΩLÔøΩ[ÔøΩ{ÔøΩ[ÔøΩhÔøΩÔøΩÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
		if (Keyboard_IsKeyDown(UpKey))    inputDir.z += +1.0f;
		if (Keyboard_IsKeyDown(DownKey))  inputDir.z += -1.0f;
		if (Keyboard_IsKeyDown(RightKey)) inputDir.x += +1.0f;
		if (Keyboard_IsKeyDown(LeftKey))  inputDir.x += -1.0f;
	}
	else
<<<<<<< HEAD
	{// ÉRÉìÉgÉçÅ[ÉâÅ[ì¸óÕ
		float lx = gPad.GetLeftStickX(); // -1Å`1
		float ly = gPad.GetLeftStickY(); // -1Å`1

		// ÉfÉbÉhÉ]Å[ÉìÇì¸ÇÍÇƒî˜è¨ì¸óÕÇñ≥éã
=======
	{// ÔøΩRÔøΩÔøΩÔøΩgÔøΩÔøΩÔøΩ[ÔøΩÔøΩÔøΩ[ÔøΩÔøΩÔøΩÔøΩ
		float lx = gPad.GetLeftStickX(); // -1ÔøΩ`1
		float ly = gPad.GetLeftStickY(); // -1ÔøΩ`1

		// ÔøΩfÔøΩbÔøΩhÔøΩ]ÔøΩ[ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩƒîÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÕÇñ≥éÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
		const float deadzone = 0.20f;
		if (fabsf(lx) < deadzone) lx = 0.0f;
		if (fabsf(ly) < deadzone) ly = 0.0f;

		// ç∂âEÇ™ãtÇ…Ç»ÇÈñ‚ëËÇèCê≥ÅiÉXÉeÉBÉbÉNâEÇ≈ x Ç™ê≥Ç…Ç»ÇÈÇÊÇ§ïÑçÜÇîΩì]Åj
		inputDir.x = -lx;      // ç∂âE
		inputDir.y = 0.0f;     // 3DÇ»ÇÁ Y ÇÕçÇÇ≥Ç∆ÇµÇƒégÇÌÇ»Ç¢
		inputDir.z = -ly;      // ëOå„
	}

<<<<<<< HEAD
	// í∑Ç≥åvéZ
	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 1e-6f)
	{
		// ê≥ãKâªÉxÉNÉgÉã Å~ â¡ë¨
		inputDir.x /= len;
		inputDir.z /= len;

		// ÉJÉÅÉâÇÃå¸Ç´Ç…çáÇÌÇπÇƒà⁄ìÆï˚å¸Çïœä∑
=======
	// ÔøΩÔøΩÔøΩÔøΩÔøΩvÔøΩZ
	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 1e-6f)
	{
		// ÔøΩÔøΩÔøΩKÔøΩÔøΩÔøΩxÔøΩNÔøΩgÔøΩÔøΩ ÔøΩ~ ÔøΩÔøΩÔøΩÔøΩ
		inputDir.x /= len;
		inputDir.z /= len;

		// ÔøΩJÔøΩÔøΩÔøΩÔøΩÔøΩÃåÔøΩÔøΩÔøΩÔøΩ…çÔøΩÔøΩÌÇπÔøΩƒà⁄ìÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩœäÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
		XMFLOAT3 camPos = GetCameraPosition();
		XMFLOAT3 camAt = GetCameraAtPosition();
		XMFLOAT3 camFwd = XMFLOAT3(
			camAt.x - camPos.x,
			0.0f,
			camAt.z - camPos.z
<<<<<<< HEAD
		);// ÉJÉÅÉâÇÃëOï˚å¸ÉxÉNÉgÉã
=======
		);// ÔøΩJÔøΩÔøΩÔøΩÔøΩÔøΩÃëOÔøΩÔøΩÔøΩÔøΩÔøΩxÔøΩNÔøΩgÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
		float flen = sqrtf(camFwd.x * camFwd.x + camFwd.z * camFwd.z);
		if (flen > 1e-6f)
		{
			camFwd.x /= flen;
			camFwd.z /= flen;
		}
		else
		{
			camFwd = XMFLOAT3(0.0f, 0.0f, 1.0f);
		}

<<<<<<< HEAD
		// ÉJÉÅÉâÇÃâEï˚å¸ÉxÉNÉgÉã
=======
		// ÔøΩJÔøΩÔøΩÔøΩÔøΩÔøΩÃâEÔøΩÔøΩÔøΩÔøΩÔøΩxÔøΩNÔøΩgÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
		XMFLOAT3 camRight = XMFLOAT3(
			camFwd.z,
			0.0f,
			-camFwd.x
		);
		float rlen = sqrtf(camRight.x * camRight.x + camRight.z * camRight.z);
		if (rlen > 1e-6f)
		{
			camRight.x /= rlen;
			camRight.z /= rlen;
		}

<<<<<<< HEAD
		// à⁄ìÆï˚å¸ÇÉèÅ[ÉãÉhç¿ïWÇ…ïœä∑
=======
		// ÔøΩ⁄ìÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ[ÔøΩÔøΩÔøΩhÔøΩÔøΩÔøΩWÔøΩ…ïœäÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
		XMFLOAT3 moveWorld = XMFLOAT3(
			camFwd.x * inputDir.z + camRight.x * inputDir.x,
			0.0f,
			camFwd.z * inputDir.z + camRight.z * inputDir.x
		);
		float mlen = sqrtf(moveWorld.x * moveWorld.x + moveWorld.z * moveWorld.z);
		if (mlen > 1e-6f)
		{
			moveWorld.x /= mlen;
			moveWorld.z /= mlen;
		}

		g_Player3D.Velocity.x += moveWorld.x * moveSpeed;
		g_Player3D.Velocity.z += moveWorld.z * moveSpeed;

<<<<<<< HEAD
		// ì¸óÕÇ™Ç†ÇÈèÍçáÇ…ÇÃÇ›å¸Ç´ÇçXêV
		float targetYawRad = atan2f(moveWorld.x, moveWorld.z);
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// ÉÇÉfÉãÇÃèâä˙å¸Ç´Ç…çáÇÌÇπÇÈÉIÉtÉZÉbÉgÅiïKóvÇ»ÇÁí≤êÆÅj
		const float yawOffset = FirstRotation.y;
		targetYawDeg += yawOffset;

		// ÉXÉÄÅ[ÉYâÒì]Åiäpìxç∑Çç≈íZåoòHÇ≈ãÅÇﬂÇƒï‚ä‘Åj
=======
		// ÔøΩÔøΩÔøΩÕÇÔøΩÔøΩÔøΩÔøΩÔøΩÍçáÔøΩ…ÇÃÇ›åÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩXÔøΩV
		float targetYawRad = atan2f(moveWorld.x, moveWorld.z);
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// ÔøΩÔøΩÔøΩfÔøΩÔøΩÔøΩÃèÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ…çÔøΩÔøΩÌÇπÔøΩÔøΩIÔøΩtÔøΩZÔøΩbÔøΩgÔøΩiÔøΩKÔøΩvÔøΩ»ÇÁí≤ÔøΩÔøΩÔøΩj
		const float yawOffset = FirstRotation.y;
		targetYawDeg += yawOffset;

		// ÔøΩXÔøΩÔøΩÔøΩ[ÔøΩYÔøΩÔøΩ]ÔøΩiÔøΩpÔøΩxÔøΩÔøΩÔøΩÔøΩÔøΩ≈íZÔøΩoÔøΩHÔøΩ≈ãÔøΩÔøΩﬂÇƒïÔøΩ‘Åj
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

<<<<<<< HEAD
		const float rotateLerp = 0.2f; //0..1Åi1Ç≈ë¶éûâÒì]Åj
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}
	// ì¸óÕñ≥ÇµÇÃÇ∆Ç´ÇÕâÒì]ÇïœçXÇµÇ»Ç¢Åiç≈å„Ç…å¸Ç¢ÇƒÇ¢ÇΩï˚å¸Çï€éùÅj

	//ë¨ìxêßå¿
=======
		const float rotateLerp = 0.2f; //0..1ÔøΩi1ÔøΩ≈ëÔøΩÔøΩÔøΩÔøΩÔøΩ]ÔøΩj
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}
	// ÔøΩÔøΩÔøΩÕñÔøΩÔøΩÔøΩÔøΩÃÇ∆ÇÔøΩÔøΩÕâÔøΩ]ÔøΩÔøΩœçXÔøΩÔøΩÔøΩ»ÇÔøΩÔøΩiÔøΩ≈åÔøΩ…åÔøΩÔøΩÔøΩÔøΩƒÇÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ€éÔøΩÔøΩj

	//ÔøΩÔøΩÔøΩxÔøΩÔøΩÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	if (g_Player3D.Velocity.x >= maxMoveSpeed)
	{
		g_Player3D.Velocity.x = maxMoveSpeed;
	}
	else if (g_Player3D.Velocity.x <= -maxMoveSpeed)
	{
		g_Player3D.Velocity.x = -maxMoveSpeed;
	}
	if (g_Player3D.Velocity.z >= maxMoveSpeed)
	{
		g_Player3D.Velocity.z = maxMoveSpeed;
	}
	else if (g_Player3D.Velocity.z <= -maxMoveSpeed)
	{
		g_Player3D.Velocity.z = -maxMoveSpeed;
	}
}

void Player3D_Jump()
{
	if (Keyboard_IsKeyDownTrigger(JumpKey) || gPad.IsButtonPressed(JumpKey))
	{
		if (g_Player3D.isGround)
		{
			g_Player3D.Velocity.y = jumpPower;
			g_Player3D.isGround = false;
			g_Player3D.state = PLAYER3D_STATE_FALL;
		}
	}
}

void Player3D_Change()
{
	if (Keyboard_IsKeyDownTrigger(ChangeKey))
	{
		// ÅüâeÇ…ïœêg
		// 
		// ï«Ç…ãﬂÇ√Ç¢ÇΩÇ∆Ç´Ç…îΩâû 
		// Å´
		// 2DÉLÉÉÉâÉNÉ^Å[Ç3DÉvÉåÉCÉÑÅ[ÇÃç¿ïWÇéQè∆ÇµÇƒê∂ê¨
		// Å´
		// 3DÉvÉåÉCÉÑÅ[ÇçÌèúÇ∑ÇÈ
		// 
		// if(ï«ÉIÉuÉWÉFÉNÉgÇÃç¿ïWÇ∆ÉvÉåÉCÉÑÅ[ÇÃç¿ïWÇ™àÍíËà»è„ãﬂÇ¢)
		// {
		// 	
		// }
		// 
		// 
	}
}

void Player3D_Action()
{
	if (Keyboard_IsKeyDownTrigger(ActionKey))
	{
		// Åüî†ÇéùÇ¬
		//
		// ÉAÉjÉÅÅ[ÉVÉáÉì
		// Å´
		// î†ÇÉvÉåÉCÉÑÅ[ÇÃç¿ïWÇ…í«è]Ç≥ÇπÇÈ
		// Å´
		// î†éùÇøèÛë‘ÇtrueÇ…Ç∑ÇÈ
		// 
		// 
		// Åüî†Çó£Ç∑
		// 
		// ÉAÉjÉÅÅ[ÉVÉáÉì
		// Å´
		// î†ÇÉvÉåÉCÉÑÅ[Ç…í«è]Ç≥ÇπÇÈÇÃÇÇ‚ÇﬂÇÈ
		// Å´
		// î†éùÇøèÛë‘ÇfalseÇ…Ç∑ÇÈ
		// 
		// 
		// Åüè∆ñæëÄçÏÇénÇﬂÇÈ
		// 
		// ÉAÉjÉÅÅ[ÉVÉáÉì
		// Å´
		// è∆ñæëÄçÏèÛë‘ÇtrueÇ…Ç∑ÇÈ
		// Å´
		// à⁄ìÆì¸óÕÇéÛÇØïtÇØÇ»Ç≠Ç∑ÇÈ
		// 
		//  
		// Åüè∆ñæëÄçÏÇÇ‚ÇﬂÇÈ
		// 
		// ÉAÉjÉÅÅ[ÉVÉáÉì
		// Å´
		// è∆ñæëÄçÏèÛë‘ÇfalseÇ…Ç∑ÇÈ
		// Å´
		// à⁄ìÆì¸óÕÇéÛÇØïtÇØÇÈ
		// 
		// 

	}
	isTrigger = false;

	TRIGGER_HIT hit;
	if (!Collision_PlayerTrigger(&hit, 0.2f)) return;
<<<<<<< HEAD
	if (hit.side != TRIGGER_SIDE_FRONT) return;// ëOñ à»äOÇÕñ≥éã
	switch (hit.type)// ìñÇΩÇ¡ÇΩÉIÉuÉWÉFÉNÉgÇÃéÌóﬁÇ≈èàóùï™äÚ
=======
	if (hit.side != TRIGGER_SIDE_FRONT) return;// ÔøΩOÔøΩ à»äOÔøΩÕñÔøΩÔøΩÔøΩ
	switch (hit.type)// ÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩIÔøΩuÔøΩWÔøΩFÔøΩNÔøΩgÔøΩÃéÔøΩﬁÇ≈èÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	{
	case FIELD_GOAL:

		break;
	case FIELD_OBJ_1:
		isTrigger = true;
		break;
	case FIELD_OBJ_2:

		break;

	default:
		break;
	}
}

void Player3D_Reset()
{
	g_Player3D.Position = Firstposition;
	g_Player3D.Rotation = FirstRotation;
	g_Player3D.Scaling = FirstScaling;
	g_Player3D.Velocity = FirstVelocity;
	g_Player3D.Acceleration = FirstAcceleration;
	g_Player3D.state = FirstState;
	g_StopTime = FirstStopTime;
	g_Player3D.Quaternion = FirstQuaternion;

	maxMoveSpeed = FirstMaxMoveSpeed;
}

PLAYER3D* GetPlayer3D()
{
	return &g_Player3D;
}

XMFLOAT3 Player3D_GetSolidHalfSize()
{
	return g_SolidHalfSize;
}

XMFLOAT3 Player3D_GetTriggerHalfSize()
{
	return g_TriggerHalfSize;
}

//=========================================================================================================
<<<<<<< HEAD
// ï`âÊèàóù
=======
// ÔøΩ`ÔøΩÊèàÔøΩÔøΩ
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
//=========================================================================================================
void Player3D_Draw(void)
{

	if (!g_Player3DActive) return;

	if (debugMode)
	{
		/*ImGui::Begin("Debug - han");
		if (ImGui::TreeNode("Player3D.cpp"))
		{
			ImGui::Text("Trigger: %s", isTrigger ? "true" : "false");
			ImGui::TreePop();
		}
		ImGui::End();*/
	}



	XMMATRIX scale = XMMatrixScaling
	(
		g_Player3D.Scaling.x,
		g_Player3D.Scaling.y,
		g_Player3D.Scaling.z);

	XMMATRIX rotation = XMMatrixRotationRollPitchYaw
	(
		XMConvertToRadians(g_Player3D.Rotation.x),
		XMConvertToRadians(g_Player3D.Rotation.y),
		XMConvertToRadians(g_Player3D.Rotation.z)

	);

	XMMATRIX translation = XMMatrixTranslation
	(
		g_Player3D.Position.x,
		g_Player3D.Position.y,
		g_Player3D.Position.z
	);

	XMMATRIX world = scale * rotation * translation;

	XMMATRIX view = GetViewMatrix();
	XMMATRIX projection = GetProjectionMatrix();
	XMMATRIX wvp = world * view * projection;

<<<<<<< HEAD
	// ïœä∑çsóÒÇí∏ì_ÉVÉFÅ[É_Ç÷ÉZÉbÉg
=======
	// ÔøΩœäÔøΩÔøΩsÔøΩÔøΩí∏ì_ÔøΩVÔøΩFÔøΩ[ÔøΩ_ÔøΩ÷ÉZÔøΩbÔøΩg
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	ModelUpdateAnimation(g_Player3D.Model, 10.0f / 600.0f);   // CPU animation
	Shader_SetBones(g_Player3D.Model);	// upload bones

<<<<<<< HEAD
	// ÉÇÉfÉãÇÃï`âÊÉäÉNÉGÉXÉg
=======
	// ÔøΩÔøΩÔøΩfÔøΩÔøΩÔøΩÃï`ÔøΩÊÉäÔøΩNÔøΩGÔøΩXÔøΩg
>>>>>>> 970cdee74d2b90f8a74582d9e1a6e94efdbb626c
	ModelDraw(g_Player3D.Model);
}

XMFLOAT3 Player3D_GetForward()
{
	const float pitch = XMConvertToRadians(g_Player3D.Rotation.x);
	const float yaw = XMConvertToRadians(g_Player3D.Rotation.y);
	const float roll = XMConvertToRadians(g_Player3D.Rotation.z);

	XMMATRIX R = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

	XMVECTOR f = XMVector3TransformNormal(XMVectorSet(0, 0, 1, 0), R);
	f = XMVector3Normalize(f);

	XMFLOAT3 out;
	XMStoreFloat3(&out, f);
	return out;
}

void Player3D_InitAt(const XMFLOAT3& pos, const XMFLOAT3& rot)
{
	g_Player3D.Position = pos;
	g_Player3D.Rotation = rot;

	g_Player3D.Velocity = XMFLOAT3(0, 0, 0);
	g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);

	g_Player3D.state = PLAYER3D_STATE_MOVE;
	g_Player3D.isGround = false;
	g_StopTime = 0.0f;
}

void Player3D_SetActive(bool active)
{
	g_Player3DActive = active;
}
