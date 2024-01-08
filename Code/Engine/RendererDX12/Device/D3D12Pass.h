/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <RendererDX12/RendererDX12PCH.h>

#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Pass.h>

struct ezGALCommandEncoderRenderState;
class ezGALRenderCommandEncoder;
class ezGALComputeCommandEncoder;

class ezGALCommandEncoderImplDX12;

class EZ_RENDERERDX12_DLL ezGALPassDX12 : public ezGALPass
{
protected:
  friend class ezGALDeviceDX12;
  friend class ezMemoryUtils;

  ezGALPassDX12(ezGALDevice& device);
  virtual ~ezGALPassDX12();

  virtual ezGALRenderCommandEncoder* BeginRenderingPlatform(coezt ezGALRenderingSetup& renderingSetup, coezt char* szName) override;
  virtual void EndRenderingPlatform(ezGALRenderCommandEncoder* pCommandEncoder) override;

  virtual ezGALComputeCommandEncoder* BeginComputePlatform(coezt char* szName) override;
  virtual void EndComputePlatform(ezGALComputeCommandEncoder* pCommandEncoder) override;

  void BeginPass(coezt char* szName);
  void EndPass();

private:
  ezUniquePtr<ezGALCommandEncoderRenderState> m_pCommandEncoderState;
  ezUniquePtr<ezGALCommandEncoderImplDX12> m_pCommandEncoderImpl;

  ezUniquePtr<ezGALRenderCommandEncoder> m_pRenderCommandEncoder;
  ezUniquePtr<ezGALComputeCommandEncoder> m_pComputeCommandEncoder;
};
