/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererFoundation/Device/Device.h>

#include <d3d12.h>
#include <dxgi.h>

struct ID3D12CommandAllocator3;
struct ID3D12CommandList3;
struct ID3D12CommandQueue3;
struct ID3D12Device3;
struct ID3D12Debug2;
struct IDXGIFactory3;
struct IDXGIAdapter3;
struct IDXGIDevice3;
struct ID3D12Resource2;
struct ID3D12Query;
struct IDXGIAdapter3;

using ezGALFormatLookupEntryDX12 = ezGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0>;
using ezGALFormatLookupTableDX12 = ezGALFormatLookupTable<ezGALFormatLookupEntryDX12>;

class ezGALPassDX12;

/// @brief The DX12 device implementation of the graphics abstraction layer.

class EZ_RENDERERDX12_DLL ezGALDeviceDX12 : public ezGALDevice
{
private:
  friend ezInternal::NewInstance<ezGALDevice> CreateDX12Device(ezAllocatorBase* pAllocator, const ezGALDeviceCreationDescription& desc);
  ezGALDeviceDX12(const ezGALDeviceCreationDescription& desc);

public:
  virtual ~ezGALDeviceDX12() {}

public:
  ID3D12Device3* GetDXDevice() const;
  IDXGIFactory3* GetDXGIFactory() const;
  ID3D12CommandAllocator3* GetDirectCommandAllocator() const;
  ID3D12CommandAllocator3* GetComputeCommandAllocator() const;
  ID3D12CommandAllocator3* GetCopyCommandAllocator() const;

  ezGALRenderCommandEncoder* GetRenderCommandEncoder() const;

  const ezGALFormatLookupTableDX12& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

protected:
  /// @brief Internal function init D3D12 for PC. (See GDXKPlatform target for Xbox Implementation).
  /// @param platformfeaturelevel: see m_uiFeatureLevel for guidance.
  /// @param wantedadapterused: if there is a certain GPU that we should use, set it is this.
  /// @return Status of the Initialization.
  ezResult InitPlatform(D3D_FEATURE_LEVEL platformfeaturelevel, IDXGIAdapter1* wantedadapterused);

  virtual ezResult InitPlatform() override;
  virtual ezResult ShutdownPlatform() override;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, ezGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(ezGALSwapChain* pSwapChain) override;

  virtual ezGALPass* BeginPassPlatform(const char* szName) override;
  virtual void EndPassPlatform(ezGALPass* pPass) override;

  virtual void FlushPlatform() override;

  // State creation functions

