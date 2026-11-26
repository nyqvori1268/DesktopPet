#pragma once
#include "core.h"
namespace pet {
inline constexpr std::array<const char*,5> transactionFiles{"DesktopPet.exe","Updater.exe","data/dialogue.json","README.md","CHANGELOG.md"};
inline bool digestText(const std::string&s){return s.size()==64&&s.find_first_not_of("0123456789abcdef")==std::string::npos;}
inline void transactionError(){throw std::runtime_error(tr(TextId::TransactionError));}
inline void validateTransaction(const Json&j){
 auto phase=j["phase"].str();auto old=Version::parse(j["oldVersion"].str()),next=Version::parse(j["version"].str());
 if(j["schema"].num()!=2||!old||!next||!(*old<*next)||j["target"].str().empty()||!digestText(j["receipt"].str())||j["manifest"].str().empty()||j["manifest"].str().size()>65536||(j["signature"].str().size()!=128||j["signature"].str().find_first_not_of("0123456789abcdef")!=std::string::npos))transactionError();
 if(phase!="backing"&&phase!="prepared"&&phase!="installed"&&phase!="committing"&&phase!="committed"&&phase!="rolledback")transactionError();
 if(!j["files"].object()||std::get<Json::Object>(j["files"].v).size()!=5)transactionError();
 for(auto name:transactionFiles){auto&file=j["files"][name];if(!std::holds_alternative<bool>(file["existed"].v)||!digestText(file["after"].str())||(file["existed"].boolean()?!digestText(file["before"].str()):!file["before"].str().empty()))transactionError();}
 if(!j["files"]["DesktopPet.exe"]["existed"].boolean()||!j["files"]["Updater.exe"]["existed"].boolean())transactionError();
}
enum class TrustCommit{Before,Exact,Conflict};
inline TrustCommit classifyTrustCommit(const std::string&highest,const std::string&storedReceipt,const std::string&oldVersion,const std::string&version,const std::string&receipt){auto high=Version::parse(highest),old=Version::parse(oldVersion),next=Version::parse(version);if(!high||!old||!next||!digestText(receipt))transactionError();if(highest==version&&storedReceipt==receipt)return TrustCommit::Exact;return !(*old<*high)?TrustCommit::Before:TrustCommit::Conflict;}

// This is the production recovery order. Tests inject power loss at each I/O.
// Backend must hold target-directory handles and authenticate the journal first.
template<class IO> void recoverTransaction(Json&j,IO&io){
 validateTransaction(j);auto state=io.trust(j);auto phase=j["phase"].str();
 if(state==TrustCommit::Conflict)transactionError();
 if(state==TrustCommit::Exact){io.verifyCurrent(j,true);if(phase!="committed"){j["phase"]="committed";io.save(j);}io.cleanup(j);return;}
 if(phase=="committed"||phase=="committing"){
  // A failed commit is rollback-safe only when durable trust is still old.
  if(phase=="committed")transactionError();
 }
 if(phase=="rolledback"||phase=="backing"){io.verifyCurrent(j,false);io.cleanup(j);return;}
 io.verifyBackups(j);io.verifyReplaceable(j); // Preflight ALL files before any mutation.
 for(auto name:transactionFiles)io.restore(j,name);
 io.verifyCurrent(j,false);j["phase"]="rolledback";io.save(j);io.cleanup(j);
}
}
