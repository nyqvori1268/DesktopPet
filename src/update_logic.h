#pragma once
#include "core.h"
#include <sstream>
namespace pet {
// No process is replaceable until its inherited identity was validated and its
// kernel process handle is signalled. PID alone is never an authority.
enum class ParentDecision{Reject,Wait,Exited,Timeout};
inline ParentDecision parentDecision(bool identity,bool exited,bool timeout){if(!identity)return ParentDecision::Reject;if(exited)return ParentDecision::Exited;return timeout?ParentDecision::Timeout:ParentDecision::Wait;}
inline std::string trimLine(std::string s){while(!s.empty()&&(s.back()=='\r'||s.back()==' '||s.back()=='\t'))s.pop_back();auto p=s.find_first_not_of(" \t");return p==std::string::npos?std::string{}:s.substr(p);}
inline std::optional<Version> noteHeading(std::string line){line=trimLine(line);if(line.rfind("## ",0)!=0)return {};line=trimLine(line.substr(3));if(!line.empty()&&line.front()=='[')line.erase(0,1);size_t n=0;while(n<line.size()&&((line[n]>='0'&&line[n]<='9')||line[n]=='.'||line[n]=='v'))++n;return Version::parse(line.substr(0,n));}
inline std::string versionNotes(const std::string&markdown,const std::string&version){
 auto wanted=Version::parse(version);if(!wanted)return "此版本未提供更新說明。";
 std::istringstream input(markdown);std::string line,out;bool found=false,hasVersions=false;
 while(std::getline(input,line)){auto heading=noteHeading(line);if(heading){hasVersions=true;if(found)break;if(heading->tuple()==wanted->tuple())found=true;continue;}if(found)out+=line+'\n';}
 if(!found&&!hasVersions)out=markdown;
 return trimLine(out).empty()?"此版本未提供更新說明。":out;
}
inline std::string readableNotes(const std::string&markdown,const std::string&version){
 std::istringstream input(versionNotes(markdown,version));std::string line,out;
 while(std::getline(input,line)){
  line=trimLine(line);if(line.rfind("```",0)==0)continue;
  while(!line.empty()&&line[0]=='#')line.erase(0,1);
  line=trimLine(line);
  for(const auto&mark:{std::string("（未發布）"),std::string("(未發布)"),std::string("未發布"),std::string("**"),std::string("__"),std::string("`")}){size_t p;while((p=line.find(mark))!=std::string::npos)line.erase(p,mark.size());}
  size_t p=0;while((p=line.find('[',p))!=std::string::npos){auto end=line.find("](",p),close=end==std::string::npos?std::string::npos:line.find(')',end+2);if(close==std::string::npos)break;auto label=line.substr(p+1,end-p-1),url=line.substr(end+2,close-end-2);line.replace(p,close-p+1,label+"（"+url+"）");p+=label.size()+url.size()+6;}
  if(line.rfind("- ",0)==0||line.rfind("* ",0)==0)line="• "+line.substr(2);
  out+=line+"\r\n";
 }
 return out;
}
}
