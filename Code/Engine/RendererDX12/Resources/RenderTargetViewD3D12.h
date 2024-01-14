/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <RendererFoundation/Resources/RenderTargetView.h>

struct ID3D12Resource;
struct ID3D12DesciptorHeap;
struct D3D12_CPU_DESCRIPTOR_RESOURCE;
struct D3D12_RENDER_TARGET_VIEW_DESC;

class ezGALRenderTargetViewD3D12 : public ezGALRenderTargetView
{
public:
  /// @brief Returns the ID3D12Resouce for the Render Target View.
  /// @return
  EZ_ALWAYS_INLINE ID3D12Resource* GetRenderTargetView() const;

  /// @brief Returns the ID3D12Resource for the DepthStencilView.
  /// @return
  EZ_ALWAYS_INLINE ID3D12Resource* GetDepthStencilView() const;

  /// @brief Returns the ID3D12Resource for the UnorderedAccessView
  /// @return
  EZ_ALWAYS_INLINE ID3D12Resource* GetUnorderedAccessView() const;

private:
  ID3D12Resource* m_rtvresource;
  ID3D12Resource* m_dsvresource;
  ID3D12Resource* m_uavresource;

  ID3D12DesciptorHeap* m_dsvdesciptorheap;
  ID3D12DesciptorHeap* m_rtvdesciptorheap;
  ID3D12DesciptorHeap* m_uavdesciptorheap;

protected:
  friend class ezGALDeviceDX12;
  friend class ezMemoryUtils;

  ezGALRenderTargetViewD3D12(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description);

  virtual ~ezGALRenderTargetViewD3D12();

  virtual ezResult InitPlatform(ezGALDevice* pDevice) override;

  virtual ezResult DeInitPlatform(ezGALDevice* pDevice) override;
};

#include <RendererDX12/Resources/Implementation/RenderTargetViewD3D12_inl.h>
