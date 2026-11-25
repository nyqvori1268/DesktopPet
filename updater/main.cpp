#include "platform.h"
#include "update.h"
#include "zip.h"
#include <set>
using namespace pet;
static const wchar_t* journalName=L".desktoppet-transaction.json";
static const wchar_t* backupName=L".desktoppet-backup";
static const std::vector<std::string> names={"DesktopPet.exe","Updater.exe","data/dialogue.json","README.md","CHANGELOG.md"};
static void copyAtomic(const fs::path&from,const fs::path&to){auto b=readBytes(from);atomicWrite(to,b.data(),b.size());}
static void closeProcess(PROCESS_INFORMATION pi){CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}
static void rollback(const fs::path&target){auto p=target/journalName;if(!fs::exists(p))return;rejectReparse(target);auto j=Json::parse(readText(p));auto backup=target/backupName;rejectReparse(backup);if(j["phase"].str()!="backing"){for(auto&name:names){auto f=fs::path(wide(name));rejectReparse(target/f);rejectReparse(backup/f);if(j["existed"][name].boolean())copyAtomic(backup/f,target/f);else fs::remove(target/f);}}
 fs::remove(p);std::error_code ec;fs::remove_all(backup,ec);
}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
 HANDLE transaction=CreateMutexW(nullptr,TRUE,L"Local\\DesktopPet.UpdateTransaction");if(!transaction||GetLastError()==ERROR_ALREADY_EXISTS){if(transaction)CloseHandle(transaction);return 2;}
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::vector<std::wstring>a;for(int i=1;i<argc;++i)a.emplace_back(argv[i]);LocalFree(argv);fs::path target;bool installed=false;HANDLE child=nullptr;
 try{
  if(a.size()==2&&a[0]==L"--recover"){target=fs::absolute(a[1]);rollback(target);closeProcess(launch(target/L"DesktopPet.exe",{L"--updated"}));return 0;}
  if(a.size()!=3)throw std::runtime_error("Invalid updater arguments");fs::path stage=fs::absolute(a[0]);target=fs::absolute(a[1]);rejectReparse(target);rejectReparse(stage);
  auto info=verifyManifest(readBytes(stage/L"update.json",65536),readBytes(stage/L"update.sig",64));verifyPackage(stage,info);auto bytes=readBytes(stage/L"package.zip");auto files=inspectZip(bytes);
  // Wait for exactly the parent application, never overwrite a running executable.
  DWORD pid=std::stoul(a[2]);HANDLE parent=OpenProcess(SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,FALSE,pid);
  if(parent){wchar_t path[32768];DWORD n=32768;bool correct=QueryFullProcessImageNameW(parent,0,path,&n)&&fs::equivalent(fs::path(path),target/L"DesktopPet.exe");DWORD result=correct?WaitForSingleObject(parent,30000):WAIT_FAILED;CloseHandle(parent);if(!correct||result!=WAIT_OBJECT_0)throw std::runtime_error("主程式尚未結束或程序不符");}
  if(fs::exists(target/journalName))rollback(target);
  auto backup=target/backupName;rejectReparse(backup);if(fs::exists(backup))fs::remove_all(backup);fs::create_directories(backup);
  Json j;j["phase"]="backing";for(auto&name:names){auto f=fs::path(wide(name));rejectReparse(target/f);j["existed"][name]=fs::exists(target/f);}writeText(target/journalName,j.dump());
  for(auto&name:names)if(j["existed"][name].boolean())copyAtomic(target/fs::path(wide(name)),backup/fs::path(wide(name)));
  j["phase"]="prepared";writeText(target/journalName,j.dump());
  for(auto&f:files){auto path=target/fs::path(wide(f.name));rejectReparse(path);atomicWrite(path,bytes.data()+f.offset,f.size);}installed=true;
  j["phase"]="installed";writeText(target/journalName,j.dump());
  auto eventName=L"Local\\DesktopPetReady-"+wide(randomId());HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,eventName.c_str());if(!ready)throw std::runtime_error("Cannot create readiness event");
  auto pi=launch(target/L"DesktopPet.exe",{L"--updated",eventName});child=pi.hProcess;CloseHandle(pi.hThread);HANDLE handles[]={ready,child};DWORD result=WaitForMultipleObjects(2,handles,FALSE,20000);CloseHandle(ready);
  // A ready process must survive a further health window before committing.
  bool good=result==WAIT_OBJECT_0&&WaitForSingleObject(child,3000)==WAIT_TIMEOUT;
  if(!good){TerminateProcess(child,1);WaitForSingleObject(child,5000);CloseHandle(child);child=nullptr;throw std::runtime_error("新版啟動未完成，已回復舊版");}
  CloseHandle(child);child=nullptr;fs::remove(target/journalName);std::error_code ec;fs::remove_all(backup,ec);fs::remove_all(stage,ec);return 0;
 }catch(const std::exception&e){
  if(child){TerminateProcess(child,1);WaitForSingleObject(child,5000);CloseHandle(child);}
  std::string error=e.what();try{if(!target.empty()&&fs::exists(target/journalName))rollback(target);}catch(const std::exception&r){error+="\n回復未完成，請保留 .desktoppet-backup 並重新啟動程式。\n";error+=r.what();}
  auto msg=L"更新未完成：\n"+wide(error);MessageBoxW(nullptr,msg.c_str(),L"DesktopPet 更新器",MB_ICONERROR|MB_OK);
  try{if(!target.empty()&&!fs::exists(target/journalName)&&fs::exists(target/L"DesktopPet.exe"))closeProcess(launch(target/L"DesktopPet.exe",{L"--updated"}));}catch(...){}
  (void)installed;return 1;
 }
}
