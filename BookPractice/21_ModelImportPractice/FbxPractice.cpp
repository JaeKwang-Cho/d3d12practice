/****************************************************************************************

Copyright (C) 2015 Autodesk, Inc.
All rights reserved.

Use of this software is subject to the terms of the Autodesk license agreement
provided at the time of installation or download, or which otherwise accompanies
this software in either electronic or hard copy form.

공부용으로 한글 주석 + 개조 중입니다.
****************************************************************************************/

#include "FbxPractice.h"
#include <cassert>
#include <Windows.h>
#include <sstream>
#include <format>
#include <fstream>

#include "FrameResource.h"
#include <stack>

FbxPractice::~FbxPractice()
{
	if (bones.size() > 0) {
		for (int i = 0; i < bones.size(); i++) {
			delete bones[i];
		}
	}
	if (ioSettings != nullptr)
	{
		ioSettings->Destroy();
	}
	if (rootScene != nullptr) 
	{
		rootScene->Destroy();
	}
	if (sdkManager != nullptr)
	{
		sdkManager->Destroy();
	}
}

// fbx manager를 초기화 하는것
void FbxPractice::Init()
{
	// SDK 매니저 만들기
	sdkManager = FbxManager::Create();
	// IO Settings 오브젝트 만들기
	ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ioSettings);
}

// fbx 파일을 import하는 것
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
	// DirectX 좌표계에 맞춘다.
	rootScene->GetGlobalSettings().SetAxisSystem(FbxAxisSystem::DirectX);
	//FbxGeometryConverter geometryConverter(sdkManager);
	//geometryConverter.Triangulate(rootScene, true);

	// Importer는 안쓰니 삭제한다.
	importer->Destroy();
}

// scene에 있는 node들이 뭐가 있는지 속성값을 print해보는 것이다.
void FbxPractice::TestTraverseScene() const
{
	FbxNode* pRootNode = rootScene->GetRootNode();

	OutputDebugStringA(std::format("***** count of ChildCount : '{}' *****\n", pRootNode->GetChildCount()).c_str());
	if (pRootNode != nullptr) {
		for (int i = 0; i < pRootNode->GetChildCount(); i++) {
			//OutputDebugStringA(std::format("^^^^^ child index : {}\n\n",i).c_str());
			PrintNode(pRootNode->GetChild(i));
		}
	}

}

void FbxPractice::TestTraverseLayer() const
{
	FbxNode* pRootNode = rootScene->GetRootNode();

	OutputDebugStringA(std::format("***** count of ChildCount : '{}' *****\n", pRootNode->GetChildCount()).c_str());
	if (pRootNode != nullptr) {
		for (int i = 0; i < pRootNode->GetChildCount(); i++) {
			FbxNodeAttribute* nodeAttrib = pRootNode->GetChild(i)->GetNodeAttribute();
			if (nodeAttrib->GetAttributeType() == FbxNodeAttribute::eMesh) {
				OutputDebugStringA("\nFbxNodeAttribute::eMesh\n");
				PrintLayerInfo(pRootNode->GetChild(i)->GetMesh(), i);
			}
		}
	}
}

void FbxPractice::TestTraverseMesh() const
{
	FbxNode* pRootNode = rootScene->GetRootNode();

	OutputDebugStringA(std::format("***** count of ChildCount : '{}' *****\n", pRootNode->GetChildCount()).c_str());
	if (pRootNode != nullptr) {
		for (int i = 0; i < pRootNode->GetChildCount(); i++) {
			FbxNodeAttribute* nodeAttrib = pRootNode->GetChild(i)->GetNodeAttribute();
			if (nodeAttrib->GetAttributeType() == FbxNodeAttribute::eMesh) {
				//OutputDebugStringA("\nFbxNodeAttribute::eMesh\n");
				PrintMeshInfo(pRootNode->GetChild(i)->GetMesh());
			}
		}
	}
}

void FbxPractice::TestTraverseSkin() const
{
	OutputDebugStringA("\n\n***Print Skin Info***\n");

	FbxScene* pRootScene = rootScene;
	FbxNode* pRootNode = pRootScene->GetRootNode();

	const char* clusterModes[] = { "Normalize", "Additive", "TotalOne" };

	for (int i = 0; i < pRootNode->GetChildCount(); i++) {
		FbxNodeAttribute* nodeAttrib = pRootNode->GetChild(i)->GetNodeAttribute();
		if (nodeAttrib->GetAttributeType() == FbxNodeAttribute::eMesh) {
			FbxMesh* pMesh = pRootNode->GetChild(i)->GetMesh();
			int skinCount = pMesh->GetDeformerCount(FbxDeformer::eSkin);
			for (int j = 0; j < skinCount; j++) {
				FbxSkin* pSkin = static_cast<FbxSkin*>(pMesh->GetDeformer(j, FbxDeformer::eSkin));
				int clusterCount = pSkin->GetClusterCount();
				OutputDebugStringA(std::format("\t{}th Mesh's {}th Skin Deformer has {} clusters\n", i, j+1, clusterCount).c_str());
				for (int k = 0; k < clusterCount; k++) {
					OutputDebugStringA(std::format("\t\t* {}th Mesh's {}th Skin Deformer's {}th cluster * \n", i, j+1, k).c_str());

					FbxCluster* pCluster = pSkin->GetCluster(k);
					int clusterModeIndex = static_cast<int>(pCluster->GetLinkMode());
					OutputDebugStringA(std::format("\t\t\tCluster's Link Mode : '{}'\n", clusterModes[clusterModeIndex]).c_str());
					FbxNode* pLink = pCluster->GetLink();
					OutputDebugStringA(std::format("\t\t\tCluster's Link Name : '{}'\n", pLink->GetName()).c_str());

					int controlPointIndicesCount = pCluster->GetControlPointIndicesCount();
					int* controlPointIndicesArr = pCluster->GetControlPointIndices();
					double* weightsArr = pCluster->GetControlPointWeights();
					OutputDebugStringA(std::format("\t\t\tCluster's control Point Indices & Weights Counts : '{}'\n", controlPointIndicesCount).c_str());

					FbxAMatrix matrix;
					matrix = pCluster->GetTransformMatrix(matrix);
					FbxVector4 translationVec = matrix.GetT();
					FbxVector4 rotationVec = matrix.GetR();
					FbxVector4 scalingVec = matrix.GetS();

					OutputDebugStringA(std::format("\t\t\tCluster's Translation Vector: ({}, {}, {}, {})\n", 
						static_cast<float>(translationVec[0]), 
						static_cast<float>(translationVec[1]), 
						static_cast<float>(translationVec[2]),
						static_cast<float>(translationVec[3])
					).c_str());

					OutputDebugStringA(std::format("\t\t\tCluster's Rotation Vector: ({}, {}, {}, {})\n",
						static_cast<float>(rotationVec[0]),
						static_cast<float>(rotationVec[1]),
						static_cast<float>(rotationVec[2]),
						static_cast<float>(rotationVec[3])
					).c_str());

					OutputDebugStringA(std::format("\t\t\tCluster's Scaling Vector: ({}, {}, {}, {})\n",
						static_cast<float>(scalingVec[0]),
						static_cast<float>(scalingVec[1]),
						static_cast<float>(scalingVec[2]),
						static_cast<float>(scalingVec[3])
					).c_str());

					matrix = pCluster->GetTransformLinkMatrix(matrix);
					FbxVector4 linkTranslationVec = matrix.GetT();
					FbxVector4 linkRotationVec = matrix.GetR();
					FbxVector4 linkScalingVec = matrix.GetS();

					OutputDebugStringA(std::format("\t\t\tCluster's Link Translation Vector: ({}, {}, {}, {})\n",
						static_cast<float>(linkTranslationVec[0]),
						static_cast<float>(linkTranslationVec[1]),
						static_cast<float>(linkTranslationVec[2]),
						static_cast<float>(linkTranslationVec[3])
					).c_str());

					OutputDebugStringA(std::format("\t\t\tCluster's Link Rotation Vector: ({}, {}, {}, {})\n",
						static_cast<float>(linkRotationVec[0]),
						static_cast<float>(linkRotationVec[1]),
						static_cast<float>(linkRotationVec[2]),
						static_cast<float>(linkRotationVec[3])
					).c_str());

					OutputDebugStringA(std::format("\t\t\tCluster's Link Scaling Vector: ({}, {}, {}, {})\n",
						static_cast<float>(linkScalingVec[0]),
						static_cast<float>(linkScalingVec[1]),
						static_cast<float>(linkScalingVec[2]),
						static_cast<float>(linkScalingVec[3])
					).c_str());

					if (pCluster->GetAssociateModel() != nullptr) {
						FbxNode* associateModel = pCluster->GetAssociateModel();
						OutputDebugStringA(std::format("\t\t\tCluster has associateModel : '{}'\n", associateModel->GetName()).c_str());
					}
				}
			}
		}
	}
}

