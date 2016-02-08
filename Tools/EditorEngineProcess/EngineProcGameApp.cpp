#include <PCH.h>
#include <EditorEngineProcess/EngineProcGameApp.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <RendererCore/Pipeline/RenderPipelinePass.h>
#include <Foundation/Configuration/Startup.h>

ezEngineProcessGameApplication::ezEngineProcessGameApplication() 
  : ezGameApplication(ezGameApplicationType::EmbeddedInTool, nullptr)
{
  m_pApp = nullptr;
}

void ezEngineProcessGameApplication::BeforeCoreStartup()
{
  int argc = GetArgumentCount();
  const char** argv = GetArgumentsArray();
  m_pApp = new QApplication(argc, (char**)argv);

  ezGameApplication::BeforeCoreStartup();
}

void ezEngineProcessGameApplication::AfterCoreStartup()
{
  // skip project creation at this point
  //ezGameApplication::AfterCoreStartup();

  WaitForDebugger();

  DisableErrorReport();

  ezTaskSystem::SetTargetFrameTime(1000.0 / 10.0);

  ConnectToHost();
}


void ezEngineProcessGameApplication::ConnectToHost()
{
  EZ_VERIFY(m_IPC.ConnectToHostProcess().Succeeded(), "Could not connect to host");

  m_IPC.m_Events.AddEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerIPC, this));

  // wait indefinitely
  m_IPC.WaitForMessage(ezGetStaticRTTI<ezSetupProjectMsgToEngine>(), ezTime());

  // after the ezSetupProjectMsgToEngine was processed, all dynamic plugins should be loaded and we can finally send the reflection information over
  SendReflectionInformation();
}

void ezEngineProcessGameApplication::DisableErrorReport()
{
#if EZ_ENABLED(EZ_PLATFORM_WINDOWS)
  // Setting this flags prevents Windows from showing a dialog when the Engine process crashes
  // this also speeds up process termination significantly (down to less than a second)
  DWORD dwMode = SetErrorMode(SEM_NOGPFAULTERRORBOX);
  SetErrorMode(dwMode | SEM_NOGPFAULTERRORBOX);
#endif
}

void ezEngineProcessGameApplication::WaitForDebugger()
{
  if (ezCommandLineUtils::GetInstance()->GetBoolOption("-debug"))
  {
    while (!IsDebuggerPresent())
    {
      ezThreadUtils::Sleep(10);
    }
  }
}

void ezEngineProcessGameApplication::BeforeCoreShutdown()
{
  m_IPC.m_Events.RemoveEventHandler(ezMakeDelegate(&ezEngineProcessGameApplication::EventHandlerIPC, this));

  ezGameApplication::BeforeCoreShutdown();
}

void ezEngineProcessGameApplication::AfterCoreShutdown()
{
  ezGameApplication::AfterCoreShutdown();

  delete m_pApp;
}

ezApplication::ApplicationExecution ezEngineProcessGameApplication::Run()
{
  ezRenderLoop::ClearMainViews();

  ProcessIPCMessages();

  return ezGameApplication::Run();

}

void ezEngineProcessGameApplication::LogWriter(const ezLoggingEventData & e)
{
  ezLogMsgToEditor msg;
  msg.m_sText = e.m_szText;
  msg.m_sTag = e.m_szTag;
  msg.m_iMsgType = (ezInt8)e.m_EventType;
  msg.m_uiIndentation = e.m_uiIndentation;
  m_IPC.SendMessage(&msg);
}


void ezEngineProcessGameApplication::ProcessIPCMessages()
{
  m_IPC.ProcessMessages();

  if (!m_IPC.IsHostAlive())
    RequestQuit();
}


void ezEngineProcessGameApplication::SendProjectReadyMessage()
{
  ezProjectReadyMsgToEditor msg;
  m_IPC.SendMessage(&msg);
}


void ezEngineProcessGameApplication::SendReflectionInformation()
{
  ezSet<const ezRTTI*> types;
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezComponent>(), types, true);
  ezReflectionUtils::GatherTypesDerivedFromClass(ezGetStaticRTTI<ezRenderPipelinePass>(), types, true);
  //ezReflectionUtils::GatherPluginTypes(types, true);
  ezDynamicArray<const ezRTTI*> sortedTypes;
  ezReflectionUtils::CreateDependencySortedTypeArray(types, sortedTypes);

  for (auto type : sortedTypes)
  {
    ezUpdateReflectionTypeMsgToEditor TypeMsg;
    ezToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(type, TypeMsg.m_desc);
    m_IPC.SendMessage(&TypeMsg);
  }
}

