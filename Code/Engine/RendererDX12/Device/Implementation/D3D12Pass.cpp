/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#include <RendererDX12/CommandEncoder/CommandEncoderImplD3D12.h>
#include <RendererDX12/Device/D3D12Device.h>
#include <RendererDX12/Device/D3D12Pass.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>

ezGALPassDX12::ezGALPassDX12(ezGALDevice& device)
  : ezGALPass(device)
{
  m_pCommandEncoderState = EZ_DEFAULT_NEW(ezGALCommandEncoderRenderState);
  m_pCommandEncoderImpl = EZ_DEFAULT_NEW(ezGALCommandEncoderImplDX12, static_cast<ezGALDeviceDX12&>(device));

  m_pRenderCommandEncoder = EZ_DEFAULT_NEW(ezGALRenderCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);
  m_pComputeCommandEncoder = EZ_DEFAULT_NEW(ezGALComputeCommandEncoder, device, *m_pCommandEncoderState, *m_pCommandEncoderImpl, *m_pCommandEncoderImpl);

  m_pCommandEncoderImpl->m_pOwner = m_pRenderCommandEncoder.Borrow();
}

ezGALPassDX12::~ezGALPassDX12()
{
}

ezGALRenderCommandEncoder* ezGALPassDX12::BeginRenderingPlatform(const ezGALRenderingSetup& renderingSetup, const char* szName)
{
  m_pCommandEncoderImpl->BeginRendering(renderingSetup);

  return m_pRenderCommandEncoder.Borrow();
}

void ezGALPassDX12::EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pRenderCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

ezGALComputeCommandEncoder* ezGALPassDX12::BeginComputePlatform(const char* szName)
{
  m_pCommandEncoderImpl->BeginCompute();
  return m_pComputeCommandEncoder.Borrow();
}

void ezGALPassDX12::EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder)
{
  EZ_ASSERT_DEV(m_pComputeCommandEncoder.Borrow() == pCommandEncoder, "Invalid command encoder");
}

void ezGALPassDX12::BeginPass(const char* szName)
{
}

void ezGALPassDX12::EndPass()
{
}
