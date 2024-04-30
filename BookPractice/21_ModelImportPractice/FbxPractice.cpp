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
	// SDK �Ŵ��� �����
	sdkManager = FbxManager::Create();
	// IO Settings ������Ʈ �����
	ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ioSettings);
}

void FbxPractice::ImportFile(const char* _fileName)
{
	// Importer �����
	importer = FbxImporter::Create(sdkManager, "");

	if (importer == nullptr)
	{
		assert("FbxPractice : import doesn't initiated" && false);
		return;
	}
	
	fileName = _fileName;
	
	// ���� ��η� Importer�� �ʱ�ȭ �� ������
	if (!importer->Initialize(fileName.c_str(), -1, sdkManager->GetIOSettings()))
	{
		std::string errorStr = importer->GetStatus().GetErrorString();
		std::string Dialog = "FbxImporter::Initialize() Failed. \n Error returned : ";
		Dialog += errorStr;
		assert(Dialog.c_str() && false);

		return;
	}

	// �� ������ �� Scene�� �����
	rootScene = FbxScene::Create(sdkManager, "rootScene");
	// Import�� �Ǵ�.
	importer->Import(rootScene);
	// Importer�� �Ⱦ��� �����Ѵ�.
	importer->Destroy();
}

void FbxPractice::TestTraverseScene()
{
}


