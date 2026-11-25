#pragma once
#include "platform.h"
namespace pet {
uint64_t processCreation(HANDLE process);
bool sameFileIdentity(const fs::path&a,const fs::path&b);
bool validateUpdateParent(HANDLE process,DWORD pid,uint64_t created,const fs::path&expected);
PROCESS_INFORMATION launchInherited(const fs::path&exe,const std::vector<std::wstring>&args,const std::vector<HANDLE>&handles);
class UpdateHandoff{
 HANDLE ready=nullptr,abort=nullptr;PROCESS_INFORMATION child{};ULONGLONG began=0;bool committed=false;
 void close();
public:
 ~UpdateHandoff(){if(!committed)cancel();close();}
 bool pending()const{return child.hProcess!=nullptr;}
 void start(const fs::path&updater,const fs::path&stage,const fs::path&target);
 // 0 pending, 1 validated/ready, -1 exited/failed, -2 prepare timeout.
 int poll()const;
 void cancel(){if(abort)SetEvent(abort);}
 void reset(){cancel();close();committed=false;}
 void commit(){committed=true;}
};
}
