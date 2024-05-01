// copyright : AutoDesk FBX

#pragma once

#include <fbxsdk.h>

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

private:
	void PrintMeshInfo(FbxMesh* _pMeshNode) const;
	void PrintLayerInfo(FbxMesh* _pMeshNode, int _meshIndex) const;
	void PrintNode(FbxNode* _pNode) const;
	void PrintTabs() const;
	void PrintAttribute(FbxNodeAttribute* _pAttribute) const;
	FbxString GetAttributeTypeName(FbxNodeAttribute::EType _type) const;

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
};

