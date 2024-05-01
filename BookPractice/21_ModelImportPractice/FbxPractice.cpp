// copyright : AutoDesk FBX

#include "FbxPractice.h"
#include <cassert>
#include <Windows.h>
#include <sstream>
#include <format>
#include <fstream>

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

// fbx manager�� �ʱ�ȭ �ϴ°�
void FbxPractice::Init()
{
	// SDK �Ŵ��� �����
	sdkManager = FbxManager::Create();
	// IO Settings ������Ʈ �����
	ioSettings = FbxIOSettings::Create(sdkManager, IOSROOT);
	sdkManager->SetIOSettings(ioSettings);
}

// fbx ������ import�ϴ� ��
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

	// Import Setting�� ���ش�.
	ioSettings->SetBoolProp(IMP_FBX_MATERIAL, true);
	ioSettings->SetBoolProp(IMP_FBX_TEXTURE, true);
	ioSettings->SetBoolProp(IMP_FBX_LINK, false);
	ioSettings->SetBoolProp(IMP_FBX_SHAPE, false);
	ioSettings->SetBoolProp(IMP_FBX_GOBO, false);
	ioSettings->SetBoolProp(IMP_FBX_ANIMATION, true);
	ioSettings->SetBoolProp(IMP_FBX_GLOBAL_SETTINGS, true);

	// �� ������ �� Scene�� �����
	rootScene = FbxScene::Create(sdkManager, "rootScene");
	// Import�� �Ǵ�.
	importer->Import(rootScene);
	// Importer�� �Ⱦ��� �����Ѵ�.
	importer->Destroy();
}

// scene�� �ִ� node���� ���� �ִ��� �Ӽ����� print�غ��� ���̴�.
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

void FbxPractice::PrintMeshInfo(FbxMesh* _pMeshNode) const
{
	// FbxGeometry - Deformer
	int deformerCount = _pMeshNode->GetDeformerCount();
	FbxArray<FbxDeformer*> deformers;
	if (deformerCount > 0) {
		OutputDebugStringA("\tMesh has deformer\n");
		for (int i = 0; i < deformerCount; i++) {
			deformers.Add(_pMeshNode->GetDeformer(i));
		}
	}
	// FbxGeometry - weight map
	int weightMapCount = _pMeshNode->GetDestinationGeometryWeightedMapCount();
	if (weightMapCount > 0) {
		OutputDebugStringA("\tMesh has weightMap\n");
	}
	// FbxGeometry - Shape Management
	int shapeCount = _pMeshNode->GetShapeCount();
	if (shapeCount > 0) {
		OutputDebugStringA("\tMesh has shape\n");
	}
	// FbxMesh - Polygon(Vertex)
https://docs.autodesk.com/FBX/2014/ENU/FBX-SDK-Documentation/index.html?url=cpp_ref/class_fbx_mesh.html,topicNumber=cpp_ref_class_fbx_mesh_html22bd1d19-4e92-42fb-be31-952116fdb029
	// FbxMesh - Polygon(Index)
	
	// FbxMesh - UV

	
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
		// smoothing, crease, hole, user data�� ���� �𸣴� �Ѿ��.
		// user data�� ���� �𸣴� �Ѿ��.
	}
	fs.close();
}

void FbxPractice::PrintNode(FbxNode* _pNode) const
{
	std::ostringstream oss;
	PrintTabs();
	const char* nodeName = _pNode->GetName();
	// Local Transform ���� �����´�.
	FbxDouble3 translation = _pNode->LclTranslation.Get();
	FbxDouble3 rotation = _pNode->LclRotation.Get();
	FbxDouble3 scaling = _pNode->LclScaling.Get();

	// node �������� ����Ѵ�.
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

	// node�� �Ӽ����� ����Ѵ�.
	for (int i = 0; i < _pNode->GetNodeAttributeCount(); i++) {
		PrintAttribute(_pNode->GetNodeAttributeByIndex(i));
	}

	// ��������� �ڽĵ鵵 ȣ���Ѵ�.
	for (int j = 0; j < _pNode->GetChildCount(); j++) {
		PrintNode(_pNode->GetChild(j));
	}

	numOfTabs--;
	PrintTabs();
	OutputDebugStringA("</node>\n");
	//printf("</node>\n");
}

// ����� hierarchy�� �鿩����� ǥ���ϱ� ���� tab�� �ɹ��� �����ϸ鼭 ������ִ� ģ����.
void FbxPractice::PrintTabs() const
{
	std::ostringstream oss;
	for (int i = 0; i < numOfTabs; i++)
	{
		oss << "\t";
	}
	OutputDebugStringA(oss.str().c_str());
}

// Node Attribute�� hierarchy�� �°� ������ִ� ģ����.
void FbxPractice::PrintAttribute(FbxNodeAttribute* _pAttribute) const
{
	if (_pAttribute == nullptr) {
		return;
	}
	std::ostringstream oss;

	FbxString typeName = GetAttributeTypeName(_pAttribute->GetAttributeType());
	FbxString attrName = _pAttribute->GetName();
	PrintTabs();
	// FbxString�� char[]�� �ް� ������ Buffer()�� ����ض�
	oss << std::format("<attribute type='{}' name='{}'/>\n", typeName.Buffer(), attrName.Buffer());
	OutputDebugStringA(oss.str().c_str());
	//printf("<attribute type='%s' name='%s;'/>\n", typeName.Buffer(), attrName.Buffer());
}

// Attribute Type enum�� FbxString���� �ٲ��ִ� �Լ��̴�.
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


