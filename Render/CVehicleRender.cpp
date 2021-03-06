#include "CVehicleRender.h"
#include "CTextureMaps.h"
#include "CRender.h"
#include "RenderWare.h"
#include "CPatch.h"
#include "CDebug.h"
#include "D3D9Headers\d3d9.h"

ID3DXEffect *CVehicleRender::m_pEffect;

void CVehicleRender::Patch()
{
	CPatch::SetPointer(0x5D9FE4, RenderCB);
}

bool CVehicleRender::Setup()
{
	ID3DXBuffer *errors;
	HRESULT result = D3DXCreateEffectFromFile(g_Device,"resources/Shaders/vehicle.fx", 0, 0, 0, 0, &m_pEffect, &errors);
	if(!CDebug::CheckForShaderErrors(errors, "CVehicleRender", "vechicle", result))
	{
		return false;
	}
	return true;
}

void CVehicleRender::Reset()
{
	if(m_pEffect)
		m_pEffect->OnResetDevice();
}

void CVehicleRender::Lost()
{
	if(m_pEffect)
		m_pEffect->OnLostDevice();
}

void CVehicleRender::RenderCB(RwResEntry *repEntry, RpAtomic *atomic, unsigned char type, char flags)
{
	D3DXCOLOR vehicleColor;
	RxD3D9InstanceData *mesh;
	UINT passes;
	D3DXMATRIX worldViewProj,vp,world;
	RpGeometry *geometry = atomic->geometry;
	unsigned int geometryFlags = geometry->flags;
	rwD3D9EnableClippingIfNeeded(atomic, type);
	if(repEntry->header.indexBuffer)
		rwD3D9SetIndices(repEntry->header.indexBuffer);
	rwD3D9SetStreams(repEntry->header.vertexStream, repEntry->header.useOffsets);
	rwD3D9SetVertexDeclaration(repEntry->header.vertexDeclaration);
	rwD3D9VSSetActiveWorldMatrix(RwFrameGetLTM(atomic->object.object.parent));
	rwD3D9VSGetWorldNormalizedTransposeMatrix(&world);
	getWorldViewProj(&worldViewProj,RwFrameGetLTM(atomic->object.object.parent),&vp);
	m_pEffect->SetMatrix("gmWorldViewProj",&worldViewProj);
	m_pEffect->SetMatrix("gmWorld",&world);
	mesh = &repEntry->meshData;
	// Render of meshes
	for(unsigned int i = 0; i < repEntry->header.numMeshes; i++)
	{
		//rwD3D9RenderStateVertexAlphaEnable(mesh->vertexAlpha || mesh->material->color.alpha != 255);
		vehicleColor.r = (float)mesh->material->color.red / 255.0f;
		vehicleColor.g = (float)mesh->material->color.green / 255.0f;
		vehicleColor.b = (float)mesh->material->color.blue / 255.0f;
		vehicleColor.a = (float)mesh->material->color.alpha / 255.0f;
		m_pEffect->SetVector("gvPaintColor", (D3DXVECTOR4 *)&vehicleColor);
		m_pEffect->SetFloat("gfSpecularFactor", (float)mesh->material->m_pReflection->m_ucIntensity* 0.0039215686f);
		if(flags & (rpGEOMETRYTEXTURED2|rpGEOMETRYTEXTURED)) {
			CRender::SetTextureMaps((STexture*)mesh->material->texture,m_pEffect);
		}
		GetCurrentStates();
		RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
		RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
		RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATESRCBLEND, (void*)rwBLENDONE);
		RwEngineInstance->dOpenDevice.fpRenderStateSet(rwRENDERSTATEDESTBLEND, (void*)rwBLENDZERO);
		m_pEffect->Begin(&passes,0);
		m_pEffect->BeginPass(0);
		m_pEffect->CommitChanges();
		if(repEntry->header.indexBuffer)
		{
			rwD3D9DrawIndexedPrimitive(repEntry->header.primType, mesh->baseIndex, 0, mesh->numVertices,
				mesh->startIndex, mesh->numPrimitives);
		}
		else
			rwD3D9DrawPrimitive(repEntry->header.primType, mesh->baseIndex, mesh->numPrimitives);
		m_pEffect->EndPass();
		m_pEffect->End();
		SetOldStates();
		mesh++;
	}
}