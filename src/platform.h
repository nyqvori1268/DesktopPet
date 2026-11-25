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
