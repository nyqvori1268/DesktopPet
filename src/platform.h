#pragma once
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <filesystem>
#include <vector>
#include "core.h"
namespace pet {
namespace fs=std::filesystem;
std::wstring wide(const std::string&s);
std::string utf8(const std::wstring&s);
inline const wchar_t* tw(TextId id){
 static const auto cache=[](){std::array<std::array<std::wstring,3>,size_t(TextId::Count)> strings;for(size_t i=0;i<strings.size();++i)for(int language=0;language<3;++language)strings[i][size_t(language)]=wide(tr(TextId(i),Language(language)));return strings;}();
 return cache[size_t(id)][size_t(uiLanguage())].c_str();
}
inline std::wstring wf(TextId id,std::initializer_list<std::wstring> arguments){std::vector<std::string> strings;for(auto&argument:arguments)strings.push_back(utf8(argument));return wide(formatText(id,strings));}
inline const wchar_t* uiFontName(){return uiLanguage()==Language::JaJP?L"Yu Gothic UI":uiLanguage()==Language::EnUS?L"Segoe UI":L"Microsoft JhengHei UI";}
inline constexpr UINT LanguageChanged=WM_APP+20;
// LR_SHARED resource icons live for the process; never destroy these handles.
inline HICON brandIcon(int size){return reinterpret_cast<HICON>(LoadImageW(GetModuleHandleW(nullptr),MAKEINTRESOURCEW(1),IMAGE_ICON,size,size,LR_SHARED));}
inline void brandWindow(HWND window){int dpi=int(GetDpiForWindow(window));SendMessageW(window,WM_SETICON,ICON_SMALL,LPARAM(brandIcon(GetSystemMetricsForDpi(SM_CXSMICON,dpi))));SendMessageW(window,WM_SETICON,ICON_BIG,LPARAM(brandIcon(GetSystemMetricsForDpi(SM_CXICON,dpi))));}
bool secureDllLoading();
Language windowsDisplayLanguage();
void broadcastLanguageChange(Language previous);

fs::path executable();
fs::path appData();
fs::path settingsPath();
std::vector<unsigned char> readBytes(const fs::path&p,size_t limit=16*1024*1024);
std::string readText(const fs::path&p,size_t limit=1024*1024);
void atomicWrite(const fs::path&p,const void*data,size_t size);
inline void writeText(const fs::path&p,const std::string&s){atomicWrite(p,s.data(),s.size());}
void saveConfig(const fs::path&p,const Config&c);
std::wstring quote(const std::wstring&s);
PROCESS_INFORMATION launch(const fs::path&exe,const std::vector<std::wstring>&args);
std::string randomId();
std::string sha256(const std::vector<unsigned char>&data);
bool publicKeyValid(const std::string&point);
bool verifySignature(const std::vector<unsigned char>&message,const std::vector<unsigned char>&sig,const std::string&publicKey);
bool signatureOK(const std::vector<unsigned char>&message,const std::vector<unsigned char>&sig);
void rejectReparse(const fs::path&p);
void setStartup(bool enable);
}
