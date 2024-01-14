/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

EZ_ALWAYS_INLINE ID3D12Resource* ezGALRenderTargetViewD3D12::GetRenderTargetView() const
{
  return m_rtvresource;
}

EZ_ALWAYS_INLINE ID3D12Resource* ezGALRenderTargetViewD3D12::GetDepthStencilView() const
{
  return m_dsvresource;
}

EZ_ALWAYS_INLINE ID3D12Resource* ezGALRenderTargetViewD3D12::GetUnorderedAccessView() const
{
  return m_uavresource;
}
