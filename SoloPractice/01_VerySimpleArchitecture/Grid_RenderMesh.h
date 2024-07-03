#pragma once
#include "BasicRenderMesh.h"
class Grid_RenderMesh : public BasicRenderMesh
{
public:
protected:
	virtual bool InitRootSignature() override;
	virtual bool InitPipelineState() override;

private:

public:
protected:
private:

public:
	Grid_RenderMesh();
	virtual ~Grid_RenderMesh();
};

