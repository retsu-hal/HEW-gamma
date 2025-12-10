#include "Player3D.h"
#include "keyboard.h"
#include "Camera.h"
#include "shader.h"
#include "Collision.h"

//=========================================================================================================
//�O���[�o���ϐ�
#include "debug.h"

//=========================================================================================================
// �}�N����`
//=========================================================================================================

//=========================================================================================================
// �O���[�o���ϐ�
//=========================================================================================================
PLAYER3D g_Player3D;
ID3D11Device* g_pDevice;
ID3D11DeviceContext* g_pContext;
float g_StopTime = 0.0f;

// ���̓x�N�g��
XMFLOAT3 inputDir(0.0f, 0.0f, 0.0f);

//���Z�b�g�p
XMFLOAT3		Firstposition;
XMFLOAT3		FirstRotation;
XMFLOAT3		FirstScaling;
XMFLOAT3		FirstVelocity;
XMFLOAT3		FirstAcceleration;
PLAYER3D_STATE	FirstState;
float			FirstStopTime;
XMVECTOR		FirstQuaternion;

//�v���C���[�X�e�[�^�X
float moveSpeed = 0.005f;			//�ړ����x�i���ǂ������ǁj
float maxMoveSpeed = 1.0f;			//�ő�ړ����x�i�����������ǂ������ǁj
float maxGravity = -0.25f;			//�ő嗎�����x�i������������������ǁj
float jumpPower = 0.175f;			//�W�����v�́i�����Ղ�傭�j
bool isGround = false;				//�ڒn����i�������͂�Ă��j

//�L�[�{�[�h��`�i�Ă����j
//�ړ��i���ǂ��j
static const auto UpKey = KK_W;			//�O�i�܂��j
static const auto RightKey = KK_D;		//�E�i�݂��j
static const auto DownKey = KK_S;		//��i������j
static const auto LeftKey = KK_A;		//���i�Ђ���j
//�s���i�����ǂ��j
static const auto JumpKey = KK_SPACE;	//�W�����v
static const auto ActionKey = KK_F;		//�A�N�V����
static const auto ChangeKey = KK_F;		//�e�ϐg�i�����ւ񂵂�j
//���̑��i���̂��j
static const auto ResetKey = KK_R;		//���Z�b�g
static const auto MenuKey = KK_ESCAPE;	//�|�[�Y���j���[

static bool debugMode = TRUE;

static XMFLOAT3 g_DetectHalfSize = XMFLOAT3(
	PLAYER3D_DETECT_HALF_X,
	PLAYER3D_DETECT_HALF_Y,
	PLAYER3D_DETECT_HALF_Z
);


//=========================================================================================================
//����������
//=========================================================================================================
void Player3D_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	// �f�o�C�X�ƃf�o�C�X�R���e�L�X�g�̕ۑ�
	g_pDevice = pDevice;
	g_pContext = pContext;

	g_Player3D.Model = ModelLoad("asset\\model\\Test_man_stand.fbx");

	Firstposition = g_Player3D.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
	FirstRotation = g_Player3D.Rotation = XMFLOAT3(-90.0f, 180.0f, 0.0f);
	FirstScaling = g_Player3D.Scaling = XMFLOAT3(0.01f, 0.01f, 0.01f);
	FirstVelocity = g_Player3D.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	FirstAcceleration = g_Player3D.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
	FirstState = g_Player3D.state = PLAYER3D_STATE_MOVE;
	FirstStopTime = g_StopTime = 0.0f;
	FirstQuaternion = g_Player3D.Quaternion = XMQuaternionIdentity();

}

//=========================================================================================================
//�I������
//=========================================================================================================
void Player3D_Finalize(void)
{
	ModelRelease(g_Player3D.Model);
}

