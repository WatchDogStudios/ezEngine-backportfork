#include <Foundation/Application/Application.h>
#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/JSONReader.h>
#include <Foundation/Logging/ConsoleWriter.h>
#include <Foundation/Logging/HTMLWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Logging/VisualStudioWriter.h>
#include <Foundation/Memory/StackAllocator.h>
#include <Foundation/Strings/PathUtils.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/System/SystemInformation.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Types/UniquePtr.h>

class ezDependencyAnalysisApp;


bool g_bVerbose = false;

namespace
{
  using DependencyListType = ezHashSet<ezString>;

  struct CompilationUnitContext
  {
    DependencyListType m_dependencies;
    ezDynamicArray<ezString> m_IncludeDirectories;
    ezHashTable<ezString, ezString> m_includeCache;
  };
  using CompilationUnitListType = ezHashTable<ezString, CompilationUnitContext>;

  EZ_ALWAYS_INLINE void SkipWhitespace(ezToken& ref_token, ezUInt32& i, const ezDeque<ezToken>& tokens)
  {
    while (ref_token.m_iType == ezTokenType::Whitespace)
    {
      ref_token = tokens[++i];
    }
  }

  EZ_ALWAYS_INLINE void SkipLine(ezToken& ref_token, ezUInt32& i, const ezDeque<ezToken>& tokens)
  {
    while (ref_token.m_iType != ezTokenType::Newline && ref_token.m_iType != ezTokenType::EndOfFile)
    {
      ref_token = tokens[++i];
    }
  }

  // Json parser for command_line.json files generated by cmake
  class CompileCommandsParser : public ezJSONParser
  {
  public:
    struct CompileCommand
    {
      ezString command;
      ezString file;
    };

    void Initialize(ezStreamReader& inout_stream)
    {
      SetInputStream(inout_stream);
    }

    ezResult GetNextCommand(CompileCommand& out_command)
    {
      while (!m_bObjectOpen)
      {
        if (!ContinueParsing())
        {
          return EZ_FAILURE;
        }
      }

      while (m_bObjectOpen)
      {
        if (!ContinueParsing())
        {
          return EZ_FAILURE;
        }
      }

      out_command = m_Command;
      return EZ_SUCCESS;
    }

  private:
    // Inherited via ezJSONParser
    virtual bool OnVariable(ezStringView sVarName) override
    {
      bool needed = false;

      if (sVarName == "file")
      {
        needed = true;
        m_pNextVar = &m_Command.file;
      }
      else if (sVarName == "command")
      {
        needed = true;
        m_pNextVar = &m_Command.command;
      }

      if (!needed && sVarName != "directory" && sVarName != "output")
      {
        ezStringBuilder fmt;
        fmt.Format("Unknown variable '{0}'", sVarName);
        ParsingError(fmt.GetView(), false);
      }
      return needed;
    }

    virtual void OnReadValue(ezStringView sValue) override
    {
      if (m_pNextVar == nullptr)
      {
        ParsingError("Unexpected value", true);
        return;
      }

      *m_pNextVar = sValue;
      m_pNextVar = nullptr;
    }

    virtual void OnReadValue(double fValue) override
    {
    }

    virtual void OnReadValue(bool bValue) override
    {
    }

    virtual void OnReadValueNULL() override
    {
    }

    virtual void OnBeginObject() override
    {
      if (!m_bOuterArrayOpen || m_bObjectOpen)
      {
        ParsingError("Unexpected object begin", true);
        return;
      }

      m_bObjectOpen = true;
      m_Command = {};
    }

    virtual void OnEndObject() override
    {
      if (!m_bObjectOpen)
      {
        ParsingError("Unexpected object end", true);
        return;
      }
      m_bObjectOpen = false;
    }

    virtual void OnBeginArray() override
    {
      if (!m_bOuterArrayOpen)
      {
        m_bOuterArrayOpen = true;
      }
      else
      {
        ParsingError("Unexpected array begin", true);
      }
    }

    virtual void OnEndArray() override
    {
      if (m_bOuterArrayOpen)
      {
        m_bOuterArrayOpen = false;
      }
      else
      {
        ParsingError("Unexpected array end", true);
      }
    }

    bool m_bOuterArrayOpen = false;
    bool m_bObjectOpen = false;
    ezString* m_pNextVar = nullptr;
    CompileCommand m_Command;
  };