void FbxPractice::TestTraverseAnimation() const
{
	OutputDebugStringA("\n\n***Print Animation Info***\n");
	FbxScene* pRootScene = rootScene;
	FbxNode* pRootNode = pRootScene->GetRootNode();

	int animstackCount = pRootScene->GetSrcObjectCount<FbxAnimStack>();

	for (int i = 0; i < animstackCount; i++) {
		FbxAnimStack* pAnimStack = pRootScene->GetSrcObject<FbxAnimStack>();

		const char* animStackName = pAnimStack->GetName();
		OutputDebugStringA(std::format("\t\t{}th AnimStack Name is '{}' \n", i + 1, animStackName).c_str());

		int animLayerCount = pAnimStack->GetMemberCount<FbxAnimLayer>();
		OutputDebugStringA(std::format("\t\t{}th AnimStack has '{}' AnimLayers \n", i + 1, animLayerCount).c_str());
		for (int j = 0; j < animLayerCount; j++) {
			FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>(j);
			OutputDebugStringA(std::format("\t\t{}th AnimStack's {}th AnimLayer's name : {}\n", i + 1, j + 1, pAnimLayer->GetName()).c_str());

			FbxAnimCurveNode* pAnimCurveNode_Translation = pRootNode->GetChild(0)->LclTranslation.GetCurveNode();
			OutputDebugStringA(std::format("\t\t(Test) Does RootNode(- Hip Skeleton) have any Animation Info (of Translation)? - {}\n", pAnimCurveNode_Translation->IsAnimated()).c_str());
			FbxTimeSpan timeInterval;
			bool bHasTimeInterval = pAnimCurveNode_Translation->GetAnimationInterval(timeInterval);
			if (bHasTimeInterval) {
				FbxTime animLength = timeInterval.GetDuration();
				OutputDebugStringA(std::format("\t\tAnimation Length : {}\n", static_cast<float>(animLength.GetSecondDouble())).c_str());
			}

			int channelsCount = pAnimCurveNode_Translation->GetChannelsCount();
			OutputDebugStringA(std::format("\t\t(Test) RootNode(- Hip Skeleton) curveNode has {} channels\n", channelsCount).c_str());
			std::vector<std::string> channelNames;
			FbxArray<FbxAnimCurve*> animCurves;
			int keyCount = 0;
			for (int k = 0; k < channelsCount; k++) {
				channelNames.push_back(pAnimCurveNode_Translation->GetChannelName(k).Buffer());
				OutputDebugStringA(std::format("\t\t\t {}th channel name is {}", k+1, channelNames[k]).c_str());
				animCurves.Add(pAnimCurveNode_Translation->GetCurve(k));
				keyCount = pAnimCurveNode_Translation->GetCurve(k)->KeyGetCount();
				OutputDebugStringA(std::format("\t\t\t and It's Key Counts is {}\n", keyCount).c_str());
			}
			if (!bHasTimeInterval) {
				continue;
			}
			for (int k = 0; k < animCurves.GetCount(); k++) {
				FbxAnimCurve* curAnimCurve = animCurves[k];
				int start = 0;
				int mid = keyCount / 2;
				int end = keyCount - 1;
				
				OutputDebugStringA(std::format("\t\t (Test) RootNode(- Hip Skeleton) curveNode '{}' channel value in start is '{}'\n", 
					channelNames[k], curAnimCurve->KeyGetValue(start)).c_str());
				OutputDebugStringA(std::format("\t\t (Test) RootNode(- Hip Skeleton) curveNode '{}' channel value in mid is '{}'\n", 
					channelNames[k], curAnimCurve->KeyGetValue(mid)).c_str());
				OutputDebugStringA(std::format("\t\t (Test) RootNode(- Hip Skeleton) curveNode '{}' channel value in end is '{}'\n", 
					channelNames[k], curAnimCurve->KeyGetValue(end)).c_str());
			}
		}
		OutputDebugStringA("\n");
	}
}

void FbxPractice::GetBonesToApp(const FbxNode* _node)
{
	BoneInfo* bone = new BoneInfo();
	bone->boneNode = _node;
	bone->name = _node->GetName();
	bone->parentIndex = -1;

	std::stack<BoneInfo*> boneStack;
	boneStack.push(bone);
	int curBoneIndex = 0;

	while (!boneStack.empty()) {
		BoneInfo* curBone = boneStack.top();
		boneStack.pop();

		const FbxNode* curNode = curBone->boneNode;
		int curChildCount = curNode->GetChildCount();
		for (int i = 0; i < curChildCount; i++) {
			BoneInfo* newBone = new BoneInfo();

			newBone->boneNode = curNode->GetChild(i);
			newBone->name = newBone->boneNode->GetName();
			newBone->parentIndex = curBoneIndex;
			boneStack.push(newBone);
		}

		bones.push_back(curBone);
		curBoneIndex = static_cast<int>(bones.size());
	}

	/*
	for (int i = 1; i < bones.size(); i++) {
		const char* nodeName = bones[i]->name.c_str();
		// Local Transform 값을 가져온다.
		FbxDouble3 translation = bones[i]->boneNode->LclTranslation.Get();
		FbxDouble3 rotation = bones[i]->boneNode->LclRotation.Get();
		FbxDouble3 scaling = bones[i]->boneNode->LclScaling.Get();

		// node 콘텐츠를 출력한다.
		OutputDebugStringA(std::format("{}th bone -> node name = '{}' / parent index and bone = '{}' - '{}'\n",
			i, nodeName, bones[i]->parentIndex, bones[bones[i]->parentIndex]->name).c_str());
	}
	*/
}