  virtual ezGALBlendState* CreateBlendStatePlatform(const ezGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(ezGALBlendState* pBlendState) override;

  virtual ezGALDepthStencilState* CreateDepthStencilStatePlatform(const ezGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(ezGALDepthStencilState* pDepthStencilState) override;

  virtual ezGALRasterizerState* CreateRasterizerStatePlatform(const ezGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(ezGALRasterizerState* pRasterizerState) override;

  virtual ezGALSamplerState* CreateSamplerStatePlatform(const ezGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(ezGALSamplerState* pSamplerState) override;

  // Resource creation functions

  virtual ezGALShader* CreateShaderPlatform(const ezGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(ezGALShader* pShader) override;

  virtual ezGALBuffer* CreateBufferPlatform(const ezGALBufferCreationDescription& Description, ezArrayPtr<const ezUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(ezGALBuffer* pBuffer) override;

  virtual ezGALTexture* CreateTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALTexture* CreateSharedTexturePlatform(const ezGALTextureCreationDescription& Description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezEnum<ezGALSharedTextureType> sharedType, ezGALPlatformSharedHandle handle) override;
  virtual void DestroySharedTexturePlatform(ezGALTexture* pTexture) override;

  virtual ezGALResourceView* CreateResourceViewPlatform(ezGALResourceBase* pResource, const ezGALResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(ezGALResourceView* pResourceView) override;

  virtual ezGALRenderTargetView* CreateRenderTargetViewPlatform(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(ezGALRenderTargetView* pRenderTargetView) override;

  ezGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(ezGALResourceBase* pResource, const ezGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(ezGALUnorderedAccessView* pUnorderedAccessView) override;

  // Other rendering creation functions

  virtual ezGALQuery* CreateQueryPlatform(const ezGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(ezGALQuery* pQuery) override;

  virtual ezGALVertexDeclaration* CreateVertexDeclarationPlatform(const ezGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(ezGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual ezGALTimestampHandle GetTimestampPlatform() override;
  virtual ezResult GetTimestampResultPlatform(ezGALTimestampHandle hTimestamp, ezTime& result) override;

  // Swap chain functions

  void PresentPlatform(const ezGALSwapChain* pSwapChain, bool bVSync);

  // Misc functions

  virtual void BeginFramePlatform(const ezUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  virtual const ezGALSharedTexture* GetSharedTexture(ezGALTextureHandle hTexture) const override;

  /// \endcond
  friend class ezGALCommandEncoderImplDX12;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,
      ENUM_COUNT
    };
  };

  ID3D12Query* GetTimestamp(ezGALTimestampHandle hTimestamp);

  ID3D12Resource* FindTempBuffer(ezUInt32 uiSize);
  ID3D12Resource* FindTempTexture(ezUInt32 uiWidth, ezUInt32 uiHeight, ezUInt32 uiDepth, ezGALResourceFormat::Enum format);
  void FreeTempResources(ezUInt64 uiFrame);

  void FillFormatLookupTable();

  void IezertFencePlatform(ID3D12Device3* pDevice, ID3D12CommandQueue3* pQueue, ID3D12Query* pFence);

  bool IsFenceReachedPlatform(ID3D12Device3* pDevice, ID3D12CommandQueue3* pQueue, ID3D12Query* pFence);

  void WaitForFencePlatform(ID3D12Device3* pDevice, ID3D12CommandQueue3* pQueue, ID3D12Query* pFence);

  /// @brief only use this for temp functions, don't use as the main device. ieztead, use m_pDevice3 (ID3D12Device3)
  ID3D12Device* m_pDevice = nullptr;
  ID3D12Device3* m_pDevice3 = nullptr;

  ID3D12Debug1* m_pDebug = nullptr;
  ID3D12DebugDevice1* m_pDebugDevice = nullptr;

  IDXGIFactory3* m_pDXGIFactory = nullptr;

  IDXGIAdapter1* m_pDXGIAdapter = nullptr;

  IDXGIDevice3* m_pDXGIDevice = nullptr;

  ezGALFormatLookupTableDX12 m_FormatLookupTable;

  /// @brief Does the GPU Support DX12U? if so, this allows RT, Mesh Shaders, Work Graphs, and more.
  bool m_pDX12Ultimate = false;

  // NOLINTNEXTLINE
  /// D3D_FEATURE_LEVEL can't be forward declared, This should only be 2 values: D3D_FEATURE_LEVEL_12_1 (Normal) D3D_FEATURE_LEVEL_12_2(DX12 Ultimate, Ray tracing, Mesh Shaders, Etc...)
  ezUInt32 m_uiFeatureLevel;
  ezUInt32 m_uiDxgiFlags = 0;

  ezUniquePtr<ezGALPassDX12> m_pDefaultPass;

  struct PerFrameData
  {
    ID3D12Fence* m_pFence = nullptr;
    ID3D12Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    ezUInt64 m_uiFrame = -1;
  };

  PerFrameData m_PerFrameData[4];
  ezUInt8 m_uiCurrentPerFrameData = 0;
  ezUInt8 m_uiNextPerFrameData = 0;

  ezUInt64 m_uiFrameCounter = 0;

  struct UsedTempResource
  {
    EZ_DECLARE_POD_TYPE();

    ID3D12Resource* m_pResource;
    ezUInt64 m_uiFrame;
    ezUInt32 m_uiHash;
  };

  ezMap<ezUInt32, ezDynamicArray<ID3D12Resource*>, ezCompareHelper<ezUInt32>, ezLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  ezDeque<UsedTempResource, ezLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  ezDynamicArray<ID3D12Query*, ezLocalAllocatorWrapper> m_Timestamps;
  ezUInt32 m_uiCurrentTimestamp = 0;
  ezUInt32 m_uiNextTimestamp = 0;

  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;

  ezTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};

/// NOTE: Our inl files are located strangely.
#include <RendererDX12/Device/Implementation/D3D12Device_inl.h>