  // ezOSFile::ExistsFile is quite slow, cache the results thread local to avoid lock contetion
  struct ExistsFileCache
  {
    static thread_local ezHashTable<ezString, bool> m_ExistsFileCache;

    static bool ExistsFile(ezStringView sFile)
    {
      bool existedAlready = false;
      bool returnValue = false;
      {
        bool& result = m_ExistsFileCache.FindOrAdd(sFile, &existedAlready);
        if (!existedAlready)
        {
          result = ezOSFile::ExistsFile(sFile);
        }
        returnValue = result;
      }
      return returnValue;
    }
  };

  thread_local ezHashTable<ezString, bool> ExistsFileCache::m_ExistsFileCache;

  // Task to parse a single header file
  // stores the results in the task, need to be extracted after finish using MoveOutDependencies
  class ParseHeaderFileTask : public ezTask
  {
  public:
    ParseHeaderFileTask()
    {
      ConfigureTask("ParseHeaderFileTask", ezTaskNesting::Maybe);
    }

    void Initialize(ezStringView sPath)
    {
      m_sPath = sPath;
    }

    virtual void Execute() override;

    ezStringView GetPath() const { return m_sPath; }
    ezHashSet<ezString>&& MoveOutDependencies() { return std::move(m_Dependencies); }

  private:
    ezString m_sPath;
    ezHashSet<ezString> m_Dependencies;
    ezTaskGroupID m_TaskGroup;
  };

  // Task to collect the dependencies of a single compilation unit
  // Might need to run multiple times (call HasWorkLeft to know if it needs to be executed again)
  // Needs to spawn other tasks when HasWorkLeft returns true: see ScheduleRemainingWork
  class CollectDependenciesTask : public ezTask
  {
  public:
    CollectDependenciesTask()
    {
      ConfigureTask("CollectDependenciesTask", ezTaskNesting::Maybe);
    }

    void Initialize(ezStringView sPath, CompilationUnitContext& ref_context, ezDependencyAnalysisApp& ref_app)
    {
      m_sPath = sPath;
      m_pContext = &ref_context;
      m_pApp = &ref_app;
    }

    void OnParsingFinished(ezTaskGroupID taskGroupID);

    virtual void Execute() override;

    bool HasWorkLeft() const { return m_LeftoverFiles.GetCount() > 0; }

    void ScheduleRemainingWork();

  private:
    ezStringView m_sPath;
    CompilationUnitContext* m_pContext = nullptr;
    ezDependencyAnalysisApp* m_pApp = nullptr;
    ezDynamicArray<ezSharedPtr<ParseHeaderFileTask>> m_ParseTasks;
    ezDynamicArray<ezString> m_LeftoverFiles;
    ezTaskGroupID m_ParseTaskGroup;
  };
} // namespace

/*
* This application finds all dependencies of a given compile setup.
*
* Currently requires a compile_commands.json generated with cmake targeting the clang compiler
*
* Basic flow
* - Parse the compile_command.json to find all .cpp files (compilation units)
* - Parse each compilation unit, and find all headers it includes
* - Parse all found headers and note their dependencies (storing only the unresolved paths)
* - For each compilation unit resolve all dependencies by doing a breadth first search in parallel
*   + Whenever we encounter a header file we have not seen yet, we need to parse it
* - Write out the results into a json file. For each compilation units all header dependencies will be listed.
*/
class ezDependencyAnalysisApp : public ezApplication
{
private:
  friend CollectDependenciesTask;

  ezString m_sCompileCommandsPath;
  bool m_bHadErrors;
  bool m_bHadSeriousWarnings;
  bool m_bHadWarnings;
  ezDynamicArray<ezString> m_IgnorePatterns;
  ezString m_sOutputPath;

  CompilationUnitListType m_CompilationUnits;
  ezMutex m_HeaderDependenciesMutex;
  ezHashTable<ezString, DependencyListType> m_HeaderDependencies;

  ezMutex m_HeaderParsingScheduledMutex;
  ezHashSet<ezString> m_HeaderParsingScheduled;

public:
  using SUPER = ezApplication;

  ezDependencyAnalysisApp()
    : ezApplication("DependencyAnalysis")
  {
    m_bHadErrors = false;
    m_bHadSeriousWarnings = false;
    m_bHadWarnings = false;
  }