// bone에 맞는 animation을 로드하기
void FbxPractice::GetAnimationToApp()
{
	여기 할 챠례
	/*
	OutputDebugStringA("\n\n***Print Animation Info***\n");
	FbxScene* pRootScene = rootScene;
	FbxNode* pRootNode = pRootScene->GetRootNode();

	int animstackCount = pRootScene->GetSrcObjectCount<FbxAnimStack>();

	for (int i = 0; i < animstackCount; i++) {
		FbxAnimStack* pAnimStack = pRootScene->GetSrcObject<FbxAnimStack>();

		const char* animStackName = pAnimStack->GetName();
		OutputDebugStringA(std::format("\t\t{}th AnimStack Name is '{}' \n", i + 1, animStackName).c_str());

		int animLayerCount = pAnimStack->GetMemberCount<FbxAnimLayer>();
		OutputDebugStringA(std::format("\t\t{}th AnimStack has '{}' AnimLayers \n", i + 1, animLayerCount).c_str());
		for (int j = 0; j < animLayerCount; j++) {
			FbxAnimLayer* pAnimLayer = pAnimStack->GetMember<FbxAnimLayer>(j);
			OutputDebugStringA(std::format("\t\t{}th AnimStack's {}th AnimLayer's name : {}\n", i + 1, j + 1, pAnimLayer->GetName()).c_str());

			FbxAnimCurveNode* pAnimCurveNode_Translation = pRootNode->GetChild(0)->LclTranslation.GetCurveNode();
			OutputDebugStringA(std::format("\t\t(Test) Does RootNode(- Hip Skeleton) have any Animation Info (of Translation)? - {}\n", pAnimCurveNode_Translation->IsAnimated()).c_str());
			FbxTimeSpan timeInterval;
			bool bHasTimeInterval = pAnimCurveNode_Translation->GetAnimationInterval(timeInterval);
			if (bHasTimeInterval) {
				FbxTime animLength = timeInterval.GetDuration();
				OutputDebugStringA(std::format("\t\tAnimation Length : {}\n", static_cast<float>(animLength.GetSecondDouble())).c_str());
			}

			int channelsCount = pAnimCurveNode_Translation->GetChannelsCount();
			OutputDebugStringA(std::format("\t\t(Test) RootNode(- Hip Skeleton) curveNode has {} channels\n", channelsCount).c_str());
			std::vector<std::string> channelNames;
			FbxArray<FbxAnimCurve*> animCurves;
			int keyCount = 0;
			for (int k = 0; k < channelsCount; k++) {
				channelNames.push_back(pAnimCurveNode_Translation->GetChannelName(k).Buffer());
				OutputDebugStringA(std::format("\t\t\t {}th channel name is {}", k + 1, channelNames[k]).c_str());
				animCurves.Add(pAnimCurveNode_Translation->GetCurve(k));
				keyCount = pAnimCurveNode_Translation->GetCurve(k)->KeyGetCount();
				OutputDebugStringA(std::format("\t\t\t and It's Key Counts is {}\n", keyCount).c_str());
			}
			if (!bHasTimeInterval) {
				continue;
			}
			for (int k = 0; k < animCurves.GetCount(); k++) {
				FbxAnimCurve* curAnimCurve = animCurves[k];
				int start = 0;
				int mid = keyCount / 2;
				int end = keyCount - 1;

				OutputDebugStringA(std::format("\t\t (Test) RootNode(- Hip Skeleton) curveNode '{}' channel value in start is '{}'\n",
					channelNames[k], curAnimCurve->KeyGetValue(start)).c_str());
				OutputDebugStringA(std::format("\t\t (Test) RootNode(- Hip Skeleton) curveNode '{}' channel value in mid is '{}'\n",
					channelNames[k], curAnimCurve->KeyGetValue(mid)).c_str());
				OutputDebugStringA(std::format("\t\t (Test) RootNode(- Hip Skeleton) curveNode '{}' channel value in end is '{}'\n",
					channelNames[k], curAnimCurve->KeyGetValue(end)).c_str());
			}
		}
		OutputDebugStringA("\n");
	}
	*/
}

