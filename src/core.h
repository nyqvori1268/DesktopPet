#pragma once
#include "json.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <cstdint>
#include <optional>
#include <tuple>
#include <limits>
namespace pet {
using Seconds=long long;
inline std::string clipUtf8(std::string s,size_t n){if(s.size()<=n)return s;while(n>0&&(static_cast<unsigned char>(s[n])&0xc0)==0x80)--n;s.resize(n);return s;}
inline Seconds now(){return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();}
inline std::tm local(Seconds t){std::time_t x=static_cast<std::time_t>(t);std::tm tm{};
#ifdef _WIN32
 localtime_s(&tm,&x);
#else
 localtime_r(&x,&tm);
#endif
 return tm;}
inline int bounded(const Json&j,int d,int lo,int hi){double n=j.num(d);return int(std::clamp(n,double(lo),double(hi)));}
struct Version{
 int a=0,b=0,c=0;
 static std::optional<Version> parse(std::string s){if(!s.empty()&&s[0]=='v')s.erase(0,1);Version v;int*o[]={&v.a,&v.b,&v.c};size_t p=0;for(int i=0;i<3;++i){size_t start=p;long long n=0;while(p<s.size()&&s[p]>='0'&&s[p]<='9'){n=n*10+s[p++]-'0';if(n>1000000)return {};}if(p==start||(p-start>1&&s[start]=='0'))return {};*o[i]=int(n);if(i<2){if(p>=s.size()||s[p++]!='.')return {};}}if(p!=s.size())return {};return v;}
 auto tuple()const{return std::make_tuple(a,b,c);}bool operator<(const Version&o)const{return tuple()<o.tuple();}
};
enum class Mode{Countdown,Interval,Alarm};
struct Timer{
 Mode mode=Mode::Countdown; bool running=false,paused=false,pending=false;
 int interval=1800,alarmHour=9,alarmMinute=0,weekdays=127,repeat=0,snoozeMinutes=5;
 Seconds due=0,snooze=0,remaining=0;std::string title="時間到了",text="休息一下，陪陪我吧！";
 Seconds nextAlarm(Seconds t)const{auto tm=local(t);for(int d=0;d<9;++d){auto x=tm;x.tm_mday+=d;x.tm_hour=alarmHour;x.tm_min=alarmMinute;x.tm_sec=0;x.tm_isdst=-1;auto ts=static_cast<Seconds>(std::mktime(&x));if(ts<=t)continue;auto actual=local(ts);if(repeat!=2||(weekdays&(1<<actual.tm_wday)))return ts;}return 0;}
 void start(Seconds t){running=true;paused=false;pending=false;snooze=0;remaining=0;due=mode==Mode::Alarm?nextAlarm(t):t+interval;}
 void stop(){running=paused=pending=false;due=snooze=remaining=0;}
 void pause(Seconds t){if(running&&!paused){remaining=std::max<Seconds>(0,due-t);if(snooze)snooze=std::max<Seconds>(1,snooze-t);paused=true;}}
 void resume(Seconds t){if(running&&paused){due=t+remaining;if(snooze)snooze+=t;paused=false;}}
 // One event per tick, even after long sleep. Interval retains its original phase.
 bool tick(Seconds t){if(!running||paused)return false;bool fired=false;
  if(snooze&&t>=snooze){snooze=0;fired=true;}
  if(due&&t>=due){fired=true;if(mode==Mode::Interval){due+=((t-due)/interval+1)*interval;}else if(mode==Mode::Alarm&&repeat){due=nextAlarm(t);}else due=0;}
  if(fired) {pending=true;if(!due&&!snooze)running=false;}
  return fired;
 }
 void acknowledge(){pending=false;}
 void defer(Seconds t,int minutes){pending=false;snooze=t+std::clamp(minutes,1,1440)*60;running=true;paused=false;}
 Json json()const{Json j; j["mode"]=int(mode);j["running"]=running;j["paused"]=paused;j["pending"]=pending;j["interval"]=interval;j["hour"]=alarmHour;j["minute"]=alarmMinute;j["weekdays"]=weekdays;j["repeat"]=repeat;j["snoozeMinutes"]=snoozeMinutes;j["due"]=due;j["snooze"]=snooze;j["remaining"]=remaining;j["title"]=title;j["text"]=text;return j;}
 void load(const Json&j,bool restore){mode=Mode(bounded(j["mode"],0,0,2));interval=bounded(j["interval"],1800,1,604800);alarmHour=bounded(j["hour"],9,0,23);alarmMinute=bounded(j["minute"],0,0,59);weekdays=bounded(j["weekdays"],127,1,127);repeat=bounded(j["repeat"],0,0,2);snoozeMinutes=bounded(j["snoozeMinutes"],5,1,1440);title=clipUtf8(j["title"].str("時間到了"),360);text=clipUtf8(j["text"].str("休息一下，陪陪我吧！"),2000);if(restore){running=j["running"].boolean();paused=j["paused"].boolean();pending=j["pending"].boolean();due=Seconds(std::clamp(j["due"].num(),0.0,4102444800.0));snooze=Seconds(std::clamp(j["snooze"].num(),0.0,4102444800.0));remaining=Seconds(std::clamp(j["remaining"].num(),0.0,604800.0));}}
};
struct Config{
 int schema=1,pet=0,coat=0,x=0,y=0,scale=100,opacity=100;bool positioned=false,top=true,pass=false,autoHide=true,startup=false,autoUpdate=true;std::string name="小橘",monitor,skip;Seconds lastCheck=0;Timer timer;
 void load(const Json&j,bool restore){if(j["schema"].num(1)>schema)return;pet=bounded(j["pet"],0,0,1);coat=bounded(j["coat"],0,0,4);scale=bounded(j["scale"],100,50,300);opacity=bounded(j["opacity"],100,20,100);x=bounded(j["x"],0,-100000,100000);y=bounded(j["y"],0,-100000,100000);positioned=j["positioned"].boolean();top=j["top"].boolean(true);pass=j["pass"].boolean();autoHide=j["autoHide"].boolean(true);startup=j["startup"].boolean();autoUpdate=j["autoUpdate"].boolean(true);name=clipUtf8(j["name"].str("小橘"),120);monitor=j["monitor"].str();skip=j["skip"].str();lastCheck=Seconds(std::clamp(j["lastCheck"].num(),0.0,4102444800.0));timer.load(j["timer"],restore);}
 Json json()const{Json j;j["schema"]=schema;j["pet"]=pet;j["coat"]=coat;j["x"]=x;j["y"]=y;j["scale"]=scale;j["opacity"]=opacity;j["positioned"]=positioned;j["top"]=top;j["pass"]=pass;j["autoHide"]=autoHide;j["startup"]=startup;j["autoUpdate"]=autoUpdate;j["name"]=name;j["monitor"]=monitor;j["skip"]=skip;j["lastCheck"]=lastCheck;j["timer"]=timer.json();return j;}
};
// Explicit allowlist shared by release tooling, extractor and transaction installer.
inline bool packagePath(const std::string&s){return s=="DesktopPet.exe"||s=="Updater.exe"||s=="data/dialogue.json"||s=="README.md"||s=="CHANGELOG.md";}
inline uint32_t crc32(const unsigned char*p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return ~c;}
}
