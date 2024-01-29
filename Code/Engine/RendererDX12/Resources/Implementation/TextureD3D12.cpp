
#include "d3dx12/d3dx12.h"
#include <RendererDX12/Device/D3D12Device.h>
#include <RendererDX12/MemoryAllocation/D3D12MemoryAllocation.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererDX12/Resources/TextureD3D12.h>


ezGALTextureD3D12::ezGALTextureD3D12(const ezGALTextureCreationDescription& Description)
  : ezGALTexture(Description)
{
}

ezGALTextureD3D12::~ezGALTextureD3D12() = default;

ezResult ezGALTextureD3D12::InitPlatform(ezGALDevice* pDevice, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData)
{
  /// Cast to device.
  ezGALDeviceDX12* pdxDevice = static_cast<ezGALDeviceDX12*>(pDevice);

  if (m_Description.m_pExisitingNativeObject != nullptr)
  {
    return InitFromNativeObject(pdxDevice);
  }

  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::TextureCube:
    {
      D3D12_RESOURCE_DESC Tex2DDesc = {};
      EZ_SUCCEED_OR_RETURN(Create2DDesc(m_Description, pdxDevice, Tex2DDesc));

      ezHybridArray<D3D12_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);

     
    }
    break;

    case ezGALTextureType::Texture3D:
    {
      D3D12_RESOURCE_DESC Tex3DDesc;
      EZ_SUCCEED_OR_RETURN(Create3DDesc(m_Description, pdxDevice, Tex3DDesc));

      ezHybridArray<D3D12_SUBRESOURCE_DATA, 16> InitialData;
      ConvertInitialData(m_Description, pInitialData, InitialData);

      /// Create the committed resource.
    
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
      return EZ_FAILURE;
  }

  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
    return CreateStagingTexture(pdxDevice);

  return EZ_SUCCESS;
}

ezResult ezGALTextureD3D12::DeInitPlatform(ezGALDevice* pDevice)
{
  EZ_GAL_DX12_RELEASE(m_finaltextureresource);
  EZ_GAL_DX12_RELEASE(m_temptextureresource);
  return EZ_SUCCESS;
}