  /// Makes sure the apps return value reflects whether there were any errors or warnings
  static void LogInspector(const ezLoggingEventData& eventData)
  {
    ezDependencyAnalysisApp* app = (ezDependencyAnalysisApp*)ezApplication::GetApplicationInstance();

    switch (eventData.m_EventType)
    {
      case ezLogMsgType::ErrorMsg:
        app->m_bHadErrors = true;
        break;
      case ezLogMsgType::SeriousWarningMsg:
        app->m_bHadSeriousWarnings = true;
        break;
      case ezLogMsgType::WarningMsg:
        app->m_bHadWarnings = true;
        break;

      default:
        break;
    }
  }


  ezResult ParseArray(const ezVariant& value, ezHashSet<ezString>& ref_dst)
  {
    if (!value.CanConvertTo<ezVariantArray>())
    {
      ezLog::Error("Expected array");
      return EZ_FAILURE;
    }
    auto a = value.Get<ezVariantArray>();
    const auto arraySize = a.GetCount();
    for (ezUInt32 i = 0; i < arraySize; i++)
    {
      auto& el = a[i];
      if (!el.CanConvertTo<ezString>())
      {
        ezLog::Error("Value {0} at index {1} can not be converted to a string. Expected array of strings.", el, i);
        return EZ_FAILURE;
      }
      ezStringBuilder file = el.Get<ezString>();
      file.ToLower();
      ref_dst.Insert(file);
    }
    return EZ_SUCCESS;
  }

  virtual void AfterCoreSystemsStartup() override
  {
    ezGlobalLog::AddLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::AddLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
    ezGlobalLog::AddLogWriter(LogInspector);


    ezSystemInformation info = ezSystemInformation::Get();
    const ezInt32 iCpuCores = info.GetCPUCoreCount();
    ezTaskSystem::SetWorkerThreadCount(iCpuCores);

    if (GetArgumentCount() < 2)
    {
      ezLog::Info("DependencyAnalysis.exe [options] path/to/compile_commands.json");
      ezLog::Info("");
      ezLog::Info("-i [pattern] --ignorePattern [pattern]    ignore all cpp/h files containing this pattern");
      ezLog::Info("-v           --verbose                    enable verbose logging");
      ezLog::Info("-o [path]    --output [path]              path to the output json file");
      ezLog::Info("");

      ezLog::Error("No command line arguments given.");
      return;
    }

    // Add the empty data directory to access files via absolute paths
    ezFileSystem::AddDataDirectory("", "App", ":", ezFileSystem::AllowWrites).IgnoreResult();

    // pass the absolute path to the directory that should be scanned as the first parameter to this application
    ezStringBuilder sCompileCommandsPath;

    auto numArgs = GetArgumentCount();
    auto shortIgnore = ezStringView("-i");
    auto longIgnore = ezStringView("--ignorePattern");
    auto shortVerbose = ezStringView("-v");
    auto longVerbose = ezStringView("--verbose");
    auto shortOutput = ezStringView("-o");
    auto longOutput = ezStringView("--output");
    for (ezUInt32 argi = 1; argi < numArgs; argi++)
    {
      auto arg = ezStringView(GetArgument(argi));
      if (arg == shortIgnore || arg == longIgnore)
      {
        if (numArgs <= argi + 1)
        {
          ezLog::Error("Missing path for {0}", arg);
          return;
        }
        ezStringBuilder ignorePattern = GetArgument(argi + 1);
        if (ignorePattern == shortIgnore || ignorePattern == longIgnore || ignorePattern == shortIgnore || ignorePattern == longIgnore || ignorePattern == shortOutput || ignorePattern == longOutput)
        {
          ezLog::Error("Missing path for {0} found {1} instead", arg, ignorePattern.GetView());
          return;
        }
        argi++;
        ignorePattern.MakeCleanPath();
        m_IgnorePatterns.PushBack(ignorePattern);
      }
      else if (arg == shortVerbose || arg == longVerbose)
      {
        g_bVerbose = true;
      }
      else if (arg == shortOutput || arg == longOutput)
      {
        if (!m_sOutputPath.IsEmpty())
        {
          ezLog::Error("Output path given multiple times");
          return;
        }
        if (numArgs <= argi + 1)
        {
          ezLog::Error("Missing path for {0}", arg);
          return;
        }
        ezStringBuilder outputPath = GetArgument(argi + 1);
        if (outputPath == shortIgnore || outputPath == longIgnore || outputPath == shortIgnore || outputPath == longIgnore || outputPath == shortOutput || outputPath == longOutput)
        {
          ezLog::Error("Missing path for {0} found {1} instead", arg, outputPath.GetView());
          return;
        }
        if (!outputPath.IsAbsolutePath())
        {
          outputPath = ezOSFile::GetCurrentWorkingDirectory();
          outputPath.AppendPath(GetArgument(argi + 1));
        }
        outputPath.MakeCleanPath();
        argi++;
        m_sOutputPath = outputPath;
      }
      else
      {
        if (sCompileCommandsPath.IsEmpty())
        {
          sCompileCommandsPath = arg;
          sCompileCommandsPath.MakeCleanPath();
        }
        else
        {
          ezLog::Error("Currently only one directory is supported for searching. Did you forget -i|--includeDir? {0}", arg);
        }
      }
    }

    if (!ezPathUtils::IsAbsolutePath(sCompileCommandsPath.GetData()))
      ezLog::Error("The given path is not absolute: '{0}'", sCompileCommandsPath);

    m_sCompileCommandsPath = sCompileCommandsPath;

    if (m_sOutputPath.IsEmpty())
    {
      ezLog::Error("No output path given. Use -o or --output");
    }
  }

