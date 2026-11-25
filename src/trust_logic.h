#pragma once
#include "core.h"
#include <functional>
namespace pet {
inline std::string publicPoint(std::string s){if(s.size()!=128)throw std::runtime_error("可信金鑰需為 P256 X||Y");for(char&c:s){if(c>='A'&&c<='F')c=char(c-'A'+'a');if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')))throw std::runtime_error("Invalid public key");}return s;}
inline std::vector<unsigned char> hexBytes(const std::string&s){if(s.size()%2)throw std::runtime_error("Invalid signature hex");auto digit=[](char c)->unsigned{if(c>='0'&&c<='9')return unsigned(c-'0');if(c>='a'&&c<='f')return unsigned(c-'a'+10);if(c>='A'&&c<='F')return unsigned(c-'A'+10);throw std::runtime_error("Invalid signature hex");};std::vector<unsigned char>b;for(size_t i=0;i<s.size();i+=2)b.push_back(static_cast<unsigned char>(digit(s[i])*16+digit(s[i+1])));return b;}
struct TrustState{
 std::string primary,backup,minimum="0.0.0",highest="0.0.0";unsigned epoch=0;std::vector<std::string> revoked;
 bool accepts(const std::string&key)const{return !key.empty()&&(key==primary||key==backup)&&std::find(revoked.begin(),revoked.end(),key)==revoked.end();}
 bool permits(const std::string&version)const{auto v=Version::parse(version),floor=Version::parse(minimum),high=Version::parse(highest);return v&&floor&&high&&!(*v<*floor)&&!(*v<*high);}
};
using PolicyVerifier=std::function<bool(const std::string&,const std::vector<unsigned char>&,const std::string&)>;
inline TrustState applyPolicy(const TrustState&old,const Json&envelope,const PolicyVerifier&verify){
 auto payload=envelope["payload"].str(),encoded=envelope["signature"].str();if(payload.empty()||payload.size()>16384||encoded.size()!=128)throw std::runtime_error("Invalid trust envelope");
 // Once provisioned, only the offline backup authority may change trust.
 const auto&authority=old.backup.empty()?old.primary:old.backup;
 if(authority.empty()||!verify(payload,hexBytes(encoded),authority))throw std::runtime_error("新金鑰未獲既有可信授權金鑰簽署");
 auto j=Json::parse(payload);double epoch=j["epoch"].num();if(j["schema"].num()!=1||epoch!=old.epoch+1||epoch>32)throw std::runtime_error("信任政策重播、跳號或超出輪替上限");
 TrustState next=old;next.epoch=unsigned(epoch);next.primary=publicPoint(j["primary"].str());next.backup=publicPoint(j["backup"].str());if(next.primary==next.backup)throw std::runtime_error("主要與備援金鑰必須不同");
 next.minimum=j["minimumVersion"].str();auto floor=Version::parse(next.minimum),previous=Version::parse(old.minimum);if(!floor||!previous||*floor<*previous)throw std::runtime_error("信任政策不得降低版本下限");
 auto revoke=[&](std::string key){key=publicPoint(std::move(key));if(std::find(next.revoked.begin(),next.revoked.end(),key)==next.revoked.end())next.revoked.push_back(key);};
 if(!j["revoked"].array()&&!std::holds_alternative<std::nullptr_t>(j["revoked"].v))throw std::runtime_error("Invalid revoked list");
 if(j["revoked"].items().size()>64)throw std::runtime_error("Revocation limit");
 for(auto&key:j["revoked"].items())revoke(key.str());
 for(auto&key:{old.primary,old.backup})if(!key.empty()&&key!=next.primary&&key!=next.backup)revoke(key);
 if(!next.accepts(next.primary)||!next.accepts(next.backup))throw std::runtime_error("已撤銷金鑰不可復活或保留為有效金鑰");
 return next;
}
inline TrustState evaluatePolicies(TrustState roots,const Json&chain,const PolicyVerifier&verify){if(!chain.array()||chain.items().size()>32)throw std::runtime_error("Invalid trust chain");for(auto&entry:chain.items())roots=applyPolicy(roots,entry,verify);return roots;}
inline bool chainExtends(const Json&old,const Json&next){if(!old.array()||!next.array()||old.items().size()>next.items().size())return false;for(size_t i=0;i<old.items().size();++i)if(old.items()[i].dump()!=next.items()[i].dump())return false;return true;}
}