// 예제 따라가면서 해보기
void FbxPractice::GetMeshToApp(const FbxMesh* _pMeshNode, std::vector<Vertex>& _vertices, std::vector<std::uint32_t>& _indices)
{
	assert(_pMeshNode != nullptr);
	m_SubMeshes.Clear();

	// 일단... control point의 갯수를 얻는다.
	int polygonCount = _pMeshNode->GetPolygonCount();

	// material...마다 polygon 갯수를 계산한다?
	FbxLayerElementArrayTemplate<int>* materialIndiceArr = nullptr;
	FbxGeometryElement::EMappingMode materialMappingMode = FbxGeometryElement::eNone;
	if (_pMeshNode->GetElementMaterial() != nullptr) {
		materialIndiceArr = &_pMeshNode->GetElementMaterial()->GetIndexArray();
		//OutputDebugStringA(std::format("\n\n\nmaterial Count {}\n\n\n", materialIndiceArr->GetCount()).c_str() );
		materialMappingMode = _pMeshNode->GetElementMaterial()->GetMappingMode();
	}

	// Material MappingMode가 eByPolygon의 경우
	if (materialIndiceArr != nullptr && materialMappingMode == FbxGeometryElement::eByPolygon) 
	{
		assert(materialIndiceArr->GetCount() == polygonCount);
		if (materialIndiceArr->GetCount() == polygonCount) 
		{
			for (int polygonIndex = 0; polygonIndex < polygonCount; polygonIndex++)
			{
				// polygon을 인덱스로 하는 MaterialIndexArr에서 Material Index 값만큼
				const int materialIndex = materialIndiceArr->GetAt(polygonIndex);
				if (m_SubMeshes.GetCount() < materialIndex + 1)
				{
					m_SubMeshes.Resize(materialIndex + 1);
				}
				// SubMesh를 만들어준다.
				if (m_SubMeshes.GetAt(materialIndex) == nullptr)
				{
					m_SubMeshes.SetAt(materialIndex, new SubMesh(0, 1));
				}
			}

			// 아래 과정은 'hole'이 생기지 않게 보장해주는 작업이라고 한다...
			for (int i = 0; i < m_SubMeshes.GetCount(); i++) {
				if (m_SubMeshes.GetAt(i) == nullptr) {
					m_SubMeshes.SetAt(i, new SubMesh());
				}
			}

			// 그리고 Submesh 간의 offset을 기록해준다.
			const int materialCount = m_SubMeshes.GetCount();
			int offset = 0;
			for (int i = 0; i < materialCount; i++) {
				m_SubMeshes.GetAt(i)->IndexOffset = offset;
				offset += m_SubMeshes.GetAt(i)->TriangleCount * 3;
				m_SubMeshes.GetAt(i)->TriangleCount = 0; // 그리고 다시 삼각형 개수를 0으로 만든다.
			}
			assert(offset == polygonCount * 3);
		}
	}
	else // 그 이외의
	{
		// 모든 면은 같은 Material을 가지도록 한다.
		if (m_SubMeshes.GetCount() == 0) {
			m_SubMeshes.Resize(1);
			m_SubMeshes.SetAt(0, new SubMesh());
		}
	}

	// FbxMesh가 가진 정보를 Vertex로 사용할 수 있도록 모은다.
	// 만약, normal이나 uv가 polygon vertex에 붙어 있다면, vertex의 모든 attribute를 저장한다.
	bool bHasNormal = _pMeshNode->GetElementNormalCount() > 0;
	bool bHasUV = _pMeshNode->GetElementUVCount() > 0;
	bool bAllByControlPoint = false;
	// 여기서도 Normal과 UV의 MappingMode에 따라 작업이 다르다.
	FbxGeometryElement::EMappingMode normalMappingMode = FbxGeometryElement::eNone;
	FbxGeometryElement::EMappingMode uVMappingMode = FbxGeometryElement::eNone;

	// normal이 vertex에 붙어있을 때
	if (bHasNormal) {
		normalMappingMode = _pMeshNode->GetElementNormal(0)->GetMappingMode();
		// 쭉정이 일 때
		if (normalMappingMode == FbxGeometryElement::eNone) {
			bHasNormal = false;
		}
		// (eByControlPoint를 제외한) 나머지 모드 일 때
		else if (normalMappingMode != FbxGeometryElement::eByControlPoint) {
			bAllByControlPoint = false;
		}
	}
	// uv가 vertex에 붙어있을 때
	if (bHasUV) {
		uVMappingMode = _pMeshNode->GetElementUV(0)->GetMappingMode();
		// 쭉정이 일 때
		if (uVMappingMode == FbxGeometryElement::eNone) {
			bHasUV = false;
		}
		// (eByControlPoint를 제외한) 나머지 모드 일 때
		else if (uVMappingMode != FbxGeometryElement::eByControlPoint) {
			bAllByControlPoint = false;
		}
	}

	// control point나 vertex 정보를 올릴 메모리를 할당한다.
	int polygonVertexCount = _pMeshNode->GetControlPointsCount();

	// 컨트롤 포인트로 매핑 좌표가 생성되지 않는 경우
	if (!bAllByControlPoint) {
		// 폴리곤 * 3 으로 점 개수를 지정한다.
		polygonVertexCount = polygonCount * 3;
	} 
	// index는 unsigned int
	std::vector<UINT> indices(polygonCount * 3);
	indices.resize(polygonCount * 3);
	_indices.resize(polygonCount * 3);
	//std::vector<XMFLOAT2> uvs;
	FbxStringList uvNameList;
	_pMeshNode->GetUVSetNames(uvNameList);
	const char* uvName = nullptr;
	if (bHasUV && uvNameList.GetCount() > 0) {
		//uvs.resize(polygonCount);
		uvName = uvNameList[0];
	}

	// 1_컨트롤 포인트로 매핑 좌표가 생성되는 경우는
	// 아래와 같이 속성 값을 Array에 넣어준다.
	const FbxVector4* controlPointArr = _pMeshNode->GetControlPoints();
	FbxVector4 curVertex;
	FbxVector4 curNormal;
	FbxVector2 curUV;
	if (bAllByControlPoint) {
		const FbxLayerElementTemplate<FbxVector4>* normalElement = nullptr;
		const FbxLayerElementTemplate<FbxVector2>* uvElement = nullptr;
		if (bHasNormal) {
			normalElement = _pMeshNode->GetElementNormal(0);
		}
		if (bHasUV) {
			uvElement = _pMeshNode->GetElementUV(0);
		}
		for (int i = 0; i < polygonVertexCount; i++) {
			// vertex
			curVertex = controlPointArr[i];
			// normal
			if (bHasNormal) {
				int normalIndex = i;
				// 레퍼런스 모드에 따라 인덱스를 얻는 방법이 달라진다.
				if (normalElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
					normalIndex = normalElement->GetIndexArray().GetAt(i);
				}
				curNormal = normalElement->GetDirectArray().GetAt(normalIndex);
			}
			// UV
			if (bHasUV) {
				int uvIndex = i;
				if (uvElement->GetReferenceMode() == FbxLayerElement::eIndexToDirect) {
					uvIndex = uvElement->GetIndexArray().GetAt(i);
				}
				curUV = uvElement->GetDirectArray().GetAt(uvIndex);
			}

			// App에 값 넣기
			XMFLOAT3 pos = XMFLOAT3(static_cast<float>(curVertex[0]), static_cast<float>(curVertex[1]), static_cast<float>(curVertex[2]));
			XMFLOAT3 normal = XMFLOAT3(static_cast<float>(curNormal[0]), static_cast<float>(curNormal[1]), static_cast<float>(curNormal[2]));
			XMFLOAT2 uv = XMFLOAT2(static_cast<float>(curUV[0]), static_cast<float>(curUV[1]));
			XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			XMFLOAT3 tanU;
			XMVECTOR vNorm = XMLoadFloat3(&normal);
			if (fabsf(XMVectorGetX(XMVector3Dot(vNorm, up))) < 1.0f - 0.001f)
			{
				XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(up, vNorm));
				XMStoreFloat3(&tanU, vTanU);
			}
			else
			{
				up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
				XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(vNorm, up));
				XMStoreFloat3(&tanU, vTanU);
			}

			Vertex vertex(pos, normal, uv, tanU);
			_vertices.push_back(vertex);
		}
	}

	int vertexCount = 0;
	for (int pI = 0; pI < polygonCount; pI++) {
		// 머테리얼 작업을 해준다... 뭔지 모르겠지만
		int materialIndex = 0;
		// eByPolygon의 경우 material index를 얻는 방법이 달라진다.
		if (materialIndiceArr != nullptr && materialMappingMode == FbxGeometryElement::eByPolygon) {
			materialIndex = materialIndiceArr->GetAt(pI);
		}
		// 위에서 eByPolygon의 경우는 SubMesh를 엄청 많이 만들고, Material IndexArray에 맞는 offset을 넣어주었다.
		// (그렇지 않은 경우는 그냥 1씩 늘려주면서 index를 넣고)
		// 무튼 아래와 같은 방법으로 indexOffset을 얻는다.
		const int indexOffset = m_SubMeshes.GetAt(materialIndex)->IndexOffset + m_SubMeshes.GetAt(materialIndex)->TriangleCount * 3;
		int a = 0;
		for (int vI = 0; vI < 3; vI++) {
			// 모든 폴리곤을 돌아다니면서 controlPoint의 Index 역할을 하는 vertex를 얻는다.
			const int controlPointIndex = _pMeshNode->GetPolygonVertex(pI, vI);

			// control point 값이 -1이 나오면 안된다.
			if (controlPointIndex >= 0) {
				// 컨트롤 포인트로 매핑 좌표가 생성되는 경우
				if (bAllByControlPoint) {
					// 위에서 이미 속성 값을 넣어주었기 때문에 index 값만 넣어준다.
					indices[indexOffset + vI] = static_cast<UINT>(controlPointIndex);
					_indices[indexOffset + vI] = static_cast<UINT>(controlPointIndex);
				}
				// 2_폴리곤 + 버텍스로 매핑 좌표가 생성되는 경우
				// 이제 속성값을 배열에 넣어준다.
				else
				{
					// (eByControlPoint의 경우는 ElementIndexArray에서 index를 얻어서 값을 가져왔는데)
					// 이 경우를 보면 
					indices[indexOffset + vI] = static_cast<UINT>(vertexCount);
					_indices[indexOffset + vI] = static_cast<UINT>(vertexCount);
					// control point가 vertex가 되고
					curVertex = controlPointArr[controlPointIndex];
					// polygon에서 직접 normal과 UV을 가져온다.
					if (bHasNormal) {
						_pMeshNode->GetPolygonVertexNormal(pI, vI, curNormal);
					}
					if (bHasUV) {
						bool bUnmappedUV;
						_pMeshNode->GetPolygonVertexUV(pI, vI, uvName, curUV, bUnmappedUV);
					}

					XMFLOAT3 pos = XMFLOAT3(static_cast<float>(curVertex[0]), static_cast<float>(curVertex[1]), static_cast<float>(curVertex[2]));
					XMFLOAT3 normal = XMFLOAT3(static_cast<float>(curNormal[0]), static_cast<float>(curNormal[1]), static_cast<float>(curNormal[2]));
					XMFLOAT2 uv = XMFLOAT2(static_cast<float>(curUV[0]), static_cast<float>(curUV[1]));
					XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
					XMFLOAT3 tanU;
					XMVECTOR vNorm = XMLoadFloat3(&normal);
					if (fabsf(XMVectorGetX(XMVector3Dot(vNorm, up))) < 1.0f - 0.001f)
					{
						XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(up, vNorm));
						XMStoreFloat3(&tanU, vTanU);
					}
					else
					{
						up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
						XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(vNorm, up));
						XMStoreFloat3(&tanU, vTanU);
					}

					Vertex vertex(pos, normal, uv, tanU);
					_vertices.push_back(vertex);
				}
			}
			vertexCount++;
		}
		m_SubMeshes.GetAt(materialIndex)->TriangleCount += 1;
	}
}

