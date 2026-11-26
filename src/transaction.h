#pragma once
#include "platform.h"
#include "transaction_logic.h"
namespace pet {
inline constexpr const wchar_t* transactionJournal=L".desktoppet-transaction.json";
inline constexpr const wchar_t* transactionBackup=L".desktoppet-backup";
// Directory handles deny rename/delete of every ancestor during the operation.
class UpdatePathLock{
 std::vector<HANDLE>handles;
public:
 explicit UpdatePathLock(const fs::path&directory);
 ~UpdatePathLock();
 UpdatePathLock(const UpdatePathLock&)=delete;UpdatePathLock&operator=(const UpdatePathLock&)=delete;
};
std::vector<unsigned char> readUpdateFile(const fs::path&path,size_t limit=16*1024*1024);
std::string updateTarget(const fs::path&path);
void saveTransaction(const fs::path&target,const Json&journal);
Json loadTransaction(const fs::path&target);
void cleanupUpdateWork();
void recoverUpdate(const fs::path&target);
void prepareUpdate(const fs::path&target,Json&journal);
void verifyInstalledUpdate(const fs::path&target,const Json&journal);
void finishUpdate(const fs::path&target,Json&journal);
void writeUpdateFile(const fs::path&path,const std::vector<unsigned char>&data);
}