  virtual void BeforeCoreSystemsShutdown() override
  {
    SetReturnCode(0);

    ezGlobalLog::RemoveLogWriter(LogInspector);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::Console::LogMessageHandler);
    ezGlobalLog::RemoveLogWriter(ezLogWriter::VisualStudio::LogMessageHandler);
  }

  static ezResult ReadEntireFile(ezStringView sFile, ezStringBuilder& out_sContents)
  {
    out_sContents.Clear();

    ezFileReader File;
    if (File.Open(sFile) == EZ_FAILURE)
    {
      ezLog::Error("Could not open for reading: '{0}'", sFile);
      return EZ_FAILURE;
    }

    ezDynamicArray<ezUInt8> FileContent;

    ezUInt8 Temp[4024];
    ezUInt64 uiRead = File.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp));

    while (uiRead > 0)
    {
      FileContent.PushBackRange(ezArrayPtr<ezUInt8>(Temp, (ezUInt32)uiRead));

      uiRead = File.ReadBytes(Temp, EZ_ARRAY_SIZE(Temp));
    }

    FileContent.PushBack(0);

    if (!ezUnicodeUtils::IsValidUtf8((const char*)&FileContent[0]))
    {
      ezLog::Error("The file \"{0}\" contains characters that are not valid Utf8. This often happens when you type special characters in "
                   "an editor that does not save the file in Utf8 encoding.",
        sFile);
      return EZ_FAILURE;
    }

    out_sContents = (const char*)&FileContent[0];

    return EZ_SUCCESS;
  }

  static ezResult FindIncludeFile(ezStringView sPath, CompilationUnitContext& inout_context, ezStringBuilder& out_sPath)
  {
    if (sPath.IsAbsolutePath())
    {
      out_sPath = sPath;
      out_sPath.MakeCleanPath();
      if (ExistsFileCache::ExistsFile(sPath))
      {
        return EZ_SUCCESS;
      }
      else
      {
        return EZ_FAILURE;
      }
    }

    for (auto& includeDir : inout_context.m_IncludeDirectories)
    {
      out_sPath = includeDir;
      out_sPath.AppendPath(sPath);
      out_sPath.MakeCleanPath();
      if (ExistsFileCache::ExistsFile(out_sPath))
      {
        return EZ_SUCCESS;
      }
    }

    return EZ_FAILURE;
  }

  bool IgnorePath(const ezStringBuilder& sBuilder)
  {
    for (auto& ignore : m_IgnorePatterns)
    {
      if (sBuilder.FindSubString(ignore))
      {
        return true;
      }
    }
    return false;
  }

  void ParseCompileCommandsJson()
  {
    ezFileReader reader;
    reader.Open(m_sCompileCommandsPath).AssertSuccess(); // TODO error handling

    CompileCommandsParser parser;
    parser.SetLogInterface(ezLog::GetThreadLocalLogSystem());
    parser.Initialize(reader);

    ezDynamicArray<ezStringView> commandParts;

    ezStringBuilder tmpPath;

    CompileCommandsParser::CompileCommand command;
    while (parser.GetNextCommand(command).Succeeded())
    {
      if (g_bVerbose)
      {
        ezLog::Info("file: {0}", command.file);
      }
      tmpPath = command.file;
      tmpPath.MakeCleanPath();

      if (IgnorePath(tmpPath))
      {
        continue;
      }

      if (tmpPath.GetFileExtension().Compare_NoCase("cpp") != 0)
      {
        continue;
      }

      auto& context = m_CompilationUnits[tmpPath];

      ezDynamicArray<ezString> includeFiles;

      command.command.Split(false, commandParts, " ");
      const ezUInt32 numParts = commandParts.GetCount();
      for (ezUInt32 i = 0; i < numParts; i++)
      {
        auto part = commandParts[i];
        if (part.Compare_NoCase("-isystem") == 0)
        {
          i++;
          part = commandParts[i];
          tmpPath = part;
          tmpPath.MakeCleanPath();

          if (!IgnorePath(tmpPath)) // TODO make command line parameter
          {
            if (g_bVerbose)
            {
              ezLog::Info(tmpPath);
            }
            context.m_IncludeDirectories.PushBack(tmpPath);
          }
        }
        else if (part.TrimWordStart("-include"))
        {
          tmpPath = part;
          tmpPath.MakeCleanPath();
          if (g_bVerbose)
          {
            ezLog::Info(tmpPath);
          }
          includeFiles.PushBack(tmpPath);
        }
        else if (part.TrimWordStart("-I"))
        {
          tmpPath = part;
          tmpPath.MakeCleanPath();
          if (!IgnorePath(tmpPath))
          {
            if (g_bVerbose)
            {
              ezLog::Info(tmpPath);
            }
            context.m_IncludeDirectories.PushBack(tmpPath);
          }
        }
      }

      for (auto& includeFile : includeFiles)
      {
        if (FindIncludeFile(includeFile.GetView(), context, tmpPath).Succeeded())
        {
          context.m_dependencies.Insert(tmpPath);
        }
        else
        {
          if (g_bVerbose)
          {
            ezLog::Warning("Could not find include file '{0}' specified on the command line in any include path", includeFile);
          }
        }
      }
    }
  }

  void ParseCompilationUnits()
  {
    struct CompUnit
    {
      ezStringView path;
      CompilationUnitContext* context;
    };
    ezDynamicArray<CompUnit> compilationUnits;
    compilationUnits.Reserve(m_CompilationUnits.GetCount());

    for (auto& compilationUnit : m_CompilationUnits)
    {
      compilationUnits.PushBack({compilationUnit.Key().GetView(), &compilationUnit.Value()});
    }

    ezTaskSystem::ParallelFor(
      compilationUnits.GetArrayPtr(), [this](ezArrayPtr<CompUnit> compUnits) {
        ezStringBuilder tmpPath;
        DependencyListType dependencies;
        for (auto& compUnit : compUnits)
        {
          ezStringView currentFile = compUnit.path;

          EZ_LOG_BLOCK("CPP", currentFile);
          dependencies.Clear();
          FindDependencies(currentFile, dependencies);
          for (auto& dependency : dependencies)
          {
            if (!dependency.IsAbsolutePath())
            {
              if (FindIncludeFile(dependency.GetView(), *compUnit.context, tmpPath).Succeeded())
              {
                compUnit.context->m_dependencies.Insert(tmpPath);
              }
              else
              {
                if (g_bVerbose)
                {
                  ezLog::Warning("Warning dependency '{0}' of '{1}' not found", dependency, currentFile);
                }
              }
            }
          }
        }
      },
      "ParseCompilationUnit");
  }

  void ParseHeaderFiles()
  {
    ezStringBuilder tmpPath;
    ezDynamicArray<ezSharedPtr<ParseHeaderFileTask>> filesToCheckTasks;

    auto taskGroup = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
    for (auto& compilationUnit : m_CompilationUnits)
    {
      for (auto& dependency : compilationUnit.Value().m_dependencies)
      {
        bool existed = false;
        if (auto& dependencies = m_HeaderDependencies.FindOrAdd(dependency.GetView(), &existed); !existed)
        {
          filesToCheckTasks.PushBack(EZ_DEFAULT_NEW(ParseHeaderFileTask));
          filesToCheckTasks.PeekBack()->Initialize(dependency.GetView());
          ezTaskSystem::AddTaskToGroup(taskGroup, filesToCheckTasks.PeekBack());
        }
      }
    }

    ezTaskSystem::StartTaskGroup(taskGroup);
    ezTaskSystem::WaitForGroup(taskGroup);

    for (auto& task : filesToCheckTasks)
    {
      m_HeaderDependencies[task->GetPath()] = task->MoveOutDependencies();
    }
  }

  void CheckDependentFiles()
  {
    ezDynamicArray<ezSharedPtr<CollectDependenciesTask>> tasks;

    for (auto& compilationUnit : m_CompilationUnits)
    {
      tasks.PushBack(EZ_DEFAULT_NEW(CollectDependenciesTask));
      tasks.PeekBack()->Initialize(compilationUnit.Key(), compilationUnit.Value(), *this);
    }

    const ezUInt32 totalNumTasks = tasks.GetCount();

    // First iteration
    {
      auto group = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
      for (auto& task : tasks)
      {
        ezTaskSystem::AddTaskToGroup(group, task);
      }
      ezTaskSystem::StartTaskGroup(group);
      ezTaskSystem::WaitForGroup(group);
    }

    while (true)
    {
      for (ezUInt32 i = 0; i < tasks.GetCount();)
      {
        if (tasks[i]->HasWorkLeft())
        {
          tasks[i]->ScheduleRemainingWork();
          ++i;
        }
        else
        {
          tasks.RemoveAtAndSwap(i);
        }
      }

      if (tasks.GetCount() == 0) // If none of the tasks had work left, we are done
      {
        break;
      }

      ezLog::Info("Collecting dependencies. Remaining {0} of {1} total", tasks.GetCount(), totalNumTasks);

      ezTaskSystem::FinishFrameTasks(); // Wait for parsing tasks to finish before doing the next round of dependency collection

      auto group = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame);
      for (auto& task : tasks)
      {
        ezTaskSystem::AddTaskToGroup(group, task);
      }
      ezTaskSystem::StartTaskGroup(group);
      ezTaskSystem::WaitForGroup(group);
    }
  }

  void WriteOutResults()
  {
    ezFileWriter FileOut;

    if (FileOut.Open(m_sOutputPath) == EZ_FAILURE)
    {
      ezLog::Error("Could not open the file for writing: '{0}'", "results.json");
      return;
    }
    else
    {
      ezStringBuilder json;
      json.Append("{\n");
      json.Append(" \"files\" : [\n");

      auto processFile = [this, &json](CompilationUnitListType::Iterator& file) {
        json.Append("    {\n");
        json.AppendFormat("      \"name\" : \"{0}\",\n", file.Key());
        json.Append("      \"dependencies\" :\n");
        json.Append("        [\n");

        auto count = (*file).Value().m_dependencies.GetCount();
        if (count > 0)
        {
          count--;
          auto depIt = (*file).Value().m_dependencies.GetIterator();

          for (ezUInt32 i = 0; i < count; ++i, depIt.Next())
          {
            json.AppendFormat("       \"{0}\",\n", (*depIt).GetView());
          }
          json.AppendFormat("       \"{0}\"\n", (*depIt).GetView());
        }
        json.Append("        ]\n");
        json.Append("    }");
      };

      auto count = m_CompilationUnits.GetCount();
      if (count > 0)
      {
        count--;
        auto fileIt = m_CompilationUnits.GetIterator();
        for (ezUInt32 i = 0; i < count; ++i, fileIt.Next())
        {
          processFile(fileIt);
          json.Append(",\n");
        }
        processFile(fileIt);
        json.Append("\n");
      }
      json.Append("  ]\n");
      json.Append("}\n");

      FileOut.WriteBytes(json.GetData(), json.GetElementCount()).AssertSuccess();
    }
  }

  static void FindDependencies(ezStringView sCurrentFile, DependencyListType& inout_dependencies)
  {
    ezStringBuilder fileContents;
    ReadEntireFile(sCurrentFile, fileContents).IgnoreResult();

    auto fileDir = sCurrentFile.GetFileDirectory();

    ezTokenizer tokenizer;
    auto dataView = fileContents.GetView();
    auto start = dataView.GetStartPointer();
    auto elementCount = dataView.GetElementCount();
    if (elementCount >= 3 && ezUnicodeUtils::SkipUtf8Bom(start))
    {
      elementCount -= 3;
    }
    tokenizer.Tokenize(ezArrayPtr<const ezUInt8>(reinterpret_cast<const ezUInt8*>(start), elementCount), ezLog::GetThreadLocalLogSystem());

    ezStringView hash("#");
    ezStringView include("include");
    ezStringView openAngleBracket("<");
    ezStringView closeAngleBracket(">");

    auto tokens = tokenizer.GetTokens();
    const auto numTokens = tokens.GetCount();
    for (ezUInt32 i = 0; i < numTokens; i++)
    {
      auto curToken = tokens[i];
      while (curToken.m_iType == ezTokenType::Whitespace)
      {
        curToken = tokens[++i];
      }
      if (curToken.m_iType == ezTokenType::NonIdentifier && curToken.m_DataView == hash)
      {
        do
        {
          curToken = tokens[++i];
        } while (curToken.m_iType == ezTokenType::Whitespace);

        if (curToken.m_iType == ezTokenType::Identifier && curToken.m_DataView == include)
        {
          auto includeToken = curToken;
          do
          {
            curToken = tokens[++i];
          } while (curToken.m_iType == ezTokenType::Whitespace);

          if (curToken.m_iType == ezTokenType::String1)
          {
            // #include "bla"
            ezStringBuilder absIncludePath;
            ezStringBuilder relativePath;
            relativePath = curToken.m_DataView;
            relativePath.Trim("\"");
            relativePath.MakeCleanPath();
            absIncludePath = fileDir;
            absIncludePath.AppendPath(relativePath);

            if (!ExistsFileCache::ExistsFile(absIncludePath))
            {
              inout_dependencies.Insert(relativePath);
            }
            else
            {
              inout_dependencies.Insert(absIncludePath);
            }
          }
          else if (curToken.m_iType == ezTokenType::NonIdentifier && curToken.m_DataView == openAngleBracket)
          {
            // #include <bla>
            bool error = false;
            auto startToken = curToken;
            do
            {
              curToken = tokens[++i];
              if (curToken.m_iType == ezTokenType::Newline)
              {
                ezLog::Error("Non-terminated '<' in #include {0} line {1}", sCurrentFile, includeToken.m_uiLine);
                error = true;
                break;
              }
            } while (curToken.m_iType != ezTokenType::NonIdentifier || curToken.m_DataView != closeAngleBracket);

            if (error)
            {
              // in case of error skip the malformed line in hopes that we can recover from the error.
              do
              {
                curToken = tokens[++i];
              } while (curToken.m_iType != ezTokenType::Newline);
            }
            else
            {
              ezStringBuilder includePath;
              includePath = ezStringView(startToken.m_DataView.GetEndPointer(), curToken.m_DataView.GetStartPointer());
              includePath.MakeCleanPath();
              inout_dependencies.Insert(includePath);
            }
          }
          else
          {
            // error
            ezLog::Error("Can not parse #include statement in {0} line {1}", sCurrentFile, includeToken.m_uiLine);
          }
        }
        else
        {
          while (curToken.m_iType != ezTokenType::Newline && curToken.m_iType != ezTokenType::EndOfFile)
          {
            curToken = tokens[++i];
          }
        }
      }
      else
      {
        while (curToken.m_iType != ezTokenType::Newline && curToken.m_iType != ezTokenType::EndOfFile)
        {
          curToken = tokens[++i];
        }
      }
    }
  }

  virtual ezApplication::Execution Run() override
  {
    // something basic has gone wrong
    if (m_bHadSeriousWarnings || m_bHadErrors)
      return ezApplication::Execution::Quit;

    auto start = ezTimestamp::CurrentTimestamp();

    if (g_bVerbose)
    {
      ezLog::Info("Parse compile commands json");
    }
    ParseCompileCommandsJson();

    auto parseJsonEnd = ezTimestamp::CurrentTimestamp();

    if (g_bVerbose)
    {
      ezLog::Info("Parse compilation units");
    }
    ParseCompilationUnits();

    auto parseCompilationUnitsEnd = ezTimestamp::CurrentTimestamp();

    if (g_bVerbose)
    {
      ezLog::Info("Parse header files");
    }
    ParseHeaderFiles();

    auto parseHeaderFilesEnd = ezTimestamp::CurrentTimestamp();

    if (g_bVerbose)
    {
      ezLog::Info("Build dependencies");
    }
    CheckDependentFiles();

    auto checkDependendtFilesEnd = ezTimestamp::CurrentTimestamp();

    if (g_bVerbose)
    {
      ezLog::Info("Write out results");
    }
    WriteOutResults();

    auto writeOutResultsEnd = ezTimestamp::CurrentTimestamp();

    ezLog::Info("Time to parse compile commands json: {0}s", (parseJsonEnd - start).AsFloatInSeconds());
    ezLog::Info("Time to parse compilation units: {0}s", (parseCompilationUnitsEnd - parseJsonEnd).AsFloatInSeconds());
    ezLog::Info("Time to parse header files: {0}s", (parseHeaderFilesEnd - parseCompilationUnitsEnd).AsFloatInSeconds());
    ezLog::Info("Time to build dependencies: {0}s", (checkDependendtFilesEnd - parseHeaderFilesEnd).AsFloatInSeconds());
    ezLog::Info("Time to write out results: {0}s", (writeOutResultsEnd - checkDependendtFilesEnd).AsFloatInSeconds());
    ezLog::Info("Total time taken: {0}s", (writeOutResultsEnd - start).AsFloatInSeconds());

    return ezApplication::Execution::Quit;
  }
};