void FbxPractice::GetMaterialToApp(const FbxSurfaceMaterial* _pMaterial, FbxMaterial& _outMaterial)
{
	ColorChannel emissiveColor = ColorChannel();
	ColorChannel ambientColor = ColorChannel();
	ColorChannel diffuseColor = ColorChannel();
	ColorChannel specularColor = ColorChannel();
	float shininess =0;
	 
	const FbxDouble3 emissive = GetMaterialProperty(_pMaterial, FbxSurfaceMaterial::sEmissive, FbxSurfaceMaterial::sEmissiveFactor, _outMaterial.emissiveColor.textureName);
	const FbxDouble3 ambient = GetMaterialProperty(_pMaterial, FbxSurfaceMaterial::sAmbient, FbxSurfaceMaterial::sAmbientFactor, _outMaterial.ambientColor.textureName);
	const FbxDouble3 diffuse = GetMaterialProperty(_pMaterial, FbxSurfaceMaterial::sDiffuse, FbxSurfaceMaterial::sDiffuseFactor, _outMaterial.diffuseColor.textureName);
	const FbxDouble3 specular = GetMaterialProperty(_pMaterial, FbxSurfaceMaterial::sSpecular, FbxSurfaceMaterial::sSpecularFactor, _outMaterial.specularColor.textureName);

	FbxProperty shininessProperty = _pMaterial->FindProperty(FbxSurfaceMaterial::sShininess);
	if (shininessProperty.IsValid()) {
		_outMaterial.shininess = static_cast<float>(shininessProperty.Get<FbxDouble>());
	}

	_outMaterial.emissiveColor.color = XMFLOAT4(
		static_cast<float>(emissive[0]),
		static_cast<float>(emissive[1]),
		static_cast<float>(emissive[2]),
		1.f
	);
	_outMaterial.ambientColor.color = XMFLOAT4(
		static_cast<float>(ambient[0]),
		static_cast<float>(ambient[1]),
		static_cast<float>(ambient[2]),
		1.f
	);
	_outMaterial.diffuseColor.color = XMFLOAT4(
		static_cast<float>(diffuse[0]),
		static_cast<float>(diffuse[1]),
		static_cast<float>(diffuse[2]),
		1.f
	);
	_outMaterial.specularColor.color = XMFLOAT4(
		static_cast<float>(specular[0]),
		static_cast<float>(specular[1]),
		static_cast<float>(specular[2]),
		1.f
	);
}

void FbxPractice::PrintDeformerInfo(FbxMesh* _pMeshNode) const
{
	OutputDebugStringA("\n\t*Print Deformer Info*\n");
	OutputDebugStringA(std::format("\t\tCurrent Mesh have {} deformers \n", _pMeshNode->GetDeformerCount()).c_str());
	const FbxDeformer* deformer = _pMeshNode->GetDeformer(0);

	FbxDeformer::EDeformerType deformerType = deformer->GetDeformerType();
	switch (deformerType) {
	case FbxDeformer::eUnknown: { OutputDebugStringA("\t\tFbxDeformer::eUnknown\n"); break; }
	case FbxDeformer::eSkin: { OutputDebugStringA("\t\tFbxDeformer::eSkin\n"); break; }
	case FbxDeformer::eBlendShape: { OutputDebugStringA("\t\tFbxDeformer::eBlendShape\n"); break; }
	case FbxDeformer::eVertexCache: { OutputDebugStringA("\t\tFbxDeformer::eVertexCache\n"); break; }
	}
}

FbxDouble3 FbxPractice::GetMaterialProperty(
	const FbxSurfaceMaterial* _pMaterial,
	const char* _pPropertyName,
	const char* _pFactorPropertyName,
	unsigned int& _pTextureName)
{
	FbxDouble3 result(0, 0, 0);
	const FbxProperty property = _pMaterial->FindProperty(_pPropertyName);
	const FbxProperty factorProperty = _pMaterial->FindProperty(_pFactorPropertyName);
	if (property.IsValid() && factorProperty.IsValid())
	{
		result = property.Get<FbxDouble3>();
		double lFactor = factorProperty.Get<FbxDouble>();
		if (lFactor != 1)
		{
			result[0] *= lFactor;
			result[1] *= lFactor;
			result[2] *= lFactor;
		}
	}

	if (property.IsValid())
	{
		const int textureCount = property.GetSrcObjectCount<FbxFileTexture>();
		if (textureCount)
		{
			const FbxFileTexture* texture = property.GetSrcObject<FbxFileTexture>();
			if (texture && texture->GetUserDataPtr())
			{
				_pTextureName = *(static_cast<UINT*>(texture->GetUserDataPtr()));
			}
		}
	}

	return result;
}

