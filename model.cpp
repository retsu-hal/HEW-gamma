//model.cpp
#define NOMINMAX

#include "model.h"
#include "debug_ostream.h"


XMMATRIX AiToXM(const aiMatrix4x4& m)
{
	return XMMatrixSet(
		m.a1, m.b1, m.c1, m.d1,
		m.a2, m.b2, m.c2, m.d2,
		m.a3, m.b3, m.c3, m.d3,
		m.a4, m.b4, m.c4, m.d4
	);
}

MODEL* ModelLoad(const char* FileName, bool preTransformVertices)
{
	MODEL* model = new MODEL;

	unsigned int flags =
		aiProcessPreset_TargetRealtime_MaxQuality |
		aiProcess_ConvertToLeftHanded |
		aiProcess_GenSmoothNormals |
		aiProcess_LimitBoneWeights |
		aiProcess_CalcTangentSpace |
		aiProcess_Triangulate;

	// For static models (walls, objects), bake transforms from Maya
	if (preTransformVertices)
	{
		flags |= aiProcess_PreTransformVertices;
	}

	model->AiScene = aiImportFile(FileName, flags);

	hal::dout << "Loading model: " << FileName << std::endl;

	if (!model->AiScene) {
		hal::dout << "Failed to load model: " << aiGetErrorString() << std::endl;
		delete model;
		return nullptr;
	}

	XMMATRIX rootTransform = AiToXM(model->AiScene->mRootNode->mTransformation);

	XMVECTOR det;
	model->GlobalInverse = XMMatrixInverse(&det, rootTransform);
	if (XMVector4Equal(det, XMVectorZero())) {
		hal::dout << "Warning: Root transform is not invertible, using identity." << std::endl;
		model->GlobalInverse = XMMatrixIdentity();
	}

	assert(model->AiScene);

	model->VertexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];//頂点データポインター
	model->IndexBuffer = new ID3D11Buffer * [model->AiScene->mNumMeshes];//インデックスデータポインター


	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// 頂点バッファ生成
		{
			Vertex3D* vertex = new Vertex3D[mesh->mNumVertices];//頂点数分の配列領域作成

			for (unsigned int v = 0; v < mesh->mNumVertices; v++)
			{
				vertex[v].position = XMFLOAT3(
					mesh->mVertices[v].x,
					mesh->mVertices[v].y,
					mesh->mVertices[v].z);
				vertex[v].normal = XMFLOAT3(
					mesh->mNormals[v].x,
					mesh->mNormals[v].y,
					mesh->mNormals[v].z);
				//vertex[v].position = XMFLOAT3(mesh->mVertices[v].x, -mesh->mVertices[v].z, mesh->mVertices[v].y);
				vertex[v].texCoord = XMFLOAT2(mesh->mTextureCoords[0][v].x, mesh->mTextureCoords[0][v].y);
				vertex[v].color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
				//vertex[v].normal = XMFLOAT3(mesh->mNormals[v].x, -mesh->mNormals[v].z, mesh->mNormals[v].y);
				for (int i = 0; i < 4; i++)
				{
					vertex[v].boneIndex[i] = 0;
					vertex[v].boneWeight[i] = 0.0f;
				}
			}


			for (UINT b = 0; b < mesh->mNumBones; b++)
			{
				aiBone* bone = mesh->mBones[b];
				std::string boneName = bone->mName.C_Str();

				UINT boneIndex = 0;

				if (model->BoneMap.find(boneName) == model->BoneMap.end())
				{
					boneIndex = (UINT)model->Bones.size();
					model->BoneMap[boneName] = boneIndex;

					BoneInfo bi;
					bi.offset = AiToXM(bone->mOffsetMatrix);

					model->Bones.push_back(bi);
				}
				else
				{
					boneIndex = model->BoneMap[boneName];
				}

				// Assign weights to vertices
				for (UINT w = 0; w < bone->mNumWeights; w++)
				{
					UINT vtx = bone->mWeights[w].mVertexId;
					float weight = bone->mWeights[w].mWeight;

					for (int i = 0; i < 4; i++)
					{
						if (vertex[vtx].boneWeight[i] == 0.0f)
						{
							vertex[vtx].boneIndex[i] = boneIndex;
							vertex[vtx].boneWeight[i] = weight;
							break;
						}
					}
				}
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DYNAMIC;
			bd.ByteWidth = sizeof(Vertex3D) * mesh->mNumVertices;
			bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = vertex;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->VertexBuffer[m]);

			delete[] vertex;
		}

		// インデックスバッファ生成
		{
			unsigned int* index = new unsigned int[mesh->mNumFaces * 3];//ポリゴン数数*3

			for (unsigned int f = 0; f < mesh->mNumFaces; f++)
			{
				const aiFace* face = &mesh->mFaces[f];

				assert(face->mNumIndices == 3);

				index[f * 3 + 0] = face->mIndices[0];
				index[f * 3 + 1] = face->mIndices[1];
				index[f * 3 + 2] = face->mIndices[2];
			}

			D3D11_BUFFER_DESC bd;
			ZeroMemory(&bd, sizeof(bd));
			bd.Usage = D3D11_USAGE_DEFAULT;
			bd.ByteWidth = sizeof(unsigned int) * mesh->mNumFaces * 3;
			bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
			bd.CPUAccessFlags = 0;

			D3D11_SUBRESOURCE_DATA sd;
			ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = index;

			Direct3D_GetDevice()->CreateBuffer(&bd, &sd, &model->IndexBuffer[m]);

			delete[] index;
		}

	}

	//テクスチャ読み込み
	for (int i = 0; i < model->AiScene->mNumTextures; i++)
	{
		aiTexture* aitexture = model->AiScene->mTextures[i];

		ID3D11ShaderResourceView* texture;
		TexMetadata metadata;
		ScratchImage image;
		LoadFromWICMemory(aitexture->pcData, aitexture->mWidth, WIC_FLAGS_NONE, &metadata, image);
		CreateShaderResourceView(Direct3D_GetDevice(), image.GetImages(), image.GetImageCount(), metadata, &texture);
		assert(texture);

		model->Texture[aitexture->mFilename.data] = texture;
	}



	return model;
}