//=========================================================================================================
//�X�V����
//=========================================================================================================
void Player3D_Update()
{
	Player3D_Respawn();	//���X�|�[��

	//�v���C���[����
	Player3D_Move();	//�ړ�
	Player3D_Jump();	//�W�����v
	Player3D_Change();	//�e�ϐg
	Player3D_Action();	//�A�N�V����

	Player3D_Gravity();	//�d�͏���


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
//�`�揈��
//=========================================================================================================
void Player3D_Draw(void)
{
	if (debugMode)
	{
<<<<<<< HEAD
		ImGui::Begin("Debug - han");
=======
		ImGui::Begin("Debug - CHEN");
>>>>>>> 810ace0ffaf91fa42f6809e1329e4933140f1320
		if (ImGui::TreeNode("Player3D.cpp"))
		{
			ImGui::Text("PosX: %.2f", g_Player3D.Position.x);
			ImGui::Text("PosY: %.2f", g_Player3D.Position.y);
			ImGui::Text("PosZ: %.2f", g_Player3D.Position.z);
			ImGui::TreePop();
		}
		ImGui::End();

	}
	//���[���h�s��쐬
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

	//�ϊ��s��쐬
	XMMATRIX view = GetViewMatrix();
	XMMATRIX projection = GetProjectionMatrix();
	XMMATRIX wvp = world * view * projection;

	//�V�F�[�_�[�֍s����Z�b�g
	Shader_SetWorldMatrix(world);
	Shader_SetMatrix(wvp);

	//モデルの描画リクエスト
	ModelDraw(g_Player3D.Model);
}

//=========================================================================================================
// �Q�b�^�[
//=========================================================================================================
XMFLOAT3 GetPlayer3DPositon()
{
	return g_Player3D.Position;
}

//=========================================================================================================
//����
//=========================================================================================================

void Player3D_Gravity()
{
	// ���E�c�E���s���̉��Z�i�������W�b�N�ێ��j
	if (g_Player3D.Velocity.x >= maxMoveSpeed)
	{
		g_Player3D.Velocity.x = maxMoveSpeed;
	}
	else
	{
		g_Player3D.Velocity.x += g_Player3D.Acceleration.x;
	}

	if (g_Player3D.Velocity.y < maxGravity)
	{
		g_Player3D.Velocity.y = maxGravity;
	}
	else
	{
		g_Player3D.Velocity.y += g_Player3D.Acceleration.y;
	}

	if (g_Player3D.Velocity.z >= maxMoveSpeed)
	{
		g_Player3D.Velocity.z = maxMoveSpeed;
	}
	else
	{
		g_Player3D.Velocity.z += g_Player3D.Acceleration.z;
	}

	//���C�ɂ�錸��
	g_Player3D.Velocity.x *= 0.925f;
	//g_Player3D.Velocity.y *= 0.98f;
	g_Player3D.Velocity.z *= 0.925f;

	// ���W�ɑ��x�����Z
	g_Player3D.Position.x += g_Player3D.Velocity.x;
	g_Player3D.Position.y += g_Player3D.Velocity.y;
	g_Player3D.Position.z += g_Player3D.Velocity.z;



	// �Î~�`�F�b�N
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

void Player3D_Respawn()
{
	//�����`�F�b�N
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
	// �O�t���[���̓��͂����Z�b�g�i�L�[�𗣂����Ƃ��ɈȑO�̓��͂��c��Ȃ��悤�ɂ���B���ꂪ�Ȃ��ƈ�x�������炻�̕����ɓ���������j
	inputDir = XMFLOAT3(0.0f, 0.0f, 0.0f);

	if (Keyboard_IsKeyDown(UpKey))    inputDir.z += +1.0f;
	if (Keyboard_IsKeyDown(DownKey))  inputDir.z += -1.0f;
	if (Keyboard_IsKeyDown(RightKey)) inputDir.x += +1.0f;
	if (Keyboard_IsKeyDown(LeftKey))  inputDir.x += -1.0f;


	// �����v�Z
	float len = sqrtf(inputDir.x * inputDir.x + inputDir.z * inputDir.z);
	if (len > 0.0001f)
	{
		// ���K���x�N�g�� �~ ����
		inputDir.x /= len;
		inputDir.z /= len;
		g_Player3D.Velocity.x += inputDir.x * moveSpeed;
		g_Player3D.Velocity.z += inputDir.z * moveSpeed;

		// --- ���͂�����ꍇ�ɂ̂݌������X�V ---
		float targetYawRad = atan2f(inputDir.x, inputDir.z); // (x,z) -> �O���� Z �Ƃ����p�x
		float targetYawDeg = XMConvertToDegrees(targetYawRad);

		// ���f���̏��������ɍ��킹��I�t�Z�b�g�i�K�v�Ȃ璲���j
		const float yawOffset = FirstRotation.y;
		targetYawDeg += yawOffset;

		// �X���[�Y��]�i�p�x�����ŒZ�o�H�ŋ��߂ĕ�ԁj
		float currentYaw = g_Player3D.Rotation.y;
		float delta = targetYawDeg - currentYaw;
		while (delta > 180.0f) delta -= 360.0f;
		while (delta < -180.0f) delta += 360.0f;

		const float rotateLerp = 0.2f; // 0..1�i1�ő�����]�j
		g_Player3D.Rotation.y = currentYaw + delta * rotateLerp;
	}
	// ���͖����̂Ƃ��͉�]��ύX���Ȃ��i�Ō�Ɍ����Ă���������ێ��j
}

void Player3D_Jump()
{
	if (Keyboard_IsKeyDownTrigger(JumpKey))
	{
		g_Player3D.Velocity.y += jumpPower;// �e�X�g
		if (isGround)
		{
			g_Player3D.Velocity.y += jumpPower;// ������ɏ�����^����
			// �󒆂ɂ����Ԃ�
			//g_Player3D.state = PLAYER3D_STATE_FALL;
		}
	}
}

void Player3D_Change()
{
	if (Keyboard_IsKeyDownTrigger(ChangeKey))
	{

	}
}

void Player3D_Action()
{
	if(Keyboard_IsKeyDownTrigger(ActionKey))
	{
		//�������i���̂����j


		//�Ɩ�����i���傤�߂��������j

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
}

PLAYER3D* GetPlayer3D()
{
	return &g_Player3D;
}

XMFLOAT3 Player3D_GetDetectHalfSize()
{
	return g_DetectHalfSize;
}