#include <RendererDX12/RendererDX12PCH.h>

#include <RendererDX12/Device/D3D12Device.h>
#include <RendererDX12/Resources/RenderTargetViewD3D12.h>
#include <RendererDX12/Resources/TextureD3D12.h>

ezGALRenderTargetViewD3D12::ezGALRenderTargetViewD3D12(ezGALTexture* pTexture, const ezGALRenderTargetViewCreationDescription& Description)
  : ezGALRenderTargetView(pTexture, Description)
{
}

ezGALRenderTargetViewD3D12::~ezGALRenderTargetViewD3D12()
{
}

ezResult ezGALRenderTargetViewD3D12::InitPlatform(ezGALDevice* pDevice)
{
}

ezResult ezGALRenderTargetViewD3D12::DeInitPlatform(ezGALDevice* pDevice)
{
}