void ModelRelease(MODEL* model)
{
	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		model->VertexBuffer[m]->Release();
		model->IndexBuffer[m]->Release();
	}

	delete[] model->VertexBuffer;
	delete[] model->IndexBuffer;


	for (std::pair<const std::string, ID3D11ShaderResourceView*> pair : model->Texture)
	{
		pair.second->Release();
	}


	aiReleaseImport(model->AiScene);


	delete model;
}

void ModelDraw(MODEL* model)
{
	// プリミティブトポロジ設定
	Direct3D_GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	for (unsigned int m = 0; m < model->AiScene->mNumMeshes; m++)
	{
		aiMesh* mesh = model->AiScene->mMeshes[m];

		// テクスチャ設定
		aiString texture;
		aiMaterial* aimaterial = model->AiScene->mMaterials[mesh->mMaterialIndex];
		aimaterial->GetTexture(aiTextureType_DIFFUSE, 0, &texture);

		if (texture != aiString(""))
			Direct3D_GetDeviceContext()->PSSetShaderResources(0, 1, &model->Texture[texture.data]);

		// 頂点バッファ設定
		UINT stride = sizeof(Vertex3D);
		UINT offset = 0;
		Direct3D_GetDeviceContext()->IASetVertexBuffers(0, 1, &model->VertexBuffer[m], &stride, &offset);

		// インデックスバッファ設定
		Direct3D_GetDeviceContext()->IASetIndexBuffer(model->IndexBuffer[m], DXGI_FORMAT_R32_UINT, 0);

		// ポリゴン描画
		Direct3D_GetDeviceContext()->DrawIndexed(mesh->mNumFaces * 3, 0, 0);
	}
}

const aiNodeAnim* FindNodeAnim(const aiAnimation* animation, const std::string& nodeName)
{
	for (UINT i = 0; i < animation->mNumChannels; i++)
	{
		const aiNodeAnim* channel = animation->mChannels[i];
		if (channel->mNodeName.C_Str() == nodeName)
			return channel;
	}
	return nullptr;
}

XMMATRIX InterpolatePosition(float time, const aiNodeAnim* channel)
{
	if (channel->mNumPositionKeys == 1)
	{
		auto& p = channel->mPositionKeys[0].mValue;
		return XMMatrixTranslation(p.x, p.y, p.z);
	}

	UINT i = 0;
	while (i + 1 < channel->mNumPositionKeys &&
		time > (float)channel->mPositionKeys[i + 1].mTime)
		i++;

	UINT j = i + 1;
	float dt =
		(float)(channel->mPositionKeys[j].mTime -
			channel->mPositionKeys[i].mTime);

	float factor =
		(time - (float)channel->mPositionKeys[i].mTime) / dt;

	auto& a = channel->mPositionKeys[i].mValue;
	auto& b = channel->mPositionKeys[j].mValue;

	XMVECTOR A = XMVectorSet(a.x, a.y, a.z, 0);
	XMVECTOR B = XMVectorSet(b.x, b.y, b.z, 0);
	XMVECTOR P = XMVectorLerp(A, B, factor);

	return XMMatrixTranslationFromVector(P);
}

XMMATRIX InterpolateRotation(float time, const aiNodeAnim* channel)
{
	if (channel->mNumRotationKeys == 1)
	{
		auto& q = channel->mRotationKeys[0].mValue;
		return XMMatrixRotationQuaternion(
			XMVectorSet(q.x, q.y, q.z, q.w));
	}

	UINT i = 0;
	while (i + 1 < channel->mNumRotationKeys &&
		time > (float)channel->mRotationKeys[i + 1].mTime)
		i++;

	UINT j = i + 1;
	float dt =
		(float)(channel->mRotationKeys[j].mTime -
			channel->mRotationKeys[i].mTime);

	float factor =
		(time - (float)channel->mRotationKeys[i].mTime) / dt;

	auto& a = channel->mRotationKeys[i].mValue;
	auto& b = channel->mRotationKeys[j].mValue;

	XMVECTOR A = XMVectorSet(a.x, a.y, a.z, a.w);
	XMVECTOR B = XMVectorSet(b.x, b.y, b.z, b.w);
	XMVECTOR Q = XMQuaternionSlerp(A, B, factor);

	return XMMatrixRotationQuaternion(Q);
}

