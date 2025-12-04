/*==============================================================================

   2D描画用頂点シェーダー [shader_vertex_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer Buffer0 : register(b0)
{
float4x4 mtx;	//C言語から渡されたデータが入っている
}

cbuffer Buffer1 : register(b1)
{
float4x4 World;	//C言語から渡されたデータが入っている
}

struct LIGHT
{
    bool Enable;
    bool3 dummy;
    float4 Direction;
    float4 Diffuse;
    float4 Ambient;
};

cbuffer Buffer2 : register(b2)
{
    LIGHT Light; //C言語から渡されたデータが入っている
}

// 最大ボーン数は仮で64
cbuffer BoneBuffer : register(b3)
{
    float4x4 BoneMatrix[64];
}

//入力用頂点構造体
struct VS_INPUT
{//              V コロン！
    float4  posL : POSITION0; //頂点座標 オーでなくゼロ！
    float4 normal : NORMAL0;    //法線
    float4  color : COLOR0;   //頂点カラー（R,G,B,A）
    float2 texcoord : TEXCOORD0;
    
    uint4 boneIndex : BONEINDEX0;
    float4 boneWeight : BONEWEIGHT0;
};

//出力用頂点構造体
struct VS_OUTPUT
{
    float4  posH : SV_POSITION;     //変換済頂点座標
    float4  color : COLOR0;         //頂点カラー
    float2 texcoord : TEXCOORD0;
};

// スキニング処理
float4 SkinPosition(VS_INPUT input)
{
    float4 pos = float4(0, 0, 0, 0);

    for (int i = 0; i < 4; i++)
    {
        pos += mul(input.posL, BoneMatrix[input.boneIndex[i]]) * input.boneWeight[i];
    }

    return pos;
}

float4 SkinNormal(VS_INPUT input)
{
    float4 nor = float4(0, 0, 0, 0);

    for (int i = 0; i < 4; i++)
    {
        nor += mul(float4(input.normal.xyz, 0), BoneMatrix[input.boneIndex[i]]) * input.boneWeight[i];
    }

    return normalize(nor);
}


VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out;   //出力用構造体変数
    
    //頂点を行列で変換
    vs_out.posH = mul(vs_in.posL, mtx);
    //頂点カラーはそのまま出力
    vs_out.color = vs_in.color;

    vs_out.texcoord = vs_in.texcoord;

    if(Light.Enable == true)
    {
        //法線をワールド変換
        float4 normal = float4(vs_in.normal.xyz, 0.0f);
        normal = mul(normal, World);        //normalをWorld行列で変換
        normal = normalize(normal);         //normalを正規化
        
        //ライティング
        float light = -dot(normal.xyz, Light.Direction.xyz);
        light = saturate(light);
        vs_out.color.rgb *= light;
        vs_out.color.rgb += Light.Ambient.rgb;
        

    }


    //結果を出力する
    return vs_out;
}



////=============================================================================
//// 頂点シェーダ
////=============================================================================
//float4 main(in float4 posL : POSITION0 ) : SV_POSITION
//{
//	return mul(posL, mtx);//頂点座標＊mtx（変換行列）
//}
