//#include "ball.h"
//#include "keyboard.h"
//#include "Camera.h"
//#include "shader.h"
//#include "Collision.h"
////=========================================================================================================
//// マクロ定義
////=========================================================================================================
//#define BALL_SPEEDMAX (2.0f)		//最大速度
//
////=========================================================================================================
//// グローバル変数
////=========================================================================================================
//BALL g_Ball;
//ID3D11Device* g_pDevice;
//ID3D11DeviceContext* g_pContext;
//float g_StopTime=0.0f;
//
////=========================================================================================================
//// 初期化処理
////=========================================================================================================
//void Ball_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
//{
//	g_pDevice = pDevice;
//	g_pContext = pContext;
//
//	g_Ball.Model= ModelLoad("asset\\model\\test.fbx");
//	g_Ball.Position = XMFLOAT3(0.0f,1.2f,0.0f);
//	g_Ball.Rotation = XMFLOAT3(0.0f,0.0f,0.0f);
//	g_Ball.Scaling = XMFLOAT3(1.0f,1.0f,1.0f);
//	g_Ball.Velocity = XMFLOAT3(0.0f,0.0f,0.0f);
//	g_Ball.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
//	g_Ball.state = BALL_STATE_MOVE;
//	g_StopTime = 0.0f;
//	g_Ball.Quaternion = XMQuaternionIdentity();
//	g_Ball.Axis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
//}
//
////=========================================================================================================
//// 終了処理
////=========================================================================================================
//void Ball_Finalize()
//{
//	ModelRelease(g_Ball.Model);
//}
//
////=========================================================================================================
//// 更新処理
////=========================================================================================================
//void Ball_Update()
//{
//	switch (g_Ball.state)
//	{
//	case BALL_STATE::BALL_STATE_IDLE:
//		Ball_Idle();
//		break;
//	case BALL_STATE::BALL_STATE_MOVE:
//		Ball_Move();
//		break;
//	case BALL_STATE::BALL_STATE_DIRECTION:
//		Ball_Direction();
//		break;
//	case BALL_STATE::BALL_STATE_POWER:
//		Ball_Power();
//		break;
//	case BALL_STATE::BALL_STATE_RESPAWN:
//		Ball_Respawn();
//		Camera_Initialize();
//		break;
//	}
//	
//}
//
//
////=========================================================================================================
//// 描画処理
////=========================================================================================================
//void Ball_Draw()
//{
//	// ワールド行列の作成
//	//スケーリング行列の作成
//	XMMATRIX ScalingMatrix = XMMatrixScaling(g_Ball.Scaling.x, g_Ball.Scaling.y, g_Ball.Scaling.z);
//	//平行移動行列の作成
//	XMMATRIX TranslationMatrix = XMMatrixTranslation(g_Ball.Position.x, g_Ball.Position.y, g_Ball.Position.z);
//	//回転行列の作成
//	//XMMATRIX RotationMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(g_Ball.Rotation.x), XMConvertToRadians(g_Ball.Rotation.y), XMConvertToRadians(g_Ball.Rotation.z));
//	XMVECTOR Quaternion = XMQuaternionRotationAxis(g_Ball.Axis, XMConvertToRadians(g_Ball.Speed));
//	g_Ball.Quaternion = XMQuaternionMultiply(g_Ball.Quaternion, Quaternion);
//	XMMATRIX RotationMatrix = XMMatrixRotationQuaternion(g_Ball.Quaternion);
//	//計算の順番「スケール*回転*平行移動」
//	XMMATRIX WorldMatrix = ScalingMatrix * RotationMatrix * TranslationMatrix;
//
//	//プロジェクション行列作成
//	XMMATRIX Projection = GetProjectionMatrix();
//
//	//ビュー行列作成
//	XMMATRIX View = GetViewMatrix();
//
//	//最終的な変換行列を作成	順番に注意！！
//	XMMATRIX WVP = WorldMatrix * View * Projection;
//
//	//変換行列を頂点シェーダへセット
//	Shader_SetWorldMatrix(WorldMatrix);
//	Shader_SetMatrix(WVP);
//
//	//描画リクエスト
//	ModelDraw(g_Ball.Model);
//}
//
////=========================================================================================================
//// ゲッター
////=========================================================================================================
//XMFLOAT3 GetBallPositon()
//{
//	return g_Ball.Position;
//}
//
////=========================================================================================================
//// stateごとの処理（Idle状態）
////=========================================================================================================
//void Ball_Idle()
//{
//
//}
//
////=========================================================================================================
//// stateごとの処理（Move状態）
////=========================================================================================================
//void Ball_Move()
//{
//	g_Ball.Velocity.x += g_Ball.Acceleration.x; //重力
//	g_Ball.Velocity.y += g_Ball.Acceleration.y; //重力
//	g_Ball.Velocity.z += g_Ball.Acceleration.z; //重力
//
//	g_Ball.Position.x += g_Ball.Velocity.x;
//	g_Ball.Position.y += g_Ball.Velocity.y;
//	g_Ball.Position.z += g_Ball.Velocity.z;
//
//	g_Ball.Velocity.x *= 0.98f;		//好みで減衰させる
//	//g_Ball.Velocity.y *= 0.98f;		//好みで減衰させる
//	g_Ball.Velocity.z *= 0.98f;		//好みで減衰させる
//
//	//落下チェック
//	if (g_Ball.Position.y < -10.0f)
//	{
//		g_Ball.state = BALL_STATE::BALL_STATE_RESPAWN;
//		return;
//	}
//	//静止チェック
//	float len = (g_Ball.Velocity.x * g_Ball.Velocity.x + g_Ball.Velocity.y * g_Ball.Velocity.y + g_Ball.Velocity.z * g_Ball.Velocity.z);
//	if (len <= 0.0002f)
//	{
//		g_StopTime++;
//		if (g_StopTime > (60.0f * 2))
//		{
//			g_Ball.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
//			g_Ball.state = BALL_STATE::BALL_STATE_DIRECTION;
//			g_StopTime = 0.0f;
//		}
//	}
//
//	float hit = Player3DField_Collision();
//
//	//回転軸と回転量を決定
//	g_Ball.Speed = sqrtf(g_Ball.Velocity.x * g_Ball.Velocity.x + g_Ball.Velocity.z * g_Ball.Velocity.z) * 110.0f;
//
//	if (g_Ball.Speed >= 0.0002f)
//	{
//		XMVECTOR vec1, vec2;
//		vec1 = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
//		vec2 = XMLoadFloat3(&g_Ball.Velocity);
//		vec2 = XMVector3Normalize(vec2);
//		g_Ball.Axis = XMVector3Cross(vec1, vec2);
//	}
//	else
//	{
//		g_Ball.Axis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
//		g_Ball.Speed = 0.0f;
//	}
//}
//
////=========================================================================================================
//// stateごとの処理（Power状態）
////=========================================================================================================
//void Ball_Power()
//{
//	float power = BALL_SPEEDMAX * 0.12f;
//
//	g_Ball.Velocity.x *= power;
//	g_Ball.Velocity.y *= power;
//	g_Ball.Velocity.z *= power;
//
//	g_Ball.state = BALL_STATE::BALL_STATE_MOVE;
//}
//
////=========================================================================================================
//// stateごとの処理（Direction状態）
////=========================================================================================================
//void Ball_Direction()
//{
//	//キーを押したら転がる
//	if (Keyboard_IsKeyDownTrigger(KK_F))
//	{
//		//カメラの向きを取得
//		XMFLOAT3 Cap = GetCameraAtPosition();
//		XMFLOAT3 Cp = GetCameraPosition();
//		XMFLOAT3 Direction;
//		Direction.x = Cap.x - Cp.x;
//		Direction.y = 0.0f;
//		Direction.z = Cap.z - Cp.z;
//
//		//正規化
//		float len = sqrtf(Direction.x * Direction.x + Direction.y * Direction.y + Direction.z * Direction.z);
//		Direction.x /= len;
//		Direction.y /= len;
//		Direction.z /= len;
//
//		g_Ball.Velocity = Direction;
//		g_Ball.state = BALL_STATE::BALL_STATE_POWER;
//	}
//}
//
////=========================================================================================================
//// stateごとの処理（Respawn状態）
////=========================================================================================================
//void Ball_Respawn()
//{
//	g_Ball.Position = XMFLOAT3(0.0f, 1.2f, 0.0f);
//	g_Ball.Rotation = XMFLOAT3(0.0f, 0.0f, 0.0f);
//	g_Ball.Scaling = XMFLOAT3(1.0f, 1.0f, 1.0f);
//	g_Ball.Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
//	g_Ball.Acceleration = XMFLOAT3(0.0f, -9.8f / 600.0f * 0.5f, 0.0f);
//	g_Ball.state = BALL_STATE_MOVE;
//	g_StopTime = 0.0f;
//	g_Ball.Quaternion = XMQuaternionIdentity();
//	g_Ball.Axis = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
//}
//
//BALL* GetBall()
//{
//	return &g_Ball;
//}
