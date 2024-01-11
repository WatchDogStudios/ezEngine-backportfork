/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

EZ_ALWAYS_INLINE ID3D12Resouce* ezGALTextureD3D12::GetDXTexture() const
{
    return m_finaltextureresource;
}

EZ_ALWAYS_INLINE ID3D12Resouce* ezGALTextureD3D12::GetDXStagingTexture() const
{
    return m_temptextureresource;
}