void ezEngineProcessGameApplication::EventHandlerIPC(const ezProcessCommunication::Event& e)
{
  // Sync 
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSyncWithProcessMsgToEngine>())
  {
    ezSyncWithProcessMsgToEditor msg;
    m_IPC.SendMessage(&msg);
    return;
  }

  // Project Messages:
  if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSetupProjectMsgToEngine>())
  {


    const ezSetupProjectMsgToEngine* pSetupMsg = static_cast<const ezSetupProjectMsgToEngine*>(e.m_pMessage);
    ezSetupProjectMsgToEngine* pSetupMsgNonConst = const_cast<ezSetupProjectMsgToEngine*>(pSetupMsg);

    m_sProjectDirectory = pSetupMsg->m_sProjectDir;
    m_CustomFileSystemConfig = pSetupMsgNonConst->m_FileSystemConfig;
    m_CustomPluginConfig = pSetupMsgNonConst->m_PluginConfig;

    // now that we know which project to initialize, do the delayed project setup
    {
      DoProjectSetup();
      ezStartup::StartupEngine();
    }

    // Project setup, we are now ready to accept document messages.
    SendProjectReadyMessage();
    return;
  }
  else if (e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezSimpleConfigMsgToEngine>())
  {
    const ezSimpleConfigMsgToEngine* pMsg = static_cast<const ezSimpleConfigMsgToEngine*>(e.m_pMessage);

    if (pMsg->m_sWhatToDo == "ReloadAssetLUT")
    {
      ezFileSystem::ReloadAllExternalDataDirectoryConfigs();
    }
    else if (pMsg->m_sWhatToDo == "ReloadResources")
    {
      ezResourceManager::ReloadAllResources();
    }
    else
      ezLog::Warning("Unknown ezSimpleConfigMsgToEngine '%s'", pMsg->m_sWhatToDo.GetData());
  }

  // Document Messages:
  if (!e.m_pMessage->GetDynamicRTTI()->IsDerivedFrom<ezEditorEngineDocumentMsg>())
    return;

  const ezEditorEngineDocumentMsg* pDocMsg = (const ezEditorEngineDocumentMsg*)e.m_pMessage;

  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pDocMsg->m_DocumentGuid);

  if (pDocMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenMsgToEngine>()) // Document was opened or closed
  {
    const ezDocumentOpenMsgToEngine* pMsg = static_cast<const ezDocumentOpenMsgToEngine*>(pDocMsg);

    if (pMsg->m_bDocumentOpen)
    {
      pDocumentContext = CreateDocumentContext(pMsg);
      EZ_ASSERT_DEV(pDocumentContext != nullptr, "Could not create a document context for document type '%s'", pMsg->m_sDocumentType.GetData());
    }
    else
    {
      ezEngineProcessDocumentContext::DestroyDocumentContext(pDocMsg->m_DocumentGuid);
    }

    return;
  }

  EZ_ASSERT_DEV(pDocumentContext != nullptr, "Document Context is invalid!");

  pDocumentContext->HandleMessage(pDocMsg);


}

ezEngineProcessDocumentContext* ezEngineProcessGameApplication::CreateDocumentContext(const ezDocumentOpenMsgToEngine* pMsg)
{
  ezDocumentOpenResponseMsgToEditor m;
  m.m_DocumentGuid = pMsg->m_DocumentGuid;
  ezEngineProcessDocumentContext* pDocumentContext = ezEngineProcessDocumentContext::GetDocumentContext(pMsg->m_DocumentGuid);

  if (pDocumentContext == nullptr)
  {
    ezRTTI* pRtti = ezRTTI::GetFirstInstance();
    while (pRtti)
    {
      if (pRtti->IsDerivedFrom<ezEngineProcessDocumentContext>())
      {
        auto* pProp = pRtti->FindPropertyByName("DocumentType");
        if (pProp && pProp->GetCategory() == ezPropertyCategory::Constant)
        {
          const ezStringBuilder sDocTypes(";", static_cast<ezAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<ezString>(), ";");
          const ezStringBuilder sRequestedType(";", pMsg->m_sDocumentType, ";");

          if (sRequestedType.FindSubString(sRequestedType) != nullptr)
          {
            ezLog::Info("Created Context of type '%s' for '%s'", pRtti->GetTypeName(), pMsg->m_sDocumentType.GetData());

            pDocumentContext = static_cast<ezEngineProcessDocumentContext*>(pRtti->GetAllocator()->Allocate());

            ezEngineProcessDocumentContext::AddDocumentContext(pMsg->m_DocumentGuid, pDocumentContext, &m_IPC);
            break;
          }
        }
      }

      pRtti = pRtti->GetNextInstance();
    }
  }
  else
  {
    pDocumentContext->Reset();
  }

  m_IPC.SendMessage(&m);
  return pDocumentContext;
}

void ezEngineProcessGameApplication::DoLoadPluginsFromConfig()
{
  m_CustomPluginConfig.SetOnlyLoadManualPlugins(false); // we also want to load editor plugin dependencies
  m_CustomPluginConfig.Apply();
}


ezString ezEngineProcessGameApplication::FindProjectDirectory() const
{
  return m_sProjectDirectory;
}

void ezEngineProcessGameApplication::DoSetupDataDirectories()
{
  ezStringBuilder sAppDir = ezOSFile::GetApplicationDirectory();
  sAppDir.AppendPath("../../../Shared/Tools/EditorEngineProcess");

  ezOSFile osf;
  osf.CreateDirectoryStructure(sAppDir);

  ezFileSystem::AddDataDirectory(sAppDir.GetData(), ezFileSystem::AllowWrites, "App"); // for everything relative

  m_CustomFileSystemConfig.Apply();
}


void ezEngineProcessGameApplication::ProcessApplicationInput()
{
  // override the escape action to not shut down the app, but instead close the play-the-game window
  if (ezInputManager::GetInputActionState("GameApp", "CloseApp") != ezKeyState::Up)
  {
    for (const auto& state : GetAllGameStates())
    {
      if (state.m_pState)
      {
        state.m_pState->RequestQuit();
      }
    }
  }
  else
  {
    ezGameApplication::ProcessApplicationInput();
  }
}

void ezEngineProcessGameApplication::DoSetupLogWriters()
{
  ezGameApplication::DoSetupLogWriters();

  ezGlobalLog::AddLogWriter(ezMakeDelegate(&ezEngineProcessGameApplication::LogWriter, this));
}

void ezEngineProcessGameApplication::DoShutdownLogWriters()
{
  ezGameApplication::DoShutdownLogWriters();

  ezGlobalLog::RemoveLogWriter(ezMakeDelegate(&ezEngineProcessGameApplication::LogWriter, this));
}

