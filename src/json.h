#pragma once
#include "i18n.h"
#include <map>
#include <vector>
#include <string>
#include <variant>
#include <stdexcept>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <cstdlib>
namespace pet {
// Small strict JSON codec. Bounded input/depth, duplicate keys rejected.
struct Json {
 using Object=std::map<std::string,Json>; using Array=std::vector<Json>;
 std::variant<std::nullptr_t,bool,double,std::string,Object,Array> v=nullptr;
 Json()=default; Json(std::nullptr_t){} Json(bool b):v(b){} Json(int n):v(double(n)){} Json(long long n):v(double(n)){} Json(double n):v(n){} Json(const char*s):v(std::string(s)){} Json(std::string s):v(std::move(s)){} Json(Object o):v(std::move(o)){} Json(Array a):v(std::move(a)){}
 bool object()const{return std::holds_alternative<Object>(v);} bool array()const{return std::holds_alternative<Array>(v);}
 const Json& operator[](const std::string& k)const{static Json nil; if(!object())return nil; auto&i=std::get<Object>(v);auto p=i.find(k);return p==i.end()?nil:p->second;}
 Json& operator[](const std::string& k){if(!object())v=Object{};return std::get<Object>(v)[k];}
 std::string str(std::string d={})const{auto p=std::get_if<std::string>(&v);return p?*p:d;}
 double num(double d=0)const{auto p=std::get_if<double>(&v);return p&&std::isfinite(*p)?*p:d;}
 bool boolean(bool d=false)const{auto p=std::get_if<bool>(&v);return p?*p:d;}
 const Array& items()const{static Array empty;auto p=std::get_if<Array>(&v);return p?*p:empty;}
 static void utf8(std::string& s,unsigned n){if(n<0x80)s+=char(n);else if(n<0x800){s+=char(0xc0|(n>>6));s+=char(0x80|(n&63));}else if(n<0x10000){s+=char(0xe0|(n>>12));s+=char(0x80|((n>>6)&63));s+=char(0x80|(n&63));}else{s+=char(0xf0|(n>>18));s+=char(0x80|((n>>12)&63));s+=char(0x80|((n>>6)&63));s+=char(0x80|(n&63));}}
 static Json parse(const std::string&s){
  if(s.size()>1024*1024)throw std::runtime_error(tr(TextId::S033));
  // Validate raw UTF-8 as well as escaped surrogate pairs.
  for(size_t i=0;i<s.size();){unsigned char c=s[i++];if(c<128)continue;unsigned n=0,cp=0,min=0;if(c>=0xc2&&c<=0xdf){n=1;cp=c&31;min=0x80;}else if(c>=0xe0&&c<=0xef){n=2;cp=c&15;min=0x800;}else if(c>=0xf0&&c<=0xf4){n=3;cp=c&7;min=0x10000;}else throw std::runtime_error(tr(TextId::S034));if(n>s.size()-i)throw std::runtime_error(tr(TextId::S034));while(n--){unsigned char d=s[i++];if((d&0xc0)!=0x80)throw std::runtime_error(tr(TextId::S034));cp=(cp<<6)|(d&63);}if(cp<min||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))throw std::runtime_error(tr(TextId::S034));}

  struct Parser{const std::string&s;size_t p=0;void ws(){while(p<s.size()&&(s[p]==' '||s[p]=='\r'||s[p]=='\n'||s[p]=='\t'))++p;} [[noreturn]] void fail(){throw std::runtime_error(tr(TextId::S035));}
   char take(){if(p>=s.size())fail();return s[p++];} unsigned hex(){unsigned n=0;for(int i=0;i<4;++i){char c=take();n<<=4;if(c>='0'&&c<='9')n+=c-'0';else if(c>='a'&&c<='f')n+=c-'a'+10;else if(c>='A'&&c<='F')n+=c-'A'+10;else fail();}return n;}
   std::string string(){if(take()!='"')fail();std::string r;for(;;){unsigned char c=take();if(c=='"')break;if(c<32)fail();if(c!='\\'){r+=char(c);continue;}switch(take()){case '"':r+='"';break;case '\\':r+='\\';break;case '/':r+='/';break;case 'b':r+='\b';break;case 'f':r+='\f';break;case 'n':r+='\n';break;case 'r':r+='\r';break;case 't':r+='\t';break;case 'u':{unsigned n=hex();if(n>=0xd800&&n<=0xdbff){if(take()!='\\'||take()!='u')fail();unsigned l=hex();if(l<0xdc00||l>0xdfff)fail();n=0x10000+((n-0xd800)<<10)+(l-0xdc00);}else if(n>=0xdc00&&n<=0xdfff)fail();Json::utf8(r,n);break;}default:fail();}}return r;}
   Json value(int depth){if(depth>32)fail();ws();if(p>=s.size())fail();char c=s[p];if(c=='"')return string();if(c=='{'){++p;Object o;ws();if(p<s.size()&&s[p]=='}'){++p;return o;}for(;;){ws();auto k=string();ws();if(take()!=':')fail();auto x=value(depth+1);if(!o.emplace(k,x).second)fail();ws();c=take();if(c=='}')return o;if(c!=',')fail();}}
    if(c=='['){++p;Array a;ws();if(p<s.size()&&s[p]==']'){++p;return a;}for(;;){a.push_back(value(depth+1));ws();c=take();if(c==']')return a;if(c!=',')fail();}}
    for(auto lit:{"true","false","null"}){std::string l=lit;if(s.compare(p,l.size(),l)==0){p+=l.size();if(l=="true")return true;if(l=="false")return false;return nullptr;}}
    size_t start=p;if(s[p]=='-')++p;if(p>=s.size())fail();if(s[p]=='0')++p;else{if(s[p]<'1'||s[p]>'9')fail();while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;}if(p<s.size()&&s[p]=='.'){++p;size_t b=p;while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;if(p==b)fail();}if(p<s.size()&&(s[p]=='e'||s[p]=='E')){++p;if(p<s.size()&&(s[p]=='+'||s[p]=='-'))++p;size_t b=p;while(p<s.size()&&s[p]>='0'&&s[p]<='9')++p;if(p==b)fail();}double n=std::strtod(s.substr(start,p-start).c_str(),nullptr);if(!std::isfinite(n))fail();return n;
   }
  }p{s};Json result=p.value(0);p.ws();if(p.p!=s.size())p.fail();return result;
 }
 std::string dump()const{
  if(std::holds_alternative<std::nullptr_t>(v))return "null";
  if(auto p=std::get_if<bool>(&v))return *p?"true":"false";
  if(auto p=std::get_if<double>(&v)){if(!std::isfinite(*p))throw std::runtime_error(tr(TextId::S036));std::ostringstream o;o<<std::setprecision(17)<<*p;return o.str();}
  if(auto p=std::get_if<std::string>(&v)){std::string r="\"";const char*h="0123456789abcdef";for(unsigned char c:*p){if(c=='"'||c=='\\'){r+='\\';r+=char(c);}else if(c<32){r+="\\u00";r+=h[c>>4];r+=h[c&15];}else r+=char(c);}return r+'"';}
  std::string r;bool first=true;if(auto p=std::get_if<Object>(&v)){r="{";for(auto&x:*p){if(!first)r+=',';first=false;r+=Json(x.first).dump()+":"+x.second.dump();}return r+'}';}r="[";for(auto&x:std::get<Array>(v)){if(!first)r+=',';first=false;r+=x.dump();}return r+']';
 }
};
}