void ParseHeaderFileTask::Execute()
{
  ezDependencyAnalysisApp::FindDependencies(m_sPath, m_Dependencies);
}

void CollectDependenciesTask::Execute()
{
  EZ_ASSERT_DEBUG(m_pContext != nullptr && m_pApp != nullptr, "Task was not initialized properly");

  auto& compilationUnitDependencies = m_pContext->m_dependencies;

  ezDynamicArray<ezString> filesToCheck;
  if (m_LeftoverFiles.GetCount() > 0)
  {
    filesToCheck.Swap(m_LeftoverFiles);
    m_ParseTasks.Clear();
  }
  else
  {
    for (auto& include : compilationUnitDependencies)
    {
      filesToCheck.PushBack(include);
    }
  }

  ezHashSet<ezString> seenFiles;

  while (!filesToCheck.IsEmpty())
  {
    auto include = std::move(filesToCheck.PeekBack());
    filesToCheck.PopBack();

    if (seenFiles.Insert(include))
    {
      continue;
    }

    bool alreadyExisted = false;

    DependencyListType* headerDependencies = nullptr;
    if (!m_pApp->m_HeaderDependencies.TryGetValue(include.GetView(), headerDependencies))
    {
      {
        EZ_LOCK(m_pApp->m_HeaderParsingScheduledMutex);
        if (!m_pApp->m_HeaderParsingScheduled.Insert(include.GetView()))
        {
          if (m_ParseTasks.GetCount() == 0)
          {
            m_ParseTaskGroup = ezTaskSystem::CreateTaskGroup(ezTaskPriority::EarlyThisFrame, ezMakeDelegate(&CollectDependenciesTask::OnParsingFinished, this));
          }
          ezSharedPtr<ParseHeaderFileTask> task = EZ_DEFAULT_NEW(ParseHeaderFileTask);
          task->Initialize(include);
          ezTaskSystem::AddTaskToGroup(m_ParseTaskGroup, task);
          m_ParseTasks.PushBack(std::move(task));
        }
      }
    }
    else
    {
      ezStringBuilder tmpPath;
      for (auto& dependency : *headerDependencies)
      {
        if (m_pApp->FindIncludeFile(dependency.GetView(), *m_pContext, tmpPath).Succeeded())
        {
          if (!compilationUnitDependencies.Contains(tmpPath.GetView()))
          {
            compilationUnitDependencies.Insert(tmpPath);
            filesToCheck.PushBack(tmpPath);
          }
        }
        else
        {
          if (g_bVerbose)
          {
            ezLog::Warning("Failed to find dependency '{0}' of compilation unit '{1}'", dependency, m_sPath);
          }
        }
      }
    }
  }
}

void CollectDependenciesTask::ScheduleRemainingWork()
{
  if (m_ParseTasks.GetCount() > 0)
  {
    ezTaskSystem::StartTaskGroup(m_ParseTaskGroup);
  }
}

void CollectDependenciesTask::OnParsingFinished(ezTaskGroupID taskGroupId)
{
  EZ_LOCK(m_pApp->m_HeaderDependenciesMutex);
  for (auto& task : m_ParseTasks)
  {
    bool alreadyExisted = false;
    auto& deps = m_pApp->m_HeaderDependencies.FindOrAdd(task->GetPath(), &alreadyExisted);
    if (!alreadyExisted)
    {
      deps = task->MoveOutDependencies();
    }
  }
}

EZ_CONSOLEAPP_ENTRY_POINT(ezDependencyAnalysisApp);