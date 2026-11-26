#include "transaction_logic.h"
#include <iostream>
#include <map>
using namespace pet;
static int checks=0;
static void check(bool value){++checks;if(!value)throw std::runtime_error("security regression");}
static Json journal(std::string phase){Json j;j["schema"]=2;j["phase"]=phase;j["oldVersion"]="1.3.0";j["version"]="1.4.0";j["target"]="c:/test";j["receipt"]=std::string(64,'f');j["manifest"]="signed bytes";j["signature"]=std::string(128,'0');for(auto name:transactionFiles){j["files"][name]["existed"]=true;j["files"][name]["before"]=std::string(64,'a');j["files"][name]["after"]=std::string(64,'b');}return j;}
struct PowerCut{};
struct Disk{
 Json persisted;std::map<std::string,std::string>target,backup;TrustCommit receipt=TrustCommit::Before;int step=0,cut=-1,mutations=0;
 void checkpoint(){if(++step==cut)throw PowerCut{};}
 TrustCommit trust(const Json&){return receipt;}
 void save(const Json&j){persisted=j;checkpoint();}
 void verifyCurrent(const Json&j,bool next){for(auto name:transactionFiles){if(target[name]!=j["files"][name][next?"after":"before"].str())transactionError();}checkpoint();}
 void verifyBackups(const Json&j){for(auto name:transactionFiles)if(backup[name]!=j["files"][name]["before"].str())transactionError();checkpoint();}
 void verifyReplaceable(const Json&j){for(auto name:transactionFiles)if(!target[name].empty()&&target[name]!=j["files"][name]["before"].str()&&target[name]!=j["files"][name]["after"].str())transactionError();checkpoint();}
 void restore(const Json&j,const char*name){target[name]=j["files"][name]["before"].str();++mutations;checkpoint();}
 void cleanup(const Json&){for(auto name:transactionFiles){backup.erase(name);checkpoint();}persisted=Json{};checkpoint();}
};
int main(){try{
 // Every replacement prefix models an abrupt process/OS loss after that atomic write.
 for(auto phase:{"backing","prepared","installed","committing","committed","rolledback"})for(int replaced=0;replaced<=5;++replaced)for(bool committed:{false,true}){
  if(std::string(phase)=="backing"&&replaced)continue;
  if((std::string(phase)=="committed")&&!committed)continue;
  if((std::string(phase)=="rolledback")&&(replaced||committed))continue;
  if(committed&&replaced!=5)continue;
  Disk start;start.persisted=journal(phase);start.receipt=committed?TrustCommit::Exact:TrustCommit::Before;
  for(size_t i=0;i<5;++i){auto name=transactionFiles[i];start.target[name]=std::string(64,int(i)<replaced?'b':'a');if(std::string(phase)!="backing"||i<2)start.backup[name]=std::string(64,'a');}
  // Cut recovery after EACH durable operation, including each rollback and cleanup deletion.
  for(int fault=0;fault<24;++fault){Disk disk=start;disk.cut=fault;auto working=disk.persisted;try{recoverTransaction(working,disk);}catch(const PowerCut&){}disk.cut=-1;
   if(!disk.persisted.str().empty()||disk.persisted.object()){auto durable=disk.persisted;recoverTransaction(durable,disk);}
   check(!disk.persisted.object());for(auto name:transactionFiles)check(disk.target[name]==std::string(64,committed?'b':'a'));check(disk.backup.empty());if(committed)check(disk.mutations==0);
  }
 }
 // All preflight checks run before rollback mutation, and conflicts never roll back trust.
 for(int scenario=0;scenario<4;++scenario){Disk disk;disk.persisted=journal("committing");for(auto name:transactionFiles){disk.target[name]=std::string(64,'b');disk.backup[name]=std::string(64,'a');}if(scenario==0)disk.backup["CHANGELOG.md"]="corrupt";if(scenario==1)disk.target["CHANGELOG.md"]="foreign";if(scenario==2)disk.receipt=TrustCommit::Conflict;if(scenario==3)disk.persisted["files"]["README.md"]["existed"]=Json{};bool rejected=false;auto before=disk.target;try{auto j=disk.persisted;recoverTransaction(j,disk);}catch(...){rejected=true;}check(rejected&&before==disk.target&&disk.mutations==0&&disk.persisted.object());}
 for(auto bad:{"", "unknown","../prepared"}){auto j=journal(bad);bool failed=false;try{validateTransaction(j);}catch(...){failed=true;}check(failed);}
 auto j=journal("installed");j["files"]["../config.json"]=j["files"]["README.md"];bool rejected=false;try{validateTransaction(j);}catch(...){rejected=true;}check(rejected);
 check(classifyTrustCommit("1.3.0","","1.3.0","1.4.0",std::string(64,'f'))==TrustCommit::Before);
 check(classifyTrustCommit("1.4.0",std::string(64,'f'),"1.3.0","1.4.0",std::string(64,'f'))==TrustCommit::Exact);
 check(classifyTrustCommit("1.4.0",std::string(64,'a'),"1.3.0","1.4.0",std::string(64,'f'))==TrustCommit::Conflict);
 check(classifyTrustCommit("1.5.0",std::string(64,'f'),"1.3.0","1.4.0",std::string(64,'f'))==TrustCommit::Conflict);
 // Terminal committed cleanup cannot silently roll back if the trust receipt is missing.
 Disk terminal;terminal.persisted=journal("committed");rejected=false;try{auto j=terminal.persisted;recoverTransaction(j,terminal);}catch(...){rejected=true;}check(rejected&&terminal.mutations==0);
 std::cout<<"PASS: "<<checks<<" security checks; durable phase/receipt, interruption at every recovery I/O, idempotent rollback/cleanup, corrupt backup and foreign-file refusal\n";
 return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<" after "<<checks<<"\n";return 1;}}