void FbxPractice::PrintMeshInfo(FbxMesh* _pMeshNode) const
{
	OutputDebugStringA("\n***Print Mesh Info***\n\n");
	// FbxNodeAttribute - Node Attribute Type
	FbxNodeAttribute::EType attribType = _pMeshNode->GetAttributeType();
	FbxString attribTypeStr = GetAttributeTypeName(attribType);
	OutputDebugStringA(std::format("FbxNodeAttribute - Mesh's attribute type is {}\n", attribTypeStr.Buffer()).c_str());

	OutputDebugStringA("\n--FbxLayerContainer--\n");
	// FbxLayerContainer - Layers
	int layerCount = _pMeshNode->GetLayerCount();
	if (layerCount > 0) {
		OutputDebugStringA(std::format("\tFbxLayerContainer - Mesh has layer {}\n", layerCount).c_str());
		for (int i = 0; i < layerCount; i++) {
			FbxLayer* curLayer = _pMeshNode->GetLayer(i);
			for (int j = 0; j < (int)FbxLayerElement::eTypeCount; j++) {
				FbxLayerElement* curLayerElement = curLayer->GetLayerElementOfType((FbxLayerElement::EType)j);
				if (curLayerElement != nullptr) {
					FbxString curLayerEleTypeStr = GetLayerElementTypeName((FbxLayerElement::EType)j);
					OutputDebugStringA(std::format("\t\t {}th's layer has {} Type's layerElement.\n", i, curLayerEleTypeStr.Buffer()).c_str());
					const char* curlayerEleName = curLayerElement->GetName();
					if (curlayerEleName != nullptr) {
						OutputDebugStringA(std::format("\t\t\t It's Name is '{}'\n", curlayerEleName).c_str());
					}
				}
			}
		}
	}
	else {
		OutputDebugStringA("\tFbxLayerContainer - Mesh has no layer \n");
	}
	
	OutputDebugStringA("\n--FbxGeometryBase--\n");
	// FbxGeometryBase - Control Point
	int controlPointCount = _pMeshNode->GetControlPointsCount();
	if (controlPointCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} control points\n", controlPointCount).c_str());
		const FbxVector4 firstControlPoint = _pMeshNode->GetControlPointAt(0); // 너무 많을 것 같으니 하나만...
		OutputDebugStringA(std::format("\t\tfirst control point is ({}, {}, {}, {})\n", firstControlPoint[0], firstControlPoint[1], firstControlPoint[2], firstControlPoint[3]).c_str());
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no control points");
	}

	// FbxGeometryBase - NormalElement
	int normalElementCount = _pMeshNode->GetElementNormalCount();
	if (normalElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} normalElements\n", normalElementCount).c_str());
		const FbxLayerElementTemplate<FbxVector4>* firstNormalElement = _pMeshNode->GetElementNormal(0);
		const FbxLayerElementArrayTemplate<FbxVector4>* firstNormalElementsArr = &firstNormalElement->GetDirectArray();
		int countNormals = firstNormalElementsArr->GetCount();
		OutputDebugStringA(std::format("\t\tFbxLayerElementTemplate - first normalElement's has {} \n", countNormals).c_str());
		FbxVector4 firstNormal = firstNormalElementsArr->GetAt(0);
		OutputDebugStringA(std::format("\t\tfirst normal is ({}, {}, {}, {})\n", firstNormal[0], firstNormal[1], firstNormal[2], firstNormal[3]).c_str());
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no normalElements\n");
	}
	
	// FbxGeometryBase - Tangent
	int tangentElementCount = _pMeshNode->GetElementTangentCount();
	if (tangentElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} tangents\n", tangentElementCount).c_str());
		const FbxLayerElementTemplate<FbxVector4>* firstTangentElement = _pMeshNode->GetElementTangent(0);
		const FbxLayerElementArrayTemplate<FbxVector4>* firstTangentElementsArr = &firstTangentElement->GetDirectArray();
		int countTangents = firstTangentElementsArr->GetCount();
		OutputDebugStringA(std::format("\t\tFbxLayerElementTemplate - first tangentElement's has {} \n", countTangents).c_str());
		FbxVector4 firstTangent = firstTangentElementsArr->GetAt(0);
		OutputDebugStringA(std::format("\t\tfirst tangent is ({}, {}, {}, {})\n", firstTangent[0], firstTangent[1], firstTangent[2], firstTangent[3]).c_str());
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no tangentElements\n");
	}

	// FbxGeometryBase - Binormal
	int binormalElementCount = _pMeshNode->GetElementBinormalCount();
	if (binormalElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} binormals\n", binormalElementCount).c_str());
		const FbxLayerElementTemplate<FbxVector4>* firstBinormalElement = _pMeshNode->GetElementBinormal(0);
		const FbxLayerElementArrayTemplate<FbxVector4>* firstBinormalElementsArr = &firstBinormalElement->GetDirectArray();
		int countBinormals = firstBinormalElementsArr->GetCount();
		OutputDebugStringA(std::format("\t\tFbxLayerElementTemplate - first binormalElement's has {} \n", countBinormals).c_str());
		FbxVector4 firstBinormal = firstBinormalElementsArr->GetAt(0);
		OutputDebugStringA(std::format("\t\tfirst binormal is ({}, {}, {}, {})\n", firstBinormal[0], firstBinormal[1], firstBinormal[2], firstBinormal[3]).c_str());
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no binormalElements\n");
	}

	// FbxGeometryBase - Material
	int materialElementCount = _pMeshNode->GetElementMaterialCount();
	if (materialElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} materialElement\n", materialElementCount).c_str());
		const FbxLayerElementMaterial* firstMaterialElement = _pMeshNode->GetElementMaterial(0);
		OutputDebugStringA("\t\tfirst material is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no MaterialElements\n");
	}

	// FbxGeometryBase - Polygon Group
	int polygonGroupElementCount = _pMeshNode->GetElementPolygonGroupCount();
	if (polygonGroupElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} polygonGroupElements\n", polygonGroupElementCount).c_str());
		const FbxLayerElementTemplate<int>* firstPolygonGroupElement = _pMeshNode->GetElementPolygonGroup(0);
		OutputDebugStringA("\t\tfirst polygonGroup is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no polygonGroupElements\n");
	}

	// FbxGeometryBase - Vertex Color
	int vertexColorElementCount = _pMeshNode->GetElementVertexColorCount();
	if (vertexColorElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} vertexColorElements\n", vertexColorElementCount).c_str());
		const FbxLayerElementTemplate<FbxColor>* firstPolygonGroupElement = _pMeshNode->GetElementVertexColor(0);
		OutputDebugStringA("\t\tfirst vertexColor is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no vertexColorElement\n");
	}

	// FbxGeometryBase - Smoothing
	int smoothingElementCount = _pMeshNode->GetElementSmoothingCount();
	if (smoothingElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} smoothingElements\n", smoothingElementCount).c_str());
		const FbxLayerElementTemplate<int>* firstSmoothingElement = _pMeshNode->GetElementSmoothing(0);
		OutputDebugStringA("\t\tfirst smoothing is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no smoothingElements\n");
	}

	// FbxGeometryBase - Vertex Crease
	int vertexCreaseElementCount = _pMeshNode->GetElementVertexCreaseCount();
	if (vertexCreaseElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} vertexCreaseElements\n", vertexCreaseElementCount).c_str());
		const FbxLayerElementTemplate<double>* firstVertexCreaseElement = _pMeshNode->GetElementVertexCrease(0);
		OutputDebugStringA("\t\tfirst vertex crease is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no vertexCreaseElement\n");
	}

	// FbxGeometryBase - Edge Crease
	int edgeCreaseElementCount = _pMeshNode->GetElementEdgeCreaseCount();
	if (edgeCreaseElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} edgeCreaseElements\n", edgeCreaseElementCount).c_str());
		const FbxLayerElementTemplate<double>* firstedgeCreaseElement = _pMeshNode->GetElementEdgeCrease(0);
		OutputDebugStringA("\t\tfirst edge crease is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no edgeCreaseElement\n");
	}

	// FbxGeometryBase - Hole
	int holeElementCount = _pMeshNode->GetElementHoleCount();
	if (holeElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} holeElements\n", holeElementCount).c_str());
		const FbxLayerElementTemplate<bool>* firstHoleElement = _pMeshNode->GetElementHole(0);
		OutputDebugStringA("\t\tfirst edge crease is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no HoleElement\n");
	}

	// FbxGeometryBase - UserData
	int userDataElementCount = _pMeshNode->GetElementUserDataCount();
	if (userDataElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} UserDataElements\n", userDataElementCount).c_str());
		const FbxLayerElementTemplate<void*>* firstUserDataElement = _pMeshNode->GetElementUserData(0);
		OutputDebugStringA("\t\tfirst edge crease is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no UserDataElement\n");
	}

	// FbxGeometryBase - Visibility
	int visibilityElementCount = _pMeshNode->GetElementVisibilityCount();
	if (visibilityElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} VisibilityElements\n", visibilityElementCount).c_str());
		const FbxLayerElementTemplate<bool>* firstVisibilityElement = _pMeshNode->GetElementVisibility(0);
		OutputDebugStringA("\t\tfirst edge crease is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no VisibilityElement\n");
	}

	// FbxGeometryBase - UV
	int uvElementCount = _pMeshNode->GetElementUVCount();
	if (uvElementCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} UVElements\n", uvElementCount).c_str());
		const FbxLayerElementTemplate<FbxVector2>* firstUVElement = _pMeshNode->GetElementUV(0);
		OutputDebugStringA("\t\tfirst UVElements is ... i don't know\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometryBase - Mesh has no UVElement\n");
	}

	// FbxGeometryBase - UVSetName
	FbxStringList uvSetNameList;
	_pMeshNode->GetUVSetNames(uvSetNameList);
	if (uvSetNameList.GetCount() > 0) {
		OutputDebugStringA(std::format("\tFbxGeometryBase - Mesh has {} UV Sets\n", uvSetNameList.GetCount()).c_str());
		for (int i = 0; i < uvSetNameList.GetCount(); i++) {
			OutputDebugStringA(std::format("\t\t{}th UV set name is {}\n", i, uvSetNameList[0].Buffer()).c_str());
		}
	}

	OutputDebugStringA("\n--FbxGeometry--\n");
	// FbxGeometry - Deformer
	int deformerCount = _pMeshNode->GetDeformerCount();
	FbxArray<FbxDeformer*> deformers;
	if (deformerCount > 0) {
		OutputDebugStringA(std::format("\tFbxGeometry - Mesh has {} deformer\n", deformerCount).c_str());
		for (int i = 0; i < deformerCount; i++) {
			deformers.Add(_pMeshNode->GetDeformer(i));
		}
	}
	else {
		OutputDebugStringA("\tFbxGeometry - Mesh has no deformers\n");
	}

	// FbxGeometry - weight map
	int weightMapCount = _pMeshNode->GetDestinationGeometryWeightedMapCount();
	if (weightMapCount > 0) {
		OutputDebugStringA("\tFbxGeometry - Mesh has weightMap\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometry - Mesh has no weightMap\n");
	}
	// FbxGeometry - Shape Management
	int shapeCount = _pMeshNode->GetShapeCount();
	if (shapeCount > 0) {
		OutputDebugStringA("\tFbxGeometry - Mesh has shape\n");
	}
	else {
		OutputDebugStringA("\tFbxGeometry - Mesh has no shape\n");
	}
	
	OutputDebugStringA("\n--FbxMesh--\n");
	// FbxMesh - Polygon
	int polygonCount = _pMeshNode->GetPolygonCount();
	OutputDebugStringA(std::format("\tFbxMesh - Mesh has '{}' polygons\n", polygonCount).c_str());

	int vertexCount = 0;
	for (int i = 0; i < polygonCount; i++) {
		vertexCount += _pMeshNode->GetPolygonSize(i);
	}
	OutputDebugStringA(std::format("\tFbxMesh - Mesh has '{}' vertices\n", vertexCount).c_str());

	int firstVertex = _pMeshNode->GetPolygonVertex(0, 0);
	OutputDebugStringA(std::format("\tFbxMesh - first vertex(which is control point's index) in first polygon is ({})\n", firstVertex).c_str());

	FbxVector4 firstVertexNormal = FbxVector4();
	bool bHasNormal = _pMeshNode->GetPolygonVertexNormal(1, 1, firstVertexNormal);
	if (bHasNormal) {
		OutputDebugStringA(std::format("\tFbxMesh - first vertex, first polygon's Normal is ({}, {}, {})\n", firstVertexNormal[0], firstVertexNormal[1], firstVertexNormal[2]).c_str());
	}	

	FbxVector2 firstVertexUV = FbxVector2();
	bool bfirstVertexUVMapped;
	if (uvSetNameList.GetCount() > 0) {
		bool bHasUV = _pMeshNode->GetPolygonVertexUV(0, 0, uvSetNameList[0], firstVertexUV, bfirstVertexUVMapped);
		if (bHasUV) {
			OutputDebugStringA(std::format("\tFbxMesh - first vertex, first polygon, first uv set' UV is ({}, {}) / mapped check '{}' \n", firstVertexUV[0], firstVertexUV[1], bfirstVertexUVMapped).c_str());
		}
	}

	// FbxMesh - UV
	OutputDebugStringA("\tFbxMesh - TextureUV\n");
	for (int i = 0; i < FbxLayerElement::eTypeCount; i++) {
		FbxLayerElementArrayTemplate<FbxVector2>* uvArrLockable = nullptr;
		FbxLayerElement::EType curType = static_cast<FbxLayerElement::EType>(i);
		bool bSuccessGetTextureUV = _pMeshNode->GetTextureUV(&uvArrLockable, curType);
		if (bSuccessGetTextureUV && uvArrLockable->GetCount() > 0) {
			OutputDebugStringA(std::format("\t\tFbxMesh - Mesh has {} UV textures- \n", GetLayerElementTypeName(curType).Buffer()).c_str());
			uvArrLockable->Clear();
		}
		else {
			OutputDebugStringA(std::format("\t\tFbxMesh - Mesh has 'no' UV textures - Types {}\n", GetLayerElementTypeName(curType).Buffer()).c_str());
		}
	}
}

