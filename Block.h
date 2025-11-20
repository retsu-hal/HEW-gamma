#pragma once

//=========================================================================================================
//構造体
//=========================================================================================================
class BLOCK
{
	public:
		bool Enable;	//ブロックがあるかないか
		bool Erase;		//消滅予定フラグ
		int Type;		//ブロック種別
};

enum BLOCK_STATE
{
	BLOCK_STATE_IDLE,			//ひま
	BLOCK_STATE_ERASE_IDLE,		//消滅中
	BLOCK_STATE_STACK_IDLE,		//落下中

};

//=========================================================================================================
//プロトタイプ宣言
//=========================================================================================================
//メイン処理関数
void Block_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
void Block_Finalize();
void Block_Update();
void Block_Draw();

//サブ処理関数
void Block_SetBlock(int x, int y, int Type);
BLOCK Block_GetBlock(int x, int y);
void Block_EraseBlock();
void Block_StackBlock();

