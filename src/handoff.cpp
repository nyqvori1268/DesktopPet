#include "handoff.h"
namespace pet {
uint64_t processCreation(HANDLE process){FILETIME created{},exited{},kernel{},user{};if(!GetProcessTimes(process,&created,&exited,&kernel,&user))throw std::runtime_error(tr(TextId::S022));return (uint64_t(created.dwHighDateTime)<<32)|created.dwLowDateTime;}
bool sameFileIdentity(const fs::path&a,const fs::path&b){
 auto open=[](const fs::path&p){return CreateFileW(p.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);};
 HANDLE x=open(a),y=open(b);BY_HANDLE_FILE_INFORMATION left{},right{};
 bool ok=x!=INVALID_HANDLE_VALUE&&y!=INVALID_HANDLE_VALUE&&GetFileInformationByHandle(x,&left)&&GetFileInformationByHandle(y,&right)&&left.dwVolumeSerialNumber==right.dwVolumeSerialNumber&&left.nFileIndexHigh==right.nFileIndexHigh&&left.nFileIndexLow==right.nFileIndexLow;
 if(x!=INVALID_HANDLE_VALUE)CloseHandle(x);if(y!=INVALID_HANDLE_VALUE)CloseHandle(y);return ok;
}
bool validateUpdateParent(HANDLE process,DWORD pid,uint64_t created,const fs::path&expected){
 if(!process||!pid||GetProcessId(process)!=pid||WaitForSingleObject(process,0)!=WAIT_TIMEOUT)return false;
 if(processCreation(process)!=created)return false;
 std::wstring path(32768,0);DWORD length=DWORD(path.size());if(!QueryFullProcessImageNameW(process,0,path.data(),&length))return false;path.resize(length);
 return sameFileIdentity(fs::path(path),expected);
}
PROCESS_INFORMATION launchInherited(const fs::path&exe,const std::vector<std::wstring>&args,const std::vector<HANDLE>&handles){
 SIZE_T bytes=0;InitializeProcThreadAttributeList(nullptr,1,0,&bytes);std::vector<unsigned char> storage(bytes);
 auto list=reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());if(!InitializeProcThreadAttributeList(list,1,0,&bytes))throw std::runtime_error(tr(TextId::S023));
 STARTUPINFOEXW startup{};startup.StartupInfo.cb=sizeof(startup);startup.lpAttributeList=list;PROCESS_INFORMATION pi{};
 std::wstring command=quote(exe.wstring());for(auto&arg:args)command+=L" "+quote(arg);
 BOOL ok=UpdateProcThreadAttribute(list,0,PROC_THREAD_ATTRIBUTE_HANDLE_LIST,const_cast<HANDLE*>(handles.data()),handles.size()*sizeof(HANDLE),nullptr,nullptr)&&CreateProcessW(exe.c_str(),command.data(),nullptr,nullptr,TRUE,EXTENDED_STARTUPINFO_PRESENT,nullptr,exe.parent_path().c_str(),&startup.StartupInfo,&pi);
 DeleteProcThreadAttributeList(list);if(!ok)throw std::runtime_error(tr(TextId::S024));return pi;
}
void UpdateHandoff::close(){if(child.hThread)CloseHandle(child.hThread);if(child.hProcess)CloseHandle(child.hProcess);if(ready)CloseHandle(ready);if(abort)CloseHandle(abort);child={};ready=abort=nullptr;}
void UpdateHandoff::start(const fs::path&updater,const fs::path&stage,const fs::path&target){
 if(pending())throw std::runtime_error(tr(TextId::S025));committed=false;
 SECURITY_ATTRIBUTES attributes{sizeof(attributes),nullptr,TRUE};HANDLE parent=nullptr;
 ready=CreateEventW(&attributes,TRUE,FALSE,nullptr);abort=CreateEventW(&attributes,TRUE,FALSE,nullptr);
 if(!ready||!abort||!DuplicateHandle(GetCurrentProcess(),GetCurrentProcess(),GetCurrentProcess(),&parent,SYNCHRONIZE|PROCESS_QUERY_LIMITED_INFORMATION,TRUE,0)){close();throw std::runtime_error(tr(TextId::S026));}
 try{child=launchInherited(updater,{L"--handoff",stage.wstring(),target.wstring(),std::to_wstring(GetCurrentProcessId()),std::to_wstring(processCreation(parent)),std::to_wstring(reinterpret_cast<uintptr_t>(parent)),std::to_wstring(reinterpret_cast<uintptr_t>(ready)),std::to_wstring(reinterpret_cast<uintptr_t>(abort)),L"--language",wide(languageCode(uiLanguage()))},{parent,ready,abort});CloseHandle(parent);began=GetTickCount64();}catch(...){CloseHandle(parent);close();throw;}
}
int UpdateHandoff::poll()const{if(!pending())return -1;if(WaitForSingleObject(child.hProcess,0)!=WAIT_TIMEOUT)return -1;if(WaitForSingleObject(ready,0)==WAIT_OBJECT_0)return 1;return GetTickCount64()-began>=60000?-2:0;}
}
