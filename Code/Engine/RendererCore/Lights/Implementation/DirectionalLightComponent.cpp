#include <RendererCore/PCH.h>
#include <RendererCore/Lights/DirectionalLightComponent.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDirectionalLightRenderData, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_COMPONENT_TYPE(ezDirectionalLightComponent, 1);
/*  EZ_BEGIN_PROPERTIES
  EZ_END_PROPERTIES*/
  EZ_BEGIN_ATTRIBUTES
    new ezCategoryAttribute("Graphics"),
  EZ_END_ATTRIBUTES
  EZ_BEGIN_MESSAGEHANDLERS
    EZ_MESSAGE_HANDLER(ezUpdateLocalBoundsMessage, OnUpdateLocalBounds),
    EZ_MESSAGE_HANDLER(ezExtractRenderDataMessage, OnExtractRenderData),
  EZ_END_MESSAGEHANDLERS
EZ_END_COMPONENT_TYPE();

ezDirectionalLightComponent::ezDirectionalLightComponent()
{
}

void ezDirectionalLightComponent::OnAfterAttachedToObject()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezDirectionalLightComponent::OnBeforeDetachedFromObject()
{
  if (IsActive())
  {
    // temporary set to inactive so we don't receive the msg
    SetActive(false);
    GetOwner()->UpdateLocalBounds();
    SetActive(true);
  }
}

void ezDirectionalLightComponent::OnUpdateLocalBounds(ezUpdateLocalBoundsMessage& msg) const
{
  // TODO: Infinity!
}

void ezDirectionalLightComponent::OnExtractRenderData( ezExtractRenderDataMessage& msg ) const
{
  ezRenderPipeline* pRenderPipeline = msg.m_pView->GetRenderPipeline();
  ezDirectionalLightRenderData* pRenderData = pRenderPipeline->CreateRenderData<ezDirectionalLightRenderData>(ezDefaultPassTypes::LightGathering, GetOwner());

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_LightColor = m_LightColor;
  pRenderData->m_fIntensity = m_fIntensity;
  pRenderData->m_bCastShadows = m_bCastShadows;
}

void ezDirectionalLightComponent::SerializeComponent(ezWorldWriter& stream) const
{
  ezLightComponent::SerializeComponent(stream);

  ezStreamWriter& s = stream.GetStream();
}

void ezDirectionalLightComponent::DeserializeComponent(ezWorldReader& stream)
{
  ezLightComponent::DeserializeComponent(stream);

  ezStreamReader& s = stream.GetStream();
}


EZ_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_DirectionalLightComponent);
