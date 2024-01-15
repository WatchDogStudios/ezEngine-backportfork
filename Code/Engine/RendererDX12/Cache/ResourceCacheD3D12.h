/*
 *   Copyright (c) 2023 Watch Dogs LLC
 *   All rights reserved.
 *   You are only allowed access to this code, if given WRITTEN permission by Watch Dogs LLC.
 */

#pragma once

#include <RendererDX12/RendererDX12DLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

/// NOTE: These classes need to be recreated for PC_D3D12 They Still use XBOX_D3D12 for some reason.
#include <RendererDX12/Resources/RenderTargetViewD3D12.h>
#include <RendererDX12/Shader/ShaderD3D12.h>
