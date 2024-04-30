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
	void TestTraverseScene();
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
};

