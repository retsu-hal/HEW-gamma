/*==============================================================================

   2D描画用頂点シェーダー [shader_vertex_2d.hlsl]
--------------------------------------------------------------------------------

==============================================================================*/

// 定数バッファ
cbuffer Buffer0 : register(b0)
{
    float4x4 mtx; //C言語から渡されたデータが入っている
}

cbuffer Buffer1 : register(b1)
{
    float4x4 World; //C言語から渡されたデータが入っている
}

struct LIGHT
{
    bool Enable;
    bool3 dummy;
    float4 Dir;
    float4 Diffuse;
    float4 Ambient;
};

cbuffer Buffer2 : register(b2)
{
    LIGHT Light; //C言語から渡されたデータが入っている
}

cbuffer Buffer3 : register(b3)
{
    float4x4 LightViewProj; // Light's VP matrix
}

//入力用頂点構造体
struct VS_INPUT
{ //              V コロン！
    float4 posL : POSITION0; //頂点座標 オーでなくゼロ！
    float4 normal : NORMAL0;
    float4 color : COLOR0; //頂点カラー（R,G,B,A）
    float2 texcoord : TEXCOORD0;
};

//出力用頂点構造体
struct VS_OUTPUT
{
    float4 posH : SV_POSITION; //変換済頂点座標
    float4 color : COLOR0; //頂点カラー
    float2 texcoord : TEXCOORD0;
    float4 posLight : TEXCOORD1;
    float3 normal : TEXCOORD2;
    float3 posWorld : TEXCOORD3;
};

VS_OUTPUT main(VS_INPUT vs_in)
{
    VS_OUTPUT vs_out; //出力用構造体変数
    
    //頂点を行列で変換
    vs_out.posH = mul(vs_in.posL, mtx);
    //頂点カラーはそのまま出力
    vs_out.color = vs_in.color;

    vs_out.texcoord = vs_in.texcoord;
    
        // Transform to world, then to light's VP (for shadow sampling)
    float4 worldPos = mul(vs_in.posL, World);
    vs_out.posWorld = worldPos.xyz;
    vs_out.posLight = mul(worldPos, LightViewProj);

    // Pass world normal for slope-scaled bias
    float4 worldNormal = mul(float4(vs_in.normal.xyz, 0.0f), World);
    vs_out.normal = normalize(worldNormal.xyz);
    

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
