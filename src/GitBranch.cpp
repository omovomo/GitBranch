#include <plugin.hpp>
#include <initguid.h>
#include "version.hpp"
#include "guid.hpp"
#include "command.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>


#include <atomic>
#include <chrono>
#include <codecvt>
#include <condition_variable>
#include <filesystem>
#include <locale>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

constexpr wchar_t EnvVarDIR[] = L"SH_DIR";
constexpr wchar_t EnvVarGIT[] = L"SH_GIT";
constexpr wchar_t EnvVarStarship[] = L"STARSHIP_CONFIG";

std::chrono::milliseconds SynchroFarRequestTimeout{333};
std::chrono::seconds ForceUpdateTimeout{5};

std::atomic<bool> Running = true;

PluginStartupInfo PSI;
FarStandardFunctions FSF;
HANDLE Heap;

std::mutex PauseMutex;
std::condition_variable PauseVariable;
std::thread Thread;

std::wstring PreviousDir;
std::chrono::time_point<std::chrono::steady_clock> PreviousUpdateTimePoint =
    std::chrono::steady_clock::now();

void WINAPI GetGlobalInfoW(struct GlobalInfo *Info) {
  Info->StructSize = sizeof(struct GlobalInfo);
  Info->MinFarVersion = MAKEFARVERSION(FARMANAGERVERSION_MAJOR, FARMANAGERVERSION_MINOR,
                                       FARMANAGERVERSION_REVISION, 5555, VS_RELEASE);
  Info->Version = PLUGIN_VERSION;
  Info->Guid = MainGuid;
  Info->Title = PLUGIN_NAME;
  Info->Description = PLUGIN_DESC;
  Info->Author = PLUGIN_AUTHOR;
}

void Run();

void WINAPI SetStartupInfoW(const PluginStartupInfo *psi) {
  char buf[MAX_PATH];
  size_t len = GetTempPathA(MAX_PATH, buf);
  std::string path{buf};

  spdlog::set_default_logger(spdlog::basic_logger_mt("plugin", path + "\\gitbranch.log"));
  spdlog::flush_on(spdlog::level::debug);
  spdlog::set_pattern("%d %b %H:%M:%S.%e [%-5P:%5t] [%l] %v");
  spdlog::set_level(spdlog::level::debug);

  spdlog::info("SetStartupInfoW: enter");

  PSI = *psi;
  FSF = *psi->FSF;
  PSI.FSF = &FSF;
  Heap = GetProcessHeap();

  SetEnvironmentVariableW(EnvVarGIT, L"");
  SetEnvironmentVariableW(EnvVarDIR, L"");

  std::filesystem::path sh_config = PSI.ModuleName;
  sh_config.remove_filename();
  sh_config /= "starship.toml";
  SetEnvironmentVariableW(EnvVarStarship, sh_config.wstring().c_str());

  Thread = std::thread(Run);

  spdlog::info("SetStartupInfoW: exit");
}

void WINAPI GetPluginInfoW(struct PluginInfo *Info) {
  Info->StructSize = sizeof(*Info);
  Info->Flags = PF_PRELOAD;
}

HANDLE WINAPI OpenW(const struct OpenInfo *) { return NULL; }

void WINAPI ExitFARW(const ExitInfo *) {
  spdlog::info("ExitFARW: enter, [Running: {}]", Running.load());

  Running = false;
  PauseVariable.notify_one();

  if (Thread.joinable()) {
    Thread.join();
  }
  spdlog::info("ExitFARW: exit, Plugin thread joined, [Running: {}]", Running.load());
}

void Run() {
  spdlog::info("Plugin thread stated, [Running: {}]", Running.load());
  while (Running) {
    PSI.AdvControl(&MainGuid, ACTL_SYNCHRO, 0, nullptr);

    std::unique_lock<std::mutex> lock(PauseMutex);
    PauseVariable.wait_for(lock, SynchroFarRequestTimeout, [] { return !Running; });
  }
  spdlog::info("Plugin thread exit, [Running: {}]", Running.load());
}

std::wstring GetEnvVars();
bool Timeout();

intptr_t WINAPI ProcessSynchroEventW(const struct ProcessSynchroEventInfo *) {
  //Get current directory
  std::wstring directory;
  if (const size_t length =
          static_cast<size_t>(PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, 0, NULL))) {
    if (auto pd = static_cast<FarPanelDirectory *>(HeapAlloc(Heap, HEAP_ZERO_MEMORY, length))) {
      pd->StructSize = sizeof(FarPanelDirectory);
      if (PSI.PanelControl(PANEL_ACTIVE, FCTL_GETPANELDIRECTORY, static_cast<int>(length), pd)) {
        directory = pd->Name;
      }
      HeapFree(Heap, 0, pd);
    }
  }

  if (PreviousDir != directory || Timeout()) {
    std::wstring sh_vars;
    if (!directory.empty()) {
      std::filesystem::path git_dir = directory;
      auto git_dir_str = git_dir.string();
      if(git_dir_str.find_first_of(' ') != std::string::npos){
        git_dir_str = "\"" + git_dir_str + "\"";
      }
      auto cmdres = raymii::Command::exec("starship prompt -p " + git_dir_str);
      std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
      sh_vars = converter.from_bytes(cmdres.output);
    }
    PreviousDir = directory;
    PreviousUpdateTimePoint = std::chrono::steady_clock::now();
    if (GetEnvVars() != sh_vars) {
      auto delim_pos = sh_vars.find_first_of(L"|");
      SetEnvironmentVariableW(EnvVarDIR, sh_vars.substr(0, delim_pos).c_str());
      SetEnvironmentVariableW(EnvVarGIT, sh_vars.substr(delim_pos + 1, sh_vars.size() - delim_pos - 1).c_str());
      PSI.AdvControl(&MainGuid, ACTL_REDRAWALL, 0, nullptr);
    }
  }

  return 0;
}

bool Timeout() {
  return std::chrono::steady_clock::now() - PreviousUpdateTimePoint > ForceUpdateTimeout;
}

std::wstring GetEnvVars() {
  wchar_t buf[1024];
  return std::wstring{buf, GetEnvironmentVariableW(EnvVarDIR, buf, 1024)} + L"|" +
         std::wstring{buf, GetEnvironmentVariableW(EnvVarGIT, buf, 1024)};
}
