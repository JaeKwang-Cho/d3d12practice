// copyright : AutoDesk FBX

#include "FbxPractice.h"
#include <cassert>
#include <Windows.h>
#include <sstream>
#include <format>
#include <fstream>

#include "FrameResource.h"

FbxPractice::~FbxPractice()
{
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

	// Import Setting을 해준다.
	ioSettings->SetBoolProp(IMP_FBX_MATERIAL, true);
	ioSettings->SetBoolProp(IMP_FBX_TEXTURE, true);
	ioSettings->SetBoolProp(IMP_FBX_LINK, false);
	ioSettings->SetBoolProp(IMP_FBX_SHAPE, false);
	ioSettings->SetBoolProp(IMP_FBX_GOBO, false);
	ioSettings->SetBoolProp(IMP_FBX_ANIMATION, true);
	ioSettings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	// 모델 정보가 들어갈 Scene을 만들고
	rootScene = FbxScene::Create(sdkManager, "rootScene");
	// Import를 건다.
	importer->Import(rootScene);
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

void FbxPractice::SetMesh()
{
	FbxNode* pRootNode = rootScene->GetRootNode();

	OutputDebugStringA(std::format("***** count of ChildCount : '{}' *****\n", pRootNode->GetChildCount()).c_str());
	if (pRootNode != nullptr) {
		for (int i = 0; i < pRootNode->GetChildCount(); i++) {
			FbxNodeAttribute* nodeAttrib = pRootNode->GetChild(i)->GetNodeAttribute();
			if (nodeAttrib->GetAttributeType() == FbxNodeAttribute::eMesh) {
				GetMesh(pRootNode->GetChild(i)->GetMesh());
			}
		}
	}
}

// 예제 따라가면서 해보기
void FbxPractice::GetMesh(const FbxMesh* _pMeshNode)
{
	assert(_pMeshNode != nullptr);
	// 일단... control point의 갯수를 얻는다.
	int polygonVertexCount = _pMeshNode->GetControlPointsCount();

	// material...마다 polygon 갯수를 계산한다. (모름)
	FbxLayerElementArrayTemplate<int>* materialIndiceArr = nullptr;
	FbxGeometryElement::EMappingMode materialMappingMode = FbxGeometryElement::eNone;
	if (_pMeshNode->GetElementMaterial() != nullptr) {
		materialIndiceArr = &_pMeshNode->GetElementMaterial()->GetIndexArray();
		materialMappingMode = _pMeshNode->GetElementMaterial()->GetMappingMode();
		if (materialIndiceArr != nullptr && materialMappingMode == FbxGeometryElement::eByPolygon) {
			
		}
	}

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
		OutputDebugStringA("\t\tfirst edge crease is ... i don't know\n");
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
	oss << std::format("<node name = '{}' translation ='{}, {}, {})' rotation ='({}, {}, {})' scaling='({}, {}, {})'>\n",
		nodeName,
		translation[0], translation[1], translation[2],
		rotation[0], rotation[1], rotation[2],
		scaling[0], scaling[1], scaling[2]);
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




