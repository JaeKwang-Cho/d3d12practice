/****************************************************************************************

Copyright (C) 2015 Autodesk, Inc.
All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement
provided at the time of installation or download, or which otherwise accompanies
this software in either electronic or hard copy form.

공부용으로 한글 주석 + 개조 중입니다.
****************************************************************************************/

#pragma once

#include <fbxsdk.h>
#include <vector>
#include <DirectXMath.h>

struct ColorChannel
{
	ColorChannel() : textureName(0)
	{
		color = DirectX::XMFLOAT4();
	}

	unsigned int textureName;
	DirectX::XMFLOAT4 color;
};

struct FbxMaterial {
	ColorChannel emissiveColor;
	ColorChannel ambientColor;
	ColorChannel diffuseColor;
	ColorChannel specularColor;
	float shininess = 0;
};


class FbxPractice
{
public:
	FbxPractice() = default;
	~FbxPractice();
	FbxPractice(const FbxPractice& _other) = delete;
	FbxPractice& operator=(const FbxPractice& _other) = delete;

public:
	void Init();
	void ImportFile(const char* _fileName);
	// 테스트 용으로 Scene안에 뭐가 들었는지 확인해보는 함수
	void TestTraverseScene() const;
	void TestTraverseLayer() const;
	void TestTraverseMesh() const;
	void TestTraverseSkin() const;
	void TestTraverseAnimation() const;

	void GetMeshToApp(const FbxMesh* _pMeshNode, std::vector<struct Vertex>& _vertices, std::vector<std::uint32_t>& _indices);
	void GetMaterialToApp(const FbxSurfaceMaterial* _pMaterial, FbxMaterial& _outMaterial);
	void GetAnimationToApp();

private:
	void PrintDeformerInfo(FbxMesh* _pMeshNode) const;
	FbxDouble3 GetMaterialProperty(
		const FbxSurfaceMaterial* _pMaterial,
		const char* _pPropertyName,
		const char* _pFactorPropertyName,
		unsigned int& _pTextureName);

	void PrintMeshInfo(FbxMesh* _pMeshNode) const;
	void PrintLayerInfo(FbxMesh* _pMeshNode, int _meshIndex) const;
	void PrintNode(FbxNode* _pNode) const;
	void PrintTabs() const;
	void PrintAttribute(FbxNodeAttribute* _pAttribute) const;
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType _type) const;
	FbxString GetLayerElementTypeName(FbxLayerElement::EType _type) const;

private:
	// File name
	std::string fileName = "";

	// FBX Mememory Manage Obj
	FbxManager* sdkManager = nullptr;
	// FBX IO Settings
	FbxIOSettings* ioSettings = nullptr;
	// FBX Importer
	FbxImporter* importer = nullptr;
	// Root FBX Scene
	FbxScene* rootScene = nullptr;

	mutable int numOfTabs = 0;

	struct SubMesh
	{
		SubMesh() : IndexOffset(0), TriangleCount(0) {}
		SubMesh(int _indexOffset, int _triangleCount) : IndexOffset(_indexOffset), TriangleCount(_triangleCount) {}

		int IndexOffset;
		int TriangleCount;
	};
	FbxArray<SubMesh*> m_SubMeshes;

public:
	FbxScene* GetRootScene() const{
		return rootScene;
	}
};

