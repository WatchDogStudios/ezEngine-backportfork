#pragma once

#include <Foundation/Application/Application.h>
#include <Foundation/IO/DirectoryWatcher.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererCore/Material/MaterialResource.h>
#include <RendererCore/Meshes/MeshResource.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class ezShaderExplorerWindow;
class ezCamera;
class ezGALDevice;
class ezDirectoryWatcher;

// A simple application that renders a full screen quad with a single shader and does live reloading of that shader
// Can be used for singed distance rendering experiments or other single shader experiments.
class ezShaderExplorerApp : public ezApplication
{
public:
  using SUPER = ezApplication;

  ezShaderExplorerApp();

  virtual Execution Run() override;

  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeHighLevelSystemsShutdown() override;

private:
  void UpdateSwapChain();
  void CreateScreenQuad();
  void OnFileChanged(ezStringView sFilename, ezDirectoryWatcherAction action, ezDirectoryWatcherType type);

  ezShaderExplorerWindow* m_pWindow = nullptr;
  ezGALDevice* m_pDevice = nullptr;

  ezGALSwapChainHandle m_hSwapChain;
  ezGALTextureHandle m_hDepthStencilTexture;

  ezMaterialResourceHandle m_hMaterial;
  ezMeshBufferResourceHandle m_hQuadMeshBuffer;

  ezUniquePtr<ezCamera> m_pCamera;
  ezUniquePtr<ezDirectoryWatcher> m_pDirectoryWatcher;

  bool m_bStuffChanged;
};