XMMATRIX InterpolateScale(float time, const aiNodeAnim* channel)
{
	if (channel->mNumScalingKeys == 1)
	{
		auto& s = channel->mScalingKeys[0].mValue;
		return XMMatrixScaling(s.x, s.y, s.z);
	}

	UINT i = 0;
	while (i + 1 < channel->mNumScalingKeys &&
		time > (float)channel->mScalingKeys[i + 1].mTime)
		i++;

	UINT j = i + 1;
	float dt =
		(float)(channel->mScalingKeys[j].mTime -
			channel->mScalingKeys[i].mTime);

	float factor =
		(time - (float)channel->mScalingKeys[i].mTime) / dt;

	auto& a = channel->mScalingKeys[i].mValue;
	auto& b = channel->mScalingKeys[j].mValue;

	XMVECTOR A = XMVectorSet(a.x, a.y, a.z, 0);
	XMVECTOR B = XMVectorSet(b.x, b.y, b.z, 0);
	XMVECTOR S = XMVectorLerp(A, B, factor);

	return XMMatrixScalingFromVector(S);
}

void ReadNodeHierarchy(
	MODEL* model,
	float animTime,
	const aiNode* node,
	const XMMATRIX& parentTransform)
{
	//XMMATRIX nodeTransform = AiToXM(node->mTransformation);
	XMMATRIX nodeTransform = XMMatrixIdentity();

	const aiAnimation* animation = model->AiScene->mAnimations[0];
	const aiNodeAnim* channel =
		FindNodeAnim(animation, node->mName.C_Str());

	if (channel)
	{
		XMMATRIX T = InterpolatePosition(animTime, channel);
		XMMATRIX R = InterpolateRotation(animTime, channel);
		XMMATRIX S = InterpolateScale(animTime, channel);

		//nodeTransform = T * R * S;
		nodeTransform = S * R * T;
	}

	//XMMATRIX globalTransform = parentTransform * nodeTransform;
	XMMATRIX globalTransform = nodeTransform * parentTransform;

	auto it = model->BoneMap.find(node->mName.C_Str());
	if (it != model->BoneMap.end())
	{
		UINT index = it->second;

		//model->Bones[index].finalTransform =
		//	model->GlobalInverse *
		//	globalTransform *
		//	model->Bones[index].offset;

		model->Bones[index].finalTransform =
			model->Bones[index].offset *
			globalTransform;

		//model->Bones[index].finalTransform =
		//	model->GlobalInverse *
		//	model->Bones[index].offset *
		//	globalTransform;
	}

	for (UINT i = 0; i < node->mNumChildren; i++)
	{
		ReadNodeHierarchy(
			model,
			animTime,
			node->mChildren[i],
			globalTransform);
	}
}

void ModelUpdateAnimation(MODEL* model, float deltaTime)
{
	if (!model->AiScene->HasAnimations() || model->Bones.empty())
		return;

	const aiAnimation* anim = model->AiScene->mAnimations[0];

	if (!model->LoopAnimation && model->AnimationFinished)
	{
		float endTime = (model->CustomEndTime > 0.0f)
			? model->CustomEndTime
			: (float)anim->mDuration;
		ReadNodeHierarchy(model, endTime, model->AiScene->mRootNode, XMMatrixIdentity());
		return;
	}

	//float ticksPerSecond =
	//	anim->mTicksPerSecond != 0.0f ?
	//	(float)anim->mTicksPerSecond : 25.0f;

	float ticksPerSecond = (float)anim->mTicksPerSecond;
	if (ticksPerSecond < 1.0f)
		ticksPerSecond = 30.0f;

	model->AnimationTime += deltaTime * ticksPerSecond;

	float duration = (model->CustomEndTime > 0.0f)
		? model->CustomEndTime
		: (float)anim->mDuration;

	if (duration <= 0.0f)
		return;

	float time;

	if (model->LoopAnimation)
	{
		// Loop: wrap time around
		time = fmod(model->AnimationTime, duration);
	}
	else
	{
		// Play once: clamp at the end
		if (model->AnimationTime >= duration)
		{
			model->AnimationTime = duration;
			model->AnimationFinished = true;
			time = duration;
		}
		else
		{
			time = model->AnimationTime;
		}
	}

	ReadNodeHierarchy(
		model,
		time,
		model->AiScene->mRootNode,
		XMMatrixIdentity());
}

void ModelResetAnimation(MODEL* model)
{
	if (model == NULL) return;
	model->AnimationTime = 0.0f;
	model->AnimationFinished = false;
}

