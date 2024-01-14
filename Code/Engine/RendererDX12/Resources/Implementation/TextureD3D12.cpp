#include <RendererDX12/Device/D3D12Device.h>
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
}

void ezGALTextureD3D12::SetDebugNamePlatform(const char* szName) const
{
}

ezResult ezGALTextureD3D12::InitFromNativeObject(ezGALDeviceDX12* pDXDevice)
{
}

ezResult ezGALTextureD3D12::Create2DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex2DDesc)
{
}

ezResult ezGALTextureD3D12::Create3DDesc(const ezGALTextureCreationDescription& description, ezGALDeviceDX12* pDXDevice, D3D12_RESOURCE_DESC& out_Tex3DDesc)
{
}

void ezGALTextureD3D12::ConvertInitialData(const ezGALTextureCreationDescription& description, ezArrayPtr<ezGALSystemMemoryDescription> pInitialData, ezHybridArray<D3D12_SUBRESOURCE_DATA, 16>& out_InitialData)
{
}

ezResult ezGALTextureD3D12::CreateStagingTexture(ezGALDeviceDX12* pDevice)
{
}
