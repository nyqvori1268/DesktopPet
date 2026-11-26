#include "platform.h"
#include "update.h"
#include "zip.h"
#include "handoff.h"
#include "trust_store.h"
#include "form.h"
#include <set>
using namespace pet;
struct StageWindow{
 HWND window=nullptr;HFONT font=nullptr;
 ~StageWindow(){if(window)DestroyWindow(window);if(font)DeleteObject(font);}
 void show(const wchar_t*text){if(!window){POINT point{};GetCursorPos(&point);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(point,MONITOR_DEFAULTTONEAREST),&mi);window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TOPMOST,L"STATIC",L"",WS_POPUP|WS_BORDER|SS_CENTER|SS_CENTERIMAGE,mi.rcWork.left+32,mi.rcWork.top+32,360,88,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);int dpi=int(GetDpiForWindow(window));font=CreateFontW(-MulDiv(14,dpi,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());SendMessageW(window,WM_SETFONT,WPARAM(font),TRUE);int width=MulDiv(360,dpi,96),height=MulDiv(88,dpi,96);SetWindowPos(window,HWND_TOPMOST,(mi.rcWork.left+mi.rcWork.right-width)/2,(mi.rcWork.top+mi.rcWork.bottom-height)/2,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);}SetWindowTextW(window,text);::UpdateWindow(window);}
};
static DWORD responsiveWait(DWORD count,const HANDLE*handles,DWORD timeout){ULONGLONG until=GetTickCount64()+timeout;for(;;){auto time=GetTickCount64();if(time>=until)return WAIT_TIMEOUT;DWORD result=MsgWaitForMultipleObjects(count,handles,FALSE,DWORD(until-time),QS_ALLINPUT);if(result!=WAIT_OBJECT_0+count)return result;MSG message{};while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)){TranslateMessage(&message);DispatchMessageW(&message);}}}
static const wchar_t* journalName=L".desktoppet-transaction.json";
static const wchar_t* backupName=L".desktoppet-backup";
static const std::vector<std::string> names={"DesktopPet.exe","Updater.exe","data/dialogue.json","README.md","CHANGELOG.md"};
static void writeInstalled(const fs::path&to,const void*data,size_t size){for(int attempt=0;;++attempt){try{atomicWrite(to,data,size);return;}catch(...){if(attempt>=19)throw;Sleep(250);}}}
static void copyAtomic(const fs::path&from,const fs::path&to){auto b=readBytes(from);writeInstalled(to,b.data(),b.size());}
static void closeProcess(PROCESS_INFORMATION pi){CloseHandle(pi.hThread);CloseHandle(pi.hProcess);}
static void rollback(const fs::path&target){auto p=target/journalName;if(!fs::exists(p))return;rejectReparse(target);auto j=Json::parse(readText(p));auto backup=target/backupName;rejectReparse(backup);if(j["phase"].str()!="backing"){for(auto&name:names){auto f=fs::path(wide(name));rejectReparse(target/f);rejectReparse(backup/f);if(j["existed"][name].boolean())copyAtomic(backup/f,target/f);else fs::remove(target/f);}}
 fs::remove(p);std::error_code ec;fs::remove_all(backup,ec);
}
static void savedLanguage(const fs::path&target){try{auto config=target/L"config.json";if(!fs::exists(config))config=appData()/L"config.json";auto j=Json::parse(readText(config));setUILanguage(j["language"].str().empty()?Language::ZhTW:languageFromCode(j["language"].str()));}catch(...){}}
int WINAPI wWinMain(HINSTANCE,HINSTANCE,PWSTR,int){
 HANDLE transaction=CreateMutexW(nullptr,TRUE,L"Local\\DesktopPet.UpdateTransaction");if(!transaction||GetLastError()==ERROR_ALREADY_EXISTS){if(transaction)CloseHandle(transaction);return 2;}
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);std::vector<std::wstring>a;for(int i=1;i<argc;++i)a.emplace_back(argv[i]);LocalFree(argv);fs::path target,stage;StageWindow progressUI;bool installed=false,parentExited=false,recovering=false;HANDLE child=nullptr,parent=nullptr,accepted=nullptr,cancelled=nullptr;
 try{
  if(a.size()==10){if(a[8]!=L"--language")throw std::runtime_error(tr(TextId::S259));setUILanguage(languageFromCode(utf8(a[9])));a.resize(8);}
  if(a.size()==2&&a[0]==L"--recover"){savedLanguage(fs::absolute(a[1]));HANDLE running=OpenMutexW(SYNCHRONIZE,FALSE,L"Local\\DesktopPet.SingleInstance");if(running){CloseHandle(running);throw std::runtime_error(tr(TextId::S256));}if(GetLastError()!=ERROR_FILE_NOT_FOUND)throw std::runtime_error(tr(TextId::S257));recovering=true;target=fs::absolute(a[1]);rollback(target);closeProcess(launch(target/L"DesktopPet.exe",{L"--updated"}));return 0;}
  if(a.size()!=8||a[0]!=L"--handoff")throw std::runtime_error(tr(TextId::S258));
  auto number=[](const std::wstring&text){if(text.empty()||text.find_first_not_of(L"0123456789")!=std::wstring::npos)throw std::runtime_error(tr(TextId::S259));size_t n=0;auto value=std::stoull(text,&n);if(n!=text.size())throw std::runtime_error(tr(TextId::S259));return value;};
  auto pidValue=number(a[3]);if(!pidValue||pidValue>MAXDWORD)throw std::runtime_error(tr(TextId::S260));
  parent=reinterpret_cast<HANDLE>(uintptr_t(number(a[5])));accepted=reinterpret_cast<HANDLE>(uintptr_t(number(a[6])));cancelled=reinterpret_cast<HANDLE>(uintptr_t(number(a[7])));
  stage=fs::absolute(a[1]);target=fs::absolute(a[2]);rejectReparse(target);rejectReparse(stage);
  // Validate while the caller is alive. An inherited process handle identifies
  // the same kernel object even after exit; never reopen/recheck by PID later.
  if(!validateUpdateParent(parent,DWORD(pidValue),number(a[4]),target/L"DesktopPet.exe"))throw std::runtime_error(tr(TextId::S261));
  auto info=verifyManifest(readBytes(stage/L"update.json",65536),readBytes(stage/L"update.sig",64));auto bytes=verifyPackage(stage,info);auto files=inspectZip(bytes);
  if(WaitForSingleObject(cancelled,0)!=WAIT_TIMEOUT)return 3;
  if(!SetEvent(accepted))throw std::runtime_error(tr(TextId::S262));
  HANDLE waiting[]={cancelled,parent};DWORD result=WaitForMultipleObjects(2,waiting,FALSE,90000);
  if(result==WAIT_OBJECT_0)return 3;
  if(result!=WAIT_OBJECT_0+1)throw std::runtime_error(result==WAIT_TIMEOUT?tr(TextId::S263):tr(TextId::S264));
  parentExited=true;savedLanguage(target);CloseHandle(parent);parent=nullptr;CloseHandle(accepted);accepted=nullptr;CloseHandle(cancelled);cancelled=nullptr;
  if(fs::exists(target/journalName))rollback(target);
  progressUI.show(tw(TextId::S265));
  auto backup=target/backupName;rejectReparse(backup);if(fs::exists(backup))fs::remove_all(backup);fs::create_directories(backup);
  Json j;j["phase"]="backing";for(auto&name:names){auto f=fs::path(wide(name));rejectReparse(target/f);j["existed"][name]=fs::exists(target/f);}writeText(target/journalName,j.dump());
  for(auto&name:names)if(j["existed"][name].boolean())copyAtomic(target/fs::path(wide(name)),backup/fs::path(wide(name)));
  j["phase"]="prepared";writeText(target/journalName,j.dump());
  progressUI.show(tw(TextId::S266));
  for(auto&f:files){auto path=target/fs::path(wide(f.name));rejectReparse(path);writeInstalled(path,bytes.data()+f.offset,f.size);}installed=true;
  j["phase"]="installed";writeText(target/journalName,j.dump());
  progressUI.show(tw(TextId::S267));
  auto eventName=L"Local\\DesktopPetReady-"+wide(randomId());HANDLE ready=CreateEventW(nullptr,TRUE,FALSE,eventName.c_str());if(!ready)throw std::runtime_error(tr(TextId::S268));
  auto pi=launch(target/L"DesktopPet.exe",{L"--updated",eventName});child=pi.hProcess;CloseHandle(pi.hThread);HANDLE handles[]={ready,child};DWORD resultReady=responsiveWait(2,handles,20000);CloseHandle(ready);
  // A ready process must survive a further health window before committing.
  bool good=resultReady==WAIT_OBJECT_0&&WaitForSingleObject(child,3000)==WAIT_TIMEOUT;
  if(!good){TerminateProcess(child,1);WaitForSingleObject(child,5000);CloseHandle(child);child=nullptr;throw std::runtime_error(tr(TextId::S269));}
  progressUI.show(tw(TextId::S270));
  commitInstalledTrust(info.manifest,info.signature,info.version);
  CloseHandle(child);child=nullptr;fs::remove(target/journalName);std::error_code ec;fs::remove_all(backup,ec);fs::remove_all(stage,ec);return 0;
 }catch(const std::exception&e){
  if(child){TerminateProcess(child,1);WaitForSingleObject(child,5000);CloseHandle(child);}
  if(parent)CloseHandle(parent);if(accepted)CloseHandle(accepted);if(cancelled)CloseHandle(cancelled);
  std::string error=e.what();try{if((parentExited||recovering)&&!target.empty()&&fs::exists(target/journalName))rollback(target);}catch(const std::exception&r){error+=tr(TextId::S271);error+=r.what();}
  if(!parentExited&&!stage.empty()){try{writeText(stage/L"handoff-error.txt",error);}catch(...){}return 1;}
  if(progressUI.window)ShowWindow(progressUI.window,SW_HIDE);auto message=std::string(error);localizedMessage(nullptr,GetModuleHandleW(nullptr),TextId::S273,[message]{return std::wstring(tw(TextId::S272))+wide(currentSystemText(message));},false);
  try{if(parentExited&&!target.empty()&&!fs::exists(target/journalName)&&fs::exists(target/L"DesktopPet.exe"))closeProcess(launch(target/L"DesktopPet.exe",{L"--updated"}));}catch(...){}
  (void)installed;return 1;
 }
}
