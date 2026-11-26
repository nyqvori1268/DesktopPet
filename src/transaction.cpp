#include "transaction.h"
#include "trust_store.h"
#include <wincrypt.h>
#include <cwctype>
#include <map>
#include <memory>
namespace pet {
UpdatePathLock::UpdatePathLock(const fs::path&directory){
 try{auto full=fs::absolute(directory).lexically_normal();if(full.root_name().wstring().size()!=2)transactionError();std::vector<fs::path>paths;for(auto p=full;!p.empty();p=p.parent_path()){paths.push_back(p);if(p==p.root_path())break;}std::reverse(paths.begin(),paths.end());
  for(auto&p:paths){HANDLE h=CreateFileW(p.c_str(),FILE_READ_ATTRIBUTES,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(h==INVALID_HANDLE_VALUE)transactionError();handles.push_back(h);BY_HANDLE_FILE_INFORMATION info{};if(!GetFileInformationByHandle(h,&info)||!(info.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)||(info.dwFileAttributes&FILE_ATTRIBUTE_REPARSE_POINT))transactionError();}
 }catch(...){for(auto h:handles)CloseHandle(h);throw;}
}
UpdatePathLock::~UpdatePathLock(){for(auto h:handles)CloseHandle(h);}
std::vector<unsigned char> readUpdateFile(const fs::path&path,size_t limit){
 UpdatePathLock lock(path.parent_path());HANDLE file=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_FLAG_OPEN_REPARSE_POINT,nullptr);if(file==INVALID_HANDLE_VALUE)transactionError();
 try{BY_HANDLE_FILE_INFORMATION info{};LARGE_INTEGER size{};if(!GetFileInformationByHandle(file,&info)||GetFileType(file)!=FILE_TYPE_DISK||(info.dwFileAttributes&(FILE_ATTRIBUTE_REPARSE_POINT|FILE_ATTRIBUTE_DIRECTORY))||info.nNumberOfLinks!=1||!GetFileSizeEx(file,&size)||size.QuadPart<0||uint64_t(size.QuadPart)>limit)transactionError();std::vector<unsigned char>data(size_t(size.QuadPart));DWORD got=0;if(!data.empty()&&(!ReadFile(file,data.data(),DWORD(data.size()),&got,nullptr)||got!=data.size()))transactionError();CloseHandle(file);return data;}catch(...){CloseHandle(file);throw;}
}
std::string updateTarget(const fs::path&path){auto full=fs::absolute(path).lexically_normal().wstring();for(auto&c:full)c=wchar_t(std::towlower(c));return utf8(full);}
static std::vector<unsigned char> protectedJournal(const std::vector<unsigned char>&data,bool seal){DATA_BLOB in{DWORD(data.size()),const_cast<BYTE*>(data.data())},out{};BOOL ok=seal?CryptProtectData(&in,L"DesktopPet transaction v2",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out):CryptUnprotectData(&in,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&out);if(!ok)transactionError();std::vector<unsigned char>result;try{result.assign(out.pbData,out.pbData+out.cbData);}catch(...){LocalFree(out.pbData);throw;}LocalFree(out.pbData);return result;}
void writeUpdateFile(const fs::path&path,const std::vector<unsigned char>&data){UpdatePathLock lock(path.parent_path());rejectReparse(path);if(fs::exists(path))readUpdateFile(path);atomicWrite(path,data.data(),data.size());if(readUpdateFile(path)!=data)transactionError();}
void saveTransaction(const fs::path&target,const Json&j){validateTransaction(j);if(j["target"].str()!=updateTarget(target))transactionError();auto text=j.dump();auto data=protectedJournal(std::vector<unsigned char>(text.begin(),text.end()),true);data.insert(data.begin(),{'D','P','J','2'});writeUpdateFile(target/transactionJournal,data);}
Json loadTransaction(const fs::path&target){auto bytes=readUpdateFile(target/transactionJournal,256*1024);if(bytes.size()<4||std::string(bytes.begin(),bytes.begin()+4)!="DPJ2")transactionError();auto plain=protectedJournal(std::vector<unsigned char>(bytes.begin()+4,bytes.end()),false);auto j=Json::parse(std::string(plain.begin(),plain.end()));validateTransaction(j);auto manifest=j["manifest"].str();if(j["target"].str()!=updateTarget(target)||j["receipt"].str()!=sha256(std::vector<unsigned char>(manifest.begin(),manifest.end())))transactionError();return j;}
struct TransactionIO{
 fs::path target,backup;std::map<std::string,std::vector<unsigned char>> verifiedBackups;
 explicit TransactionIO(fs::path root):target(std::move(root)),backup(target/transactionBackup){}
 TrustCommit trust(const Json&j){auto manifest=j["manifest"].str();auto message=std::vector<unsigned char>(manifest.begin(),manifest.end());auto sig=hexBytes(j["signature"].str());auto metadata=Json::parse(manifest);if(metadata["version"].str()!=j["version"].str()||metadata["schema"].num()!=1||metadata["arch"].str()!="windows-x64"||metadata["asset"].str()!="DesktopPet-windows-x64.zip"||!verifyTrustedManifest(message,sig))transactionError();return installedTrustCommit(j["oldVersion"].str(),j["version"].str(),j["receipt"].str());}
 std::string digest(const fs::path&p){rejectReparse(p);if(!fs::exists(p))return {};return sha256(readUpdateFile(p));}
 void verifyCurrent(const Json&j,bool next){for(auto name:transactionFiles){auto expected=j["files"][name][next?"after":"before"].str();if(digest(target/wide(name))!=expected)transactionError();}}
 void verifyBackups(const Json&j){for(auto name:transactionFiles){auto expected=j["files"][name]["before"].str();if(expected.empty()){if(fs::exists(backup/wide(name)))transactionError();continue;}auto bytes=readUpdateFile(backup/wide(name));if(sha256(bytes)!=expected)transactionError();verifiedBackups[name]=std::move(bytes);}}
 void verifyReplaceable(const Json&j){for(auto name:transactionFiles){auto hash=digest(target/wide(name));if(!hash.empty()&&hash!=j["files"][name]["before"].str()&&hash!=j["files"][name]["after"].str())transactionError();}}
 void restore(const Json&j,const char*name){auto path=target/wide(name);if(j["files"][name]["existed"].boolean())writeUpdateFile(path,verifiedBackups.at(name));else if(fs::exists(path)){UpdatePathLock lock(path.parent_path());rejectReparse(path);fs::remove(path);}}
 void save(const Json&j){saveTransaction(target,j);}
 void cleanup(const Json&){
  // Never recursively remove an unknown tree. Validate the entire cleanup set first.
  std::vector<fs::path>files;rejectReparse(backup);
  if(fs::exists(backup)){UpdatePathLock lock(backup);std::unique_ptr<UpdatePathLock>dataGuard;if(fs::exists(backup/L"data"))dataGuard=std::make_unique<UpdatePathLock>(backup/L"data");for(auto&entry:fs::recursive_directory_iterator(backup)){rejectReparse(entry.path());auto relative=entry.path().lexically_relative(backup).generic_string();if(entry.is_directory()){if(relative!="data")transactionError();continue;}bool known=packagePath(relative);auto suffix=relative.find(".tmp-");if(suffix!=std::string::npos){auto id=relative.substr(suffix+5);known=packagePath(relative.substr(0,suffix))&&id.size()==32&&id.find_first_not_of("0123456789abcdef")==std::string::npos;}if(!known)transactionError();readUpdateFile(entry.path());files.push_back(entry.path());}for(auto&file:files)fs::remove(file);dataGuard.reset();if(fs::exists(backup/L"data"))fs::remove(backup/L"data");}
  if(fs::exists(backup))fs::remove(backup);
  // Journal is deleted LAST. A cleanup interruption is resumable from its terminal phase.
  fs::remove(target/transactionJournal);
 }
};
void prepareUpdate(const fs::path&target,Json&j){UpdatePathLock lock(target);validateTransaction(j);if(fs::exists(target/transactionJournal)||fs::exists(target/transactionBackup))transactionError();TransactionIO io(target);io.verifyCurrent(j,false);saveTransaction(target,j);fs::create_directory(io.backup);fs::create_directory(io.backup/L"data");if(!fs::exists(target/L"data"))fs::create_directory(target/L"data");for(auto name:transactionFiles)if(j["files"][name]["existed"].boolean())writeUpdateFile(io.backup/wide(name),readUpdateFile(target/wide(name)));io.verifyBackups(j);j["phase"]="prepared";saveTransaction(target,j);}
void verifyInstalledUpdate(const fs::path&target,const Json&j){TransactionIO(target).verifyCurrent(j,true);}
void finishUpdate(const fs::path&target,Json&j){UpdatePathLock lock(target);TransactionIO io(target);recoverTransaction(j,io);}
void recoverUpdate(const fs::path&target){UpdatePathLock lock(target);auto j=loadTransaction(target);TransactionIO io(target);recoverTransaction(j,io);}
void cleanupUpdateWork(){
 for(auto group:{L"updates",L"updaters"}){auto root=appData()/group;if(!fs::exists(root))continue;try{UpdatePathLock rootLock(root);for(auto&entry:fs::directory_iterator(root)){auto id=entry.path().filename().wstring();if(id.size()!=32||id.find_first_not_of(L"0123456789abcdef")!=std::wstring::npos||fs::file_time_type::clock::now()-entry.last_write_time()<std::chrono::hours(24*7))continue;
  try{std::vector<fs::path>files;{UpdatePathLock childLock(entry.path());for(auto&file:fs::directory_iterator(entry.path())){auto name=file.path().filename().wstring();if(name!=L"package.zip"&&name!=L"update.json"&&name!=L"update.sig"&&name!=L"Updater.exe"&&name!=L"handoff-error.txt")transactionError();readUpdateFile(file.path());files.push_back(file.path());}for(auto&file:files)fs::remove(file);}fs::remove(entry.path());}catch(...){}}
 }catch(...){}}
}

}
