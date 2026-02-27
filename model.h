//model.h
#pragma once
//#define NOMINMAX
#include <unordered_map>

#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include "assimp/matrix4x4.h"
#pragma comment (lib, "assimp-vc143-mt.lib")

#include	"d3d11.h"
#include	"DirectXMath.h"
using namespace DirectX;
#include	"direct3d.h"

struct BoneInfo
{
	XMMATRIX offset;        // inverse bind pose
	XMMATRIX finalTransform;
};

struct MODEL
{
	const aiScene* AiScene = nullptr;

	ID3D11Buffer** VertexBuffer;
	ID3D11Buffer** IndexBuffer;

	std::unordered_map<std::string, ID3D11ShaderResourceView*> Texture;
	std::unordered_map<std::string, UINT> BoneMap;
	std::vector<BoneInfo> Bones;

	float AnimationTime = 0.0f;
	bool LoopAnimation = true;       // true = loop, false = play once
	bool AnimationFinished = false;   // flag to check if done

	float CustomEndTime = 0.0f;

	XMMATRIX GlobalInverse;
};

MODEL* ModelLoad(const char* FileName, bool preTransformVertices = false);
//if use true mean it will follow maya stats
//if use false mean it become defult in maya
void ModelRelease(MODEL* model);
void ModelDraw(MODEL* model);
void ModelResetAnimation(MODEL* model);

const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName);
XMMATRIX InterpolatePosition(float time, const aiNodeAnim* channel);
XMMATRIX InterpolateRotation(float time, const aiNodeAnim* channel);
XMMATRIX InterpolateScale(float time, const aiNodeAnim* channel);

void ReadNodeHierarchy(
	MODEL* model,
	float animTime,
	const aiNode* node,
	const XMMATRIX& parentTransform);

void ModelUpdateAnimation(MODEL* model, float deltaTime);