/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <RendererFoundation/Resources/Texture.h>

struct ID3D12Resource;
struct D3D12_RESOURCE_DESC;
struct D3D12_SUBRESOURCE_DATA;
class ezGALDeviceDX12;

EZ_DEFINE_AS_POD_TYPE(D3D12_SUBRESOURCE_DATA);

class ezGALTextureD3D12 : public ezGALTexture
{
public:
  EZ_ALWAYS_INLINE ID3D12Resource* GetDXTexture() const;

  EZ_ALWAYS_INLINE ID3D12Resource* GetDXStagingTexture() const;

protected:
  friend class ezGALDeviceDX12;
  friend class ezMemoryUtils;
  friend class ezGALSharedTextureDX12;

  ezGALTextureD3D12(const ezGALTextureCreationDescription& Description);
  ~ezGALTextureD3D12();

  virtual ezResult InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
  virtual void SetDebugNamePlatform(const char* szName) const override;

  ezResult InitFromNativeObject(ezGALDeviceDX12* pDXDevice);
  static ezResult Create2DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex2DDesc);
  static ezResult Create3DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex3DDesc);
  static void ConvertInitialData(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezHybridArray<D3D12_SUBRESOURCE_DATA, 16>& out_InitialData);
  ezResult CreateStagingTexture(ezGALDeviceDX12* pDevice);

protected:
  ID3D12Resource* m_finaltextureresource;
  ID3D12Resource* m_temptextureresource;
};

#include <RendererDX12/Resources/Implementation/TextureD3D12_inl.h>