void FbxPractice::PrintLayerInfo(FbxMesh* _pMeshNode, int _meshIndex) const
{
	std::ofstream fs;
	fs.open(std::format("PrintLayerInfo_{}.txt", _meshIndex).c_str());
	assert(_pMeshNode != nullptr);

	int layerCount = _pMeshNode->GetLayerCount();
	fs << "******** layer Count : " << layerCount << "********\n";
	for (int i = 0; i < layerCount; i++) {
		FbxLayer* layer = _pMeshNode->GetLayer(i);

		const FbxLayerElementTemplate<int>* polygons = layer->GetPolygonGroups();
		if(polygons != nullptr){
			OutputDebugStringA("layer has polygons\n");
		}
		const FbxLayerElementTemplate<FbxVector4>* normal = layer->GetNormals();
		if (normal != nullptr) {
			OutputDebugStringA("layer has normal\n");
			const FbxLayerElementArrayTemplate<FbxVector4> normalArr = normal->GetDirectArray();
			fs << "\n normalArr - normalArrCounts : " << normalArr.GetCount() << "\n\t";
			for (int j = 0; j < normalArr.GetCount(); j++)
			{
				fs << "(" << normalArr[j][0] << ", " << normalArr[j][1] << ", " << normalArr[j][2] << ") ";
			}
			fs << "\n\n";
			
		}
		const FbxLayerElementTemplate<FbxVector4>* tangent = layer->GetTangents();
		if (tangent != nullptr) {
			OutputDebugStringA("layer has tangent\n");
		}
		const FbxLayerElementTemplate<FbxVector4>* binormal = layer->GetBinormals();
		if (binormal != nullptr) {
			OutputDebugStringA("layer has binormal\n");
		}
		FbxArray<const FbxLayerElementUV*> uvsSet = layer->GetUVSets();
		if (uvsSet.GetCount() > 0) {
			OutputDebugStringA(std::format("layer has {} uvs\n", uvsSet.GetCount()).c_str());
			for (int j = 0; j < uvsSet.GetCount(); j++) {
				const FbxLayerElementArrayTemplate<FbxVector2> uvArr = uvsSet[j]->GetDirectArray();
				fs << "\n uvArr[" << j << "] - uvArrCounts :" << uvArr.GetCount() << "\n\t";
				for (int k = 0; k < uvArr.GetCount(); k++) {
					fs << "(" << uvArr[k][0] << ", " << uvArr[k][1] << ") ";
				}
				fs << "\n";
			}
			fs << "\n\n";
		}
		const FbxLayerElementTemplate<FbxSurfaceMaterial*>* materials = layer->GetMaterials();
		if (materials != nullptr) {
			OutputDebugStringA("layer has materialElement\n");
			const FbxLayerElementArrayTemplate<FbxSurfaceMaterial*> matArr = materials->GetDirectArray();
			fs << "\n matArr - matArrCounts : " << matArr.GetCount() << "\n";
			for (int j = 0; j < matArr.GetCount(); j++) {
				fs << "\tShadingModel : '" << matArr[j]->sShadingModel << "' \n";
				fs << "\tMultiLayer: '" << matArr[j]->sMultiLayer << "' \n";
				fs << "\tEmissive : '" << matArr[j]->sEmissive << "' \n";
				fs << "\tEmissiveFactor: '" << matArr[j]->sEmissiveFactor << "' \n";
				fs << "\tAmbient : '" << matArr[j]->sAmbient << "' \n";
				fs << "\tAmbientFactor : '" << matArr[j]->sAmbientFactor << "' \n";
				fs << "\tDiffuse : '" << matArr[j]->sDiffuse << "' \n";
				fs << "\tDiffuseFactor : '" << matArr[j]->sDiffuseFactor << "' \n";
				fs << "\tSpecular : '" << matArr[j]->sSpecular << "' \n";
				fs << "\tSpecularFactor : '" << matArr[j]->sSpecularFactor << "' \n";
				fs << "\tShininess : '" << matArr[j]->sShininess << "' \n";
				fs << "\tBump : '" << matArr[j]->sBump << "' \n";
				fs << "\tNormalMap : '" << matArr[j]->sNormalMap << "' \n";
				fs << "\tBumpFactor : '" << matArr[j]->sBumpFactor << "' \n";
				fs << "\tTransparentColor : '" << matArr[j]->sTransparentColor << "' \n";
				fs << "\tTransparencyFactor : '" << matArr[j]->sTransparencyFactor << "' \n";
				fs << "\tReflection : '" << matArr[j]->sReflection << "' \n";
				fs << "\tReflectionFactor : '" << matArr[j]->sReflectionFactor << "' \n";
				fs << "\tDisplacementColor : '" << matArr[j]->sDisplacementColor << "' \n";
				fs << "\tDisplacementFactor : '" << matArr[j]->sDisplacementFactor << "' \n";
				fs << "\tVectorDisplacementColor : '" << matArr[j]->sVectorDisplacementColor << "' \n";
				fs << "\tVectorDisplacementFactor : '" << matArr[j]->sVectorDisplacementFactor << "' \n";
			}
		}
		//const FbxSurfaceMaterial* material = pRootNode->GetMaterial(materialElement->GetIndexArray()[0]);
		const FbxLayerElementTemplate<FbxColor>* vertexColors = layer->GetVertexColors();
		if (vertexColors != nullptr) {
			OutputDebugStringA("layer has vertexColors\n");
		}
		// smoothing, crease, hole, user data는 뭔지 모르니 넘어간다.
		// user data도 뭔지 모르니 넘어간다.
	}
	fs.close();
}

