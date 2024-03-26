//***************************************************************************************
// LoadM3d.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "LoadM3d.h"

using namespace DirectX;

bool M3DLoader::LoadM3d(const std::string& _filename, std::vector<Vertex>& _vertices, std::vector<USHORT>& _indices, std::vector<Subset>& _subsets, std::vector<M3dMaterial>& _mats)
{
	std::ifstream fin(_filename);

	UINT numOfMaterials = 0;
	UINT numOfVertices = 0;
	UINT numOfTriangles = 0;
	UINT numOfBones = 0;
	UINT numOfAnimationClips = 0;

	std::string trash;

	if (fin)
	{
		fin >> trash;
		fin >> trash >> numOfMaterials;
		fin >> trash >> numOfVertices;
		fin >> trash >> numOfTriangles;
		fin >> trash >> numOfBones;
		fin >> trash >> numOfAnimationClips;

		ReadMaterials(fin, numOfMaterials, _mats);
		ReadSubsetTable(fin, numOfMaterials, _subsets);
		ReadVertices(fin, numOfVertices, _vertices);
		ReadTriangles(fin, numOfTriangles, _indices);

		return true;
	}

	return false;
}

bool M3DLoader::LoadM3d(const std::string& _filename, std::vector<SkinnedVertex>& _vertices, std::vector<USHORT>& _indices, std::vector<Subset>& _subsets, std::vector<M3dMaterial>& _mats, SkinnedData& _skinInfo)
{
	std::ifstream fin(_filename);

	UINT numOfMaterials = 0;
	UINT numOfVertices = 0;
	UINT numOfTriangles = 0;
	UINT numOfBones = 0;
	UINT numOfAnimationClips = 0;

	std::string trash;

	if (fin)
	{
		fin >> trash;
		fin >> trash >> numOfMaterials;
		fin >> trash >> numOfVertices;
		fin >> trash >> numOfTriangles;
		fin >> trash >> numOfBones;
		fin >> trash >> numOfAnimationClips;

		std::vector<XMFLOAT4X4> boneOffsets;
		std::vector<int> boneIndexToParentIndex;
		std::unordered_map<std::string, AnimationClip> animations;

		ReadMaterials(fin, numOfMaterials, _mats);
		ReadSubsetTable(fin, numOfMaterials, _subsets);
		ReadSkinnedVertices(fin, numOfVertices, _vertices);
		ReadTriangles(fin, numOfTriangles, _indices);
		ReadBoneOffsets(fin, numOfBones, boneOffsets);
		ReadBoneHierarchy(fin, numOfBones, boneIndexToParentIndex);
		ReadAnimationClips(fin, numOfBones, numOfAnimationClips, animations);

		_skinInfo.Set(boneIndexToParentIndex, boneOffsets, animations);

		return true;
	}

	return false;
}

void M3DLoader::ReadMaterials(std::ifstream& _fin, UINT _numOfMaterials, std::vector<M3dMaterial>& _mats)
{
	std::string trash;
	_mats.resize(_numOfMaterials);

	std::string diffuseMapName;
	std::string normalMapName;

	_fin >> trash;
	for (UINT i = 0; i < _numOfMaterials; i++)
	{
		_fin >> trash >> _mats[i].Name;
		_fin >> trash >> _mats[i].DiffuseAlbedo.x >> _mats[i].DiffuseAlbedo.y >> _mats[i].DiffuseAlbedo.z;
		_fin >> trash >> _mats[i].FresnelR0.x >> _mats[i].FresnelR0.y >> _mats[i].FresnelR0.z;
		_fin >> trash >> _mats[i].Roughness;
		_fin >> trash >> _mats[i].AlphaClip;
		_fin >> trash >> _mats[i].MaterialTypeName;
		_fin >> trash >> _mats[i].DiffuseMapName;
		_fin >> trash >> _mats[i].NormalMapName;
	}
	
}

void M3DLoader::ReadSubsetTable(std::ifstream& _fin, UINT _numOfSubsets, std::vector<Subset>& _subsets)
{
	std::string trash;
	_subsets.resize(_numOfSubsets);

	_fin >> trash;
	for (UINT i = 0; i < _numOfSubsets; i++)
	{
		_fin >> trash >> _subsets[i].Id;
		_fin >> trash >> _subsets[i].VertexStart;
		_fin >> trash >> _subsets[i].VertexCount;
		_fin >> trash >> _subsets[i].FaceStart;
		_fin >> trash >> _subsets[i].FaceCount;
	}
}

void M3DLoader::ReadVertices(std::ifstream& _fin, UINT _numOfVertices, std::vector<Vertex>& _vertices)
{
	std::string trash;
	_vertices.resize(_numOfVertices);

	_fin >> trash;
	for (UINT i = 0; i < _numOfVertices; i++)
	{
		_fin >> trash >> _vertices[i].Pos.x >> _vertices[i].Pos.y >> _vertices[i].Pos.z;
		_fin >> trash >> _vertices[i].TangentU.x >> _vertices[i].TangentU.y >> _vertices[i].TangentU.z >> _vertices[i].TangentU.w;
		_fin >> trash >> _vertices[i].Normal.x >> _vertices[i].Normal.y >> _vertices[i].Normal.z;
		_fin >> trash >> _vertices[i].TexC.x >> _vertices[i].TexC.y;
	}
}

