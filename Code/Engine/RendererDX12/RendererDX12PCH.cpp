#include <RendererDX12/RendererDX12PCH.h>

EZ_STATICLINK_LIBRARY(RendererDX12)
{
  if (bReturn)
    return;

  EZ_STATICLINK_REFERENCE(RendererDX12_Context_Implementation_ContextDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Device_Implementation_DeviceDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Device_Implementation_SwapChainDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_BufferDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_FenceDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_QueryDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_RenderTargetViewDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_ResourceViewDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_TextureDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_SharedTextureDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Resources_Implementation_UnorderedAccessViewDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Shader_Implementation_ShaderDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_Shader_Implementation_VertexDeclarationDX12);
  EZ_STATICLINK_REFERENCE(RendererDX12_State_Implementation_StateDX12);
}