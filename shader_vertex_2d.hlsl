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
    float Radius;
    float3 pad1;
};

cbuffer Buffer2 : register(b2)
{
    LIGHT Light; //C言語から渡されたデータが入っている
}

cbuffer Buffer3 : register(b3)
{
    float4x4 LightViewProj; // Light's VP matrix
}

cbuffer Buffer4 : register(b4)
{
    float3 ShadowLightPos;
    float ShadowPassMode;
    float ShadowRadius;
    float3 pad2;
    float ShadowIntensity;
};

cbuffer Buffer5 : register(b5)
{
    float4x4 Bones[100];
}

//入力用頂点構造体
struct VS_INPUT
{ //              V コロン！
    float4 posL : POSITION0; //頂点座標 オーでなくゼロ！
    float4 normal : NORMAL0;
    float4 color : COLOR0; //頂点カラー（R,G,B,A）
    float2 texcoord : TEXCOORD0;
    
    uint4 boneIndex : BONEINDEX0;
    float4 boneWeight : BONEWEIGHT0;
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
    
    float4 finalPos = vs_in.posL;
    float3 finalNormal = vs_in.normal;

    float totalWeight =
        vs_in.boneWeight.x +
        vs_in.boneWeight.y +
        vs_in.boneWeight.z +
        vs_in.boneWeight.w;
    
    if (totalWeight > 0.0f)
    {
        //vs_in.color = float4(1, 0, 0, 1);
        float4x4 bone0 = Bones[vs_in.boneIndex.x];
        float4x4 bone1 = Bones[vs_in.boneIndex.y];
        float4x4 bone2 = Bones[vs_in.boneIndex.z];
        float4x4 bone3 = Bones[vs_in.boneIndex.w];
        
        float w0 = vs_in.boneWeight.x / totalWeight;
        float w1 = vs_in.boneWeight.y / totalWeight;
        float w2 = vs_in.boneWeight.z / totalWeight;
        float w3 = vs_in.boneWeight.w / totalWeight;

        finalPos =
              mul(vs_in.posL, bone0) * w0
            + mul(vs_in.posL, bone1) * w1
            + mul(vs_in.posL, bone2) * w2
            + mul(vs_in.posL, bone3) * w3;

        finalNormal =
              mul(vs_in.normal.xyz, (float3x3) bone0) * w0
            + mul(vs_in.normal.xyz, (float3x3) bone1) * w1
            + mul(vs_in.normal.xyz, (float3x3) bone2) * w2
            + mul(vs_in.normal.xyz, (float3x3) bone3) * w3;

        finalNormal = normalize(finalNormal);
    }

    
    
    // Transform to clip space
    vs_out.posH = mul(finalPos, mtx);
    vs_out.color = vs_in.color;
    vs_out.texcoord = vs_in.texcoord;
    
    // World position for lighting/shadows
    float4 worldPos = mul(finalPos, World);
    vs_out.posWorld = worldPos.xyz;
    vs_out.posLight = mul(worldPos, LightViewProj);
    
    // Transform normal to world space
    float3 worldNormal = mul(finalNormal, (float3x3) World);
    vs_out.normal = normalize(worldNormal);
    
    return vs_out;
}


////=============================================================================
//// 頂点シェーダ
////=============================================================================
//float4 main(in float4 posL : POSITION0 ) : SV_POSITION
//{
//	return mul(posL, mtx);//頂点座標＊mtx（変換行列）
//}