void M3DLoader::ReadSkinnedVertices(std::ifstream& _fin, UINT _numOfVertices, std::vector<SkinnedVertex>& _vertices)
{
	std::string trash;
	_vertices.resize(_numOfVertices);

	_fin >> trash; 

	int boneIndices[4];
	float weights[4];

	for (UINT i = 0; i < _numOfVertices; ++i)
	{
		float speck;
		_fin >> trash >> _vertices[i].Pos.x >> _vertices[i].Pos.y >> _vertices[i].Pos.z;
		_fin >> trash >> _vertices[i].TangentU.x >> _vertices[i].TangentU.y >> _vertices[i].TangentU.z >> speck /*vertices[i].TangentU.w*/;
		_fin >> trash >> _vertices[i].Normal.x >> _vertices[i].Normal.y >> _vertices[i].Normal.z;
		_fin >> trash >> _vertices[i].TexC.x >> _vertices[i].TexC.y;
		_fin >> trash >> weights[0] >> weights[1] >> weights[2] >> weights[3];
		_fin >> trash >> boneIndices[0] >> boneIndices[1] >> boneIndices[2] >> boneIndices[3];

		_vertices[i].BoneWeights.x = weights[0];
		_vertices[i].BoneWeights.y = weights[1];
		_vertices[i].BoneWeights.z = weights[2];

		_vertices[i].BoneIndices[0] = (BYTE)boneIndices[0];
		_vertices[i].BoneIndices[1] = (BYTE)boneIndices[1];
		_vertices[i].BoneIndices[2] = (BYTE)boneIndices[2];
		_vertices[i].BoneIndices[3] = (BYTE)boneIndices[3];
	}
}

void M3DLoader::ReadTriangles(std::ifstream& _fin, UINT _numOfTriangles, std::vector<USHORT>& _indices)
{
	std::string trash;
	_indices.resize(_numOfTriangles * 3);

	_fin >> trash; 
	for (UINT i = 0; i < _numOfTriangles; ++i)
	{
		_fin >> _indices[i * 3 + 0] >> _indices[i * 3 + 1] >> _indices[i * 3 + 2];
	}
}

void M3DLoader::ReadBoneOffsets(std::ifstream& _fin, UINT _numOfBones, std::vector<DirectX::XMFLOAT4X4>& _boneOffsets)
{
	std::string trash;
	_boneOffsets.resize(_numOfBones);

	_fin >> trash;
	for (UINT i = 0; i < _numOfBones; ++i)
	{
		_fin >> trash >>
			_boneOffsets[i](0, 0) >> _boneOffsets[i](0, 1) >> _boneOffsets[i](0, 2) >> _boneOffsets[i](0, 3) >>
			_boneOffsets[i](1, 0) >> _boneOffsets[i](1, 1) >> _boneOffsets[i](1, 2) >> _boneOffsets[i](1, 3) >>
			_boneOffsets[i](2, 0) >> _boneOffsets[i](2, 1) >> _boneOffsets[i](2, 2) >> _boneOffsets[i](2, 3) >>
			_boneOffsets[i](3, 0) >> _boneOffsets[i](3, 1) >> _boneOffsets[i](3, 2) >> _boneOffsets[i](3, 3);
	}
}

void M3DLoader::ReadBoneHierarchy(std::ifstream& _fin, UINT _numOfBones, std::vector<int>& _boneIndexToParentIndex)
{
	std::string trash;
	_boneIndexToParentIndex.resize(_numOfBones);

	_fin >> trash; 
	for (UINT i = 0; i < _numOfBones; ++i)
	{
		_fin >> trash >> _boneIndexToParentIndex[i];
	}
}

void M3DLoader::ReadAnimationClips(std::ifstream& _fin, UINT _numOfBones, UINT _numOfAnimationsClips, std::unordered_map<std::string, AnimationClip>& _animations)
{
	std::string trash;
	_fin >> trash; 
	for (UINT clipIndex = 0; clipIndex < _numOfAnimationsClips; ++clipIndex)
	{
		std::string clipName;
		_fin >> trash >> clipName;
		_fin >> trash; 

		AnimationClip clip;
		clip.BoneAnimations.resize(_numOfBones);

		for (UINT boneIndex = 0; boneIndex < _numOfBones; ++boneIndex)
		{
			ReadBoneKeyframes(_fin, _numOfBones, clip.BoneAnimations[boneIndex]);
		}
		_fin >> trash;

		_animations[clipName] = clip;
	}
}

void M3DLoader::ReadBoneKeyframes(std::ifstream& _fin, UINT _numOfBones, BoneAnimation& _boneAnimation)
{
	std::string trash;
	UINT numOfKeyframes = 0;
	_fin >> trash >> trash >> numOfKeyframes;
	_fin >> trash;

	_boneAnimation.Keyframes.resize(numOfKeyframes);
	for (UINT i = 0; i < numOfKeyframes; ++i)
	{
		float t = 0.0f;
		XMFLOAT3 p(0.0f, 0.0f, 0.0f);
		XMFLOAT3 s(1.0f, 1.0f, 1.0f);
		XMFLOAT4 q(0.0f, 0.0f, 0.0f, 1.0f);
		_fin >> trash >> t;
		_fin >> trash >> p.x >> p.y >> p.z;
		_fin >> trash >> s.x >> s.y >> s.z;
		_fin >> trash >> q.x >> q.y >> q.z >> q.w;

		_boneAnimation.Keyframes[i].TimePos = t;
		_boneAnimation.Keyframes[i].Translation = p;
		_boneAnimation.Keyframes[i].Scale = s;
		_boneAnimation.Keyframes[i].RotationQuat = q;
	}

	_fin >> trash;
}