void FbxPractice::PrintNode(FbxNode* _pNode) const
{
	std::ostringstream oss;
	PrintTabs();
	const char* nodeName = _pNode->GetName();
	// Local Transform 값을 가져온다.
	FbxDouble3 translation = _pNode->LclTranslation.Get();
	FbxDouble3 rotation = _pNode->LclRotation.Get();
	FbxDouble3 scaling = _pNode->LclScaling.Get();

	// node 콘텐츠를 출력한다.
	oss << std::format("<node name = '{}'", nodeName);
	//oss << std::format("translation ='{}, {}, {})' rotation ='({}, {}, {})' scaling='({}, {}, {})'>\n",
		//translation[0], translation[1], translation[2],
		//rotation[0], rotation[1], rotation[2],
		//scaling[0], scaling[1], scaling[2]);
	OutputDebugStringA(oss.str().c_str());
	oss.clear();
	/*
	printf("<node name = '%s' translation ='(%f, %f, %f)' rotation ='(%f, %f, %f)' scaling='(%f, %f, %f)'>\n",
		nodeName,
		translation[0], translation[1], translation[2],
		rotation[0], rotation[1], rotation[2],
		scaling[0], scaling[1], scaling[2]
	);
	*/
	numOfTabs++;

	// node의 속성값을 출력한다.
	for (int i = 0; i < _pNode->GetNodeAttributeCount(); i++) {
		PrintAttribute(_pNode->GetNodeAttributeByIndex(i));
	}

	// 재귀적으로 자식들도 호출한다.
	for (int j = 0; j < _pNode->GetChildCount(); j++) {
		PrintNode(_pNode->GetChild(j));
	}

	numOfTabs--;
	PrintTabs();
	OutputDebugStringA("</node>\n");
	//printf("</node>\n");
}

// 노드의 hierarchy를 들여쓰기로 표현하기 위한 tab을 맴버로 관리하면서 출력해주는 친구다.
void FbxPractice::PrintTabs() const
{
	std::ostringstream oss;
	for (int i = 0; i < numOfTabs; i++)
	{
		oss << "\t";
	}
	OutputDebugStringA(oss.str().c_str());
}

// Node Attribute를 hierarchy에 맞게 출력해주는 친구다.
void FbxPractice::PrintAttribute(FbxNodeAttribute* _pAttribute) const
{
	if (_pAttribute == nullptr) {
		return;
	}
	std::ostringstream oss;

	FbxString typeName = GetAttributeTypeName(_pAttribute->GetAttributeType());
	FbxString attrName = _pAttribute->GetName();
	PrintTabs();
	// FbxString을 char[]로 받고 싶으면 Buffer()를 사용해라
	oss << std::format("<attribute type='{}' name='{}'/>\n", typeName.Buffer(), attrName.Buffer());
	OutputDebugStringA(oss.str().c_str());
	//printf("<attribute type='%s' name='%s;'/>\n", typeName.Buffer(), attrName.Buffer());
}

// Attribute Type enum을 FbxString으로 바꿔주는 함수이다.
FbxString FbxPractice::GetAttributeTypeName(FbxNodeAttribute::EType _type) const
{
	switch (_type) {
		case FbxNodeAttribute::eUnknown: return "unidentified";
		case FbxNodeAttribute::eNull: return "null";
		case FbxNodeAttribute::eMarker: return "marker";
		case FbxNodeAttribute::eSkeleton: return "skeleton";
		case FbxNodeAttribute::eMesh: return "mesh";
		case FbxNodeAttribute::eNurbs: return "nurbs";
		case FbxNodeAttribute::ePatch: return "patch";
		case FbxNodeAttribute::eCamera: return "camera";
		case FbxNodeAttribute::eCameraStereo: return "stereo";
		case FbxNodeAttribute::eCameraSwitcher: return "camera switcher";
		case FbxNodeAttribute::eLight: return "light";
		case FbxNodeAttribute::eOpticalReference: return "optical reference";
		case FbxNodeAttribute::eOpticalMarker: return "marker";
		case FbxNodeAttribute::eNurbsCurve: return "nurbs curve";
		case FbxNodeAttribute::eTrimNurbsSurface: return "trim nurbs surface";
		case FbxNodeAttribute::eBoundary: return "boundary";
		case FbxNodeAttribute::eNurbsSurface: return "nurbs surface";
		case FbxNodeAttribute::eShape: return "shape";
		case FbxNodeAttribute::eLODGroup: return "lodgroup";
		case FbxNodeAttribute::eSubDiv: return "subdiv";
		default: return "unknown";
	}
}

// LayerElement Type enum을 FbxString으로 바꿔주는 함수이다.
FbxString FbxPractice::GetLayerElementTypeName(FbxLayerElement::EType _type) const
{
	switch (_type) {
	case FbxLayerElement::eUnknown: return "unidentified";
	case FbxLayerElement::eNormal: return "Normal";
	case FbxLayerElement::eBiNormal: return "BiNormal";
	case FbxLayerElement::eTangent: return "Tangent";
	case FbxLayerElement::eMaterial: return "Material";
	case FbxLayerElement::ePolygonGroup: return "PolygonGroup";
	case FbxLayerElement::eUV: return "UV";
	case FbxLayerElement::eVertexColor: return "VertexColor";
	case FbxLayerElement::eSmoothing: return "Smoothing";
	case FbxLayerElement::eVertexCrease: return "VertexCrease";
	case FbxLayerElement::eEdgeCrease: return "EdgeCrease";
	case FbxLayerElement::eHole: return "Hole";
	case FbxLayerElement::eUserData: return "UserData";
	case FbxLayerElement::eVisibility: return "Visibility";
	case FbxLayerElement::eTextureDiffuse: return "TextureDiffuse";
	case FbxLayerElement::eTextureDiffuseFactor: return "TextureDiffuseFactor";
	case FbxLayerElement::eTextureEmissive: return "TextureEmissive";
	case FbxLayerElement::eTextureEmissiveFactor: return "TextureEmissiveFactor";
	case FbxLayerElement::eTextureAmbient: return "TextureAmbient";
	case FbxLayerElement::eTextureAmbientFactor: return "TextureAmbientFactor";
	case FbxLayerElement::eTextureSpecular: return "TextureSpecular";
	case FbxLayerElement::eTextureSpecularFactor: return "TextureSpecularFactor";
	case FbxLayerElement::eTextureShininess: return "TextureShininess";
	case FbxLayerElement::eTextureNormalMap: return "TextureNormalMap";
	case FbxLayerElement::eTextureBump: return "TextureBump";
	case FbxLayerElement::eTextureTransparency: return "TextureTransparency";
	case FbxLayerElement::eTextureTransparencyFactor: return "TextureTransparencyFactor";
	case FbxLayerElement::eTextureReflection: return "TextureReflection";
	case FbxLayerElement::eTextureReflectionFactor: return "TextureReflectionFactor";
	case FbxLayerElement::eTextureDisplacement: return "TextureDisplacement";
	default: return "unknown";
	}
}




