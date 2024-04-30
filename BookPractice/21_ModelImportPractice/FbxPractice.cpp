#include "FbxPractice.h"
#include <cassert>

FbxPractice::~FbxPractice()
{
	if (importer != nullptr)
	{
		importer->Destroy();
	}
	if (ioSettings != nullptr)
	{
		ioSettings->Destroy();
	}
	if (sdkManager != nullptr)
	{
		sdkManager->Destroy();
	}
}

void FbxPractice::Init()
{
	// SDK 매니저 만들기
	sdkManager = FbxManager::Create();
	// IO Settings 오브젝트 만들기
	ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ioSettings);
}

void FbxPractice::ImportFile(const char* _fileName)
{
	// Importer 만들기
	importer = FbxImporter::Create(sdkManager, "");

	if (importer == nullptr)
	{
		assert("FbxPractice : import doesn't initiated" && false);
		return;
	}
	
	fileName = _fileName;
	
	// 파일 경로로 Importer를 초기화 한 다음에
	if (!importer->Initialize(fileName.c_str(), -1, sdkManager->GetIOSettings()))
	{
		std::string errorStr = importer->GetStatus().GetErrorString();
		std::string Dialog = "FbxImporter::Initialize() Failed. \n Error returned : ";
		Dialog += errorStr;
		assert(Dialog.c_str() && false);

		return;
	}

	// 모델 정보가 들어갈 Scene을 만들고
	rootScene = FbxScene::Create(sdkManager, "rootScene");
	// Import를 건다.
	importer->Import(rootScene);
	// Importer는 안쓰니 삭제한다.
	importer->Destroy();
}

void FbxPractice::TestTraverseScene()
{
}


