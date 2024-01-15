/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

EZ_ALWAYS_INLINE ID3D12Resource* ezGALTextureD3D12::GetDXTexture() const
{
  return (ID3D12Resource*)m_finaltextureresource.GetAddressOf();
}

EZ_ALWAYS_INLINE ID3D12Resource* ezGALTextureD3D12::GetDXStagingTexture() const
{
  return (ID3D12Resource*)m_temptextureresource.GetAddressOf();
}
