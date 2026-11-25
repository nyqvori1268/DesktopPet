#include "trust_store.h"
#include "build_config.h"
#include <wincrypt.h>
namespace pet {
namespace {
struct Guard{
 HANDLE h=INVALID_HANDLE_VALUE;
 Guard(){auto path=appData()/L"update-trust.lock";rejectReparse(path);for(int attempt=0;attempt<50;++attempt){h=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);if(h!=INVALID_HANDLE_VALUE)return;if(GetLastError()!=ERROR_SHARING_VIOLATION)throw std::runtime_error("無法鎖定更新信任資料");Sleep(200);}throw std::runtime_error("Trust state busy");}
 ~Guard(){if(h!=INVALID_HANDLE_VALUE)CloseHandle(h);}
};
void validPoints(const TrustState&s){if(!publicKeyValid(s.primary)||(!s.backup.empty()&&!publicKeyValid(s.backup)))throw std::runtime_error("P256 公開金鑰不是有效曲線點");}
TrustState roots(){TrustState s;if(std::string(PET_PUBLIC_KEY).empty())throw std::runtime_error("尚未設定可信公開金鑰");s.primary=publicPoint(PET_PUBLIC_KEY);if(std::string(PET_BACKUP_PUBLIC_KEY).size())s.backup=publicPoint(PET_BACKUP_PUBLIC_KEY);if(s.primary==s.backup)throw std::runtime_error("主要／備援金鑰不得相同");s.highest=PET_VERSION;validPoints(s);return s;}
PolicyVerifier verifier(){return [](const std::string&m,const std::vector<unsigned char>&s,const std::string&key){return verifySignature(std::vector<unsigned char>(m.begin(),m.end()),s,key);};}
fs::path statePath(){return appData()/L"update-trust.bin";}
std::vector<unsigned char> protect(const std::vector<unsigned char>&bytes,bool encode){
 DATA_BLOB input{DWORD(bytes.size()),const_cast<BYTE*>(bytes.data())},output{};BOOL ok=encode?CryptProtectData(&input,L"DesktopPet update trust",nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output):CryptUnprotectData(&input,nullptr,nullptr,nullptr,nullptr,CRYPTPROTECT_UI_FORBIDDEN,&output);
 if(!ok)throw std::runtime_error("更新信任資料損毀或不屬於此 Windows 使用者；已停止線上更新，請還原可信備份。");
 std::vector<unsigned char> result;try{result.assign(output.pbData,output.pbData+output.cbData);}catch(...){LocalFree(output.pbData);throw;}LocalFree(output.pbData);return result;
}
struct Snapshot{Json chain=Json::Array{};TrustState state;};
Snapshot load(){Snapshot s;s.state=roots();auto path=statePath();rejectReparse(path);if(!fs::exists(path))return s;auto bytes=protect(readBytes(path,1024*1024),false);auto j=Json::parse(std::string(bytes.begin(),bytes.end()));if(j["schema"].num()!=1)throw std::runtime_error("Unknown trust state");s.chain=j["chain"];s.state=evaluatePolicies(roots(),s.chain,verifier());auto high=Version::parse(j["highest"].str()),current=Version::parse(PET_VERSION);if(!high||!current)throw std::runtime_error("Invalid trust high-water version");s.state.highest=*high<*current?PET_VERSION:j["highest"].str();return s;}
Snapshot candidate(const Snapshot&old,const Json&manifest){Snapshot next=old;if(manifest["trust"].array()){if(!chainExtends(old.chain,manifest["trust"]))throw std::runtime_error("更新信任鏈不得回退或改寫既有政策");next.chain=manifest["trust"];next.state=evaluatePolicies(roots(),next.chain,verifier());next.state.highest=old.state.highest;}else if(!std::holds_alternative<std::nullptr_t>(manifest["trust"].v))throw std::runtime_error("Invalid trust chain field");validPoints(next.state);if(!next.state.permits(manifest["version"].str()))throw std::runtime_error("更新低於信任政策／已安裝版本下限");return next;}
bool signature(const Snapshot&s,const std::vector<unsigned char>&m,const std::vector<unsigned char>&sig){return (s.state.accepts(s.state.primary)&&verifySignature(m,sig,s.state.primary))||(s.state.accepts(s.state.backup)&&verifySignature(m,sig,s.state.backup));}
}
bool verifyTrustedManifest(const std::vector<unsigned char>&message,const std::vector<unsigned char>&sig){if(message.size()>65536||sig.size()!=64)return false;Guard guard;auto manifest=Json::parse(std::string(message.begin(),message.end()));auto next=candidate(load(),manifest);return signature(next,message,sig);}
void checkTrustVersion(const Json&manifest){Guard guard;candidate(load(),manifest);}
void commitInstalledTrust(const std::vector<unsigned char>&message,const std::vector<unsigned char>&sig,const std::string&installedVersion){
 if(message.size()>65536||sig.size()!=64)throw std::runtime_error("Trust manifest size");Guard guard;auto j=Json::parse(std::string(message.begin(),message.end()));auto next=candidate(load(),j);
 auto minimum=Version::parse(j["minVersion"].str()),version=Version::parse(installedVersion);if(!minimum||!version||*version<*minimum||j["schema"].num()!=1||j["arch"].str()!="windows-x64"||j["asset"].str()!="DesktopPet-windows-x64.zip"||j["version"].str()!=installedVersion||!signature(next,message,sig))throw std::runtime_error("無法提交未經驗證或版本不符的更新信任資料");
 Json state;state["schema"]=1;state["chain"]=next.chain;state["highest"]=installedVersion;auto text=state.dump();auto encrypted=protect(std::vector<unsigned char>(text.begin(),text.end()),true);atomicWrite(statePath(),encrypted.data(),encrypted.size());
}
bool authorizedSigningKey(const Json&chain,const std::string&point,bool emergency){auto s=evaluatePolicies(roots(),chain,verifier());validPoints(s);auto key=publicPoint(point);return emergency?key==s.backup&&!key.empty():key==s.primary;}
}