void ezGALTextureD3D12::SetDebugNamePlatform(const char* szName) const
{
  ezUInt32 uiLength = ezStringUtils::GetStringElementCount(szName);

  if (m_finaltextureresource != nullptr)
  {
    m_finaltextureresource->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

ezResult ezGALTextureD3D12::InitFromNativeObject(ezGALDeviceDX12* pDXDevice)
{
  if (m_finaltextureresource = static_cast<Microsoft::WRL::ComPtr<ID3D12Resource>>(m_Description.m_pExisitingNativeObject) == nullptr || m_Description.m_pExisitingNativeObject == nullptr)
  {
    ezLog::Error("RendererDX12: Object Type of : {0} is invalid!", m_Description.m_pExisitingNativeObject);
  }
  if (!m_Description.m_ResourceAccess.IsImmutable() || m_Description.m_ResourceAccess.m_bReadBack)
  {
    ezResult res = CreateStagingTexture(pDXDevice);
    if (res == EZ_FAILURE)
    {
      m_finaltextureresource = nullptr;
      return res;
    }
  }
  return EZ_SUCCESS;
}

ezResult ezGALTextureD3D12::Create2DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex2DDesc)
{
  out_Tex2DDesc.DepthOrArraySize = (description.m_Type == ezGALTextureType::Texture2D ? description.m_uiArraySize : (description.m_uiArraySize * 6));

  if (description.m_bAllowShaderResourceView || description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
  if (description.m_bAllowUAV)
    out_Tex2DDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  if (description.m_bCreateRenderTarget || description.m_bAllowDynamicMipGeneration)
    out_Tex2DDesc.Flags |= ezGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

  out_Tex2DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_Tex2DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("No storage format available for given format: {0}", description.m_Format);
    return EZ_FAILURE;
  }
  /// NOTE: Since this this is a Texture Cube Resource, The Array Size should be > 6.
  /// Add Extra logic to check if the array size is valid?
  if (description.m_Type == ezGALTextureType::TextureCube)
    out_Tex2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  out_Tex2DDesc.Width = description.m_uiWidth;
  out_Tex2DDesc.Height = description.m_uiHeight;
  out_Tex2DDesc.MipLevels = description.m_uiMipLevelCount;

  out_Tex2DDesc.SampleDesc.Count = description.m_SampleCount;
  out_Tex2DDesc.SampleDesc.Quality = 0;
  return EZ_SUCCESS;
}

ezResult ezGALTextureD3D12::Create3DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex3DDesc)
{
  out_Tex3DDesc.DepthOrArraySize = (description.m_Type == ezGALTextureType::Texture3D ? description.m_uiArraySize : (description.m_uiArraySize * 6));

  if (description.m_bAllowShaderResourceView || description.m_bAllowDynamicMipGeneration)
    out_Tex3DDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_SIMULTANEOUS_ACCESS;
  if (description.m_bAllowUAV)
    out_Tex3DDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

  if (description.m_bCreateRenderTarget || description.m_bAllowDynamicMipGeneration)
    out_Tex3DDesc.Flags |= ezGALResourceFormat::IsDepthFormat(description.m_Format) ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL : D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

  out_Tex3DDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(description.m_Format).m_eStorage;

  if (out_Tex3DDesc.Format == DXGI_FORMAT_UNKNOWN)
  {
    ezLog::Error("No storage format available for given format: {0}", description.m_Format);
    return EZ_FAILURE;
  }

  if (description.m_Type == ezGALTextureType::TextureCube)
    out_Tex3DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

  out_Tex3DDesc.Width = description.m_uiWidth;
  out_Tex3DDesc.Height = description.m_uiHeight;
  out_Tex3DDesc.MipLevels = description.m_uiMipLevelCount;

  out_Tex3DDesc.SampleDesc.Count = description.m_SampleCount;
  out_Tex3DDesc.SampleDesc.Quality = 0;
  return EZ_SUCCESS;
}

void ezGALTextureD3D12::ConvertInitialData(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezHybridArray<D3D12_SUBRESOURCE_DATA, 16>& out_InitialData)
{
  if (!pInitialData.IsEmpty())
  {
    ezUInt32 uiArraySize = 1;
    switch (description.m_Type)
    {
      case ezGALTextureType::Texture2D:
        uiArraySize = description.m_uiArraySize;
        break;
      case ezGALTextureType::TextureCube:
        uiArraySize = description.m_uiArraySize * 6;
        break;
      case ezGALTextureType::Texture3D:
      default:
        break;
    }
    const ezUInt32 uiInitialDataCount = (description.m_uiMipLevelCount * uiArraySize);

    EZ_ASSERT_DEV(pInitialData.GetCount() == uiInitialDataCount, "The array of initial data values is not equal to the amount of mip levels!");

    out_InitialData.SetCountUninitialized(uiInitialDataCount);

    for (ezUInt32 i = 0; i < uiInitialDataCount; i++)
    {
      out_InitialData[i].pData = pInitialData[i].m_pData;
      out_InitialData[i].RowPitch = pInitialData[i].m_uiRowPitch;
      out_InitialData[i].SlicePitch = pInitialData[i].m_uiSlicePitch;
    }
  }
}

ezResult ezGALTextureD3D12::CreateStagingTexture(ezGALDeviceDX12* pDevice)
{
  switch (m_Description.m_Type)
  {
    case ezGALTextureType::Texture2D:
    case ezGALTextureType::TextureCube:
    {
      D3D12_RESOURCE_DESC Desc = m_finaltextureresource->GetDesc();
      // Need to remove this flag on the staging resource or texture readback no longer works.
      Desc.SampleDesc.Count = 1; // We need to disable MSAA for the readback texture, the conversion needs to happen during readback!

      if (m_Description.m_ResourceAccess.m_bReadBack)
        Desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    break;

    default:
      EZ_ASSERT_NOT_IMPLEMENTED;
  }
  return EZ_SUCCESS;
}
