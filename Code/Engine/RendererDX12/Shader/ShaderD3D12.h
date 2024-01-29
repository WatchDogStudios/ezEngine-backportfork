/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <RendererDX12/RendererDX12DLL.h>

#include <RendererFoundation/Shader/Shader.h>
#include <RendererCore/Shader/ShaderStageBinary.h>

#include <wrl/client.h>
#include <d3dx12/d3dx12.h>
class EZ_RENDERERDX12_DLL ezGALShaderD3D12 : public ezGALShader
{
public:
private:
  Microsoft::WRL::ComPtr<ID3DBlob> m_shaders[ezGALShaderStage::ENUM_COUNT];
};
