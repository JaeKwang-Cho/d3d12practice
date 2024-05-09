//***************************************************************************************
// LoadM3d.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************
#pragma once

#include "SkinnedData.h"

class M3DLoader
{
public:
    struct Vertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT2 TexC;
        DirectX::XMFLOAT4 TangentU;
    };

    struct SkinnedVertex
    {
        DirectX::XMFLOAT3 Pos;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT2 TexC;
        DirectX::XMFLOAT3 TangentU;
        DirectX::XMFLOAT3 BoneWeights;
        BYTE BoneIndices[4];
        int boneCount = 0;
    };

    struct Subset
    {
        UINT Id = -1;
        UINT VertexStart = 0;
        UINT VertexCount = 0;
        UINT FaceStart = 0;
        UINT FaceCount = 0;
    };

    struct M3dMaterial
    {
        std::string Name;

        DirectX::XMFLOAT4 DiffuseAlbedo = { 1.f, 1.f, 1.f, 1.f };
        DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
        float Roughness = 0.8f;
        bool AlphaClip = false;

        std::string MaterialTypeName;
        std::string DiffuseMapName;
        std::string NormalMapName;
    };

    bool LoadM3d(
        const std::string& _filename,
        std::vector<Vertex>& _vertices,
        std::vector<USHORT>& _indices,
        std::vector<Subset>& _subsets,
        std::vector<M3dMaterial>& _mats
    );

    bool LoadM3d(
        const std::string& _filename,
        std::vector<SkinnedVertex>& _vertices,
        std::vector<USHORT>& _indices,
        std::vector<Subset>& _subsets,
        std::vector<M3dMaterial>& _mats,
        SkinnedData& _skinInfo
    );

private:
    void ReadMaterials(std::ifstream& _fin, UINT _numOfMaterials, std::vector<M3dMaterial>& _mats);
    void ReadSubsetTable(std::ifstream& _fin, UINT _numOfSubsets, std::vector<Subset>& _subsets);
    void ReadVertices(std::ifstream& _fin, UINT _numOfVertices, std::vector<Vertex>& _vertices);
    void ReadSkinnedVertices(std::ifstream& _fin, UINT _numOfVertices, std::vector<SkinnedVertex>& _vertices);
    void ReadTriangles(std::ifstream& _fin, UINT _numOfTriangles, std::vector<USHORT>& _indices);
    void ReadBoneOffsets(std::ifstream& _fin, UINT _numOfBones, std::vector<DirectX::XMFLOAT4X4>& _boneOffsets);
    void ReadBoneHierarchy(std::ifstream& _fin, UINT _numOfBones, std::vector<int>& _boneIndexToParentIndex);
    void ReadAnimationClips(std::ifstream& _fin, UINT _numOfBones, UINT _numOfAnimationsClips, std::unordered_map<std::string, AnimationClip>& _animations);
    void ReadBoneKeyframes(std::ifstream& _fin, UINT _numOfBones, BoneAnimation& _boneAnimation);

};

