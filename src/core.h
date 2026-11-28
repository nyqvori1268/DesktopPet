#pragma once
#include "json.h"
#include "name_rules.h"
#include "image_core.h"
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
 Seconds due=0,snooze=0,remaining=0,started=0;std::string title=std::string(tr(TextId::S000,Language::ZhTW)),text=std::string(tr(TextId::S001,Language::ZhTW));
 static constexpr int maxInterval(Mode m){return m==Mode::Interval?86400:604800;}
 bool validInterval()const{return interval>=1&&interval<=maxInterval(mode);}
 Seconds nextAlarm(Seconds t)const{auto tm=local(t);for(int d=0;d<9;++d){auto x=tm;x.tm_mday+=d;x.tm_hour=alarmHour;x.tm_min=alarmMinute;x.tm_sec=0;x.tm_isdst=-1;auto ts=static_cast<Seconds>(std::mktime(&x));if(ts<=t)continue;auto actual=local(ts);if(repeat!=2||(weekdays&(1<<actual.tm_wday)))return ts;}return 0;}
 void start(Seconds t){started=t;interval=std::clamp(interval,1,maxInterval(mode));running=true;paused=false;pending=false;snooze=0;remaining=0;due=mode==Mode::Alarm?nextAlarm(t):t+interval;}
 void stop(){running=paused=pending=false;due=snooze=remaining=0;}
 void pause(Seconds t){if(running&&!paused){remaining=std::max<Seconds>(0,due-t);if(snooze)snooze=std::max<Seconds>(1,snooze-t);paused=true;}}
 void resume(Seconds t){if(running&&paused){if(due)due=t+remaining;if(snooze)snooze+=t;paused=false;}}
 // Advance scheduled rounds on their original phase. Rounds covered by a
 // snooze merge into its single delivery; they cannot interrupt or replay it.
 bool tick(Seconds t){
  if(!running||paused)return false;
  const bool deferred=snooze!=0;
  bool fired=deferred&&t>=snooze;
  if(due&&t>=due){
   if(!deferred)fired=true;
   if(mode==Mode::Interval){
    const int step=std::clamp(interval,1,maxInterval(mode));
    due+=((t-due)/step+1)*step;
   }else if(mode==Mode::Alarm&&repeat)due=nextAlarm(t);
   else due=0;
  }
  if(fired){snooze=0;pending=true;if(!due)running=false;}
  return fired;
 }
 void acknowledge(){pending=false;}
 void defer(Seconds t,int minutes){
  if(paused)resume(t);
  pending=false;snooze=t+std::clamp(minutes,1,1440)*60;running=true;paused=false;
 }
 Json json()const{Json j; j["started"]=started;j["mode"]=int(mode);j["running"]=running;j["paused"]=paused;j["pending"]=pending;j["interval"]=interval;j["hour"]=alarmHour;j["minute"]=alarmMinute;j["weekdays"]=weekdays;j["repeat"]=repeat;j["snoozeMinutes"]=snoozeMinutes;j["due"]=due;j["snooze"]=snooze;j["remaining"]=remaining;j["title"]=title;j["text"]=text;return j;}
 void load(const Json&j,bool restore){mode=Mode(bounded(j["mode"],0,0,2));const int savedInterval=bounded(j["interval"],1800,1,604800);interval=std::min(savedInterval,maxInterval(mode));alarmHour=bounded(j["hour"],9,0,23);alarmMinute=bounded(j["minute"],0,0,59);weekdays=bounded(j["weekdays"],127,1,127);repeat=bounded(j["repeat"],0,0,2);snoozeMinutes=bounded(j["snoozeMinutes"],5,1,1440);title=clipUtf8(j["title"].str(std::string(tr(TextId::S000,Language::ZhTW))),360);text=clipUtf8(j["text"].str(std::string(tr(TextId::S001,Language::ZhTW))),2000);if(restore){started=Seconds(std::clamp(j["started"].num(),0.0,4102444800.0));running=j["running"].boolean();paused=j["paused"].boolean();pending=j["pending"].boolean();due=Seconds(std::clamp(j["due"].num(),0.0,4102444800.0));snooze=Seconds(std::clamp(j["snooze"].num(),0.0,4102444800.0));if(snooze)pending=false;remaining=Seconds(std::clamp(j["remaining"].num(),0.0,604800.0));if(mode==Mode::Interval&&savedInterval!=interval){if(due)due=std::max<Seconds>(1,due-savedInterval+interval);remaining=std::min<Seconds>(remaining,interval);}}}
};
// Three bounded live tasks. One pending slot per mode merges overdue rounds.
struct TimerModes{
 std::array<Timer,3> tasks;std::vector<int> queue;Mode selected=Mode::Countdown;
 TimerModes(){for(int i=0;i<3;++i)tasks[size_t(i)].mode=Mode(i);}
 Timer&at(Mode mode){return tasks.at(size_t(mode));}const Timer&at(Mode mode)const{return tasks.at(size_t(mode));}
 void reconcile(){std::vector<int> next;for(int i:queue)if(i>=0&&i<3&&tasks[size_t(i)].pending&&std::find(next.begin(),next.end(),i)==next.end())next.push_back(i);for(int i=0;i<3;++i)if(tasks[size_t(i)].pending&&std::find(next.begin(),next.end(),i)==next.end())next.push_back(i);queue=std::move(next);}
 int front()const{return queue.empty()?-1:queue.front();}
 bool pending()const{return !queue.empty();}
 bool tick(Seconds t){bool fired=false;for(auto&task:tasks){bool wasPending=task.pending;bool due=task.tick(t);fired=(due&&!wasPending)||fired;}reconcile();return fired;}
 void acknowledge(){if(front()>=0)tasks[size_t(front())].acknowledge();reconcile();}
 void defer(Seconds t,int minutes){if(front()>=0)tasks[size_t(front())].defer(t,minutes);reconcile();}
 void stopAll(){for(auto&task:tasks)task.stop();queue.clear();}
 void migrate(const Timer&legacy){for(int i=0;i<3;++i){auto&task=tasks[size_t(i)];task=legacy;task.mode=Mode(i);task.interval=std::min(task.interval,Timer::maxInterval(task.mode));if(task.mode!=legacy.mode){task.stop();task.started=0;}}selected=legacy.mode;queue.clear();reconcile();}
 Json json()const{Json j;Json::Array list,order;for(auto&task:tasks)list.push_back(task.json());for(int i:queue)order.emplace_back(i);j["tasks"]=list;j["queue"]=order;j["selected"]=int(selected);return j;}
 void load(const Json&j,bool restore){selected=Mode(bounded(j["selected"],0,0,2));for(int i=0;i<3;++i){Json saved;if(j["tasks"].items().size()>size_t(i))saved=j["tasks"].items()[size_t(i)];saved["mode"]=i;tasks[size_t(i)]=Timer{};tasks[size_t(i)].load(saved,restore);}queue.clear();if(restore)for(auto&entry:j["queue"].items()){int value=bounded(entry,-1,-1,3);if(value>=0&&value<3)queue.push_back(value);}reconcile();}
};
inline constexpr std::array<const char*,8> dialogueStates{"general","happy","pet","angry","drag","sleep","wake","remind"};
inline const char* dialogueSpecies(int pet,int coat){return pet==0?"cat":pet==1?"dog":pet==3?(coat>=3?"cockatiel":"budgie"):pet==4?"panda":pet==5?"dolphin":pet==6?"whaleshark":"generic";}
inline bool dialogueStateValid(const std::string&state){return std::find(dialogueStates.begin(),dialogueStates.end(),state)!=dialogueStates.end();}
inline std::vector<std::string> dialogueLines(const std::string&text,bool strict=false){
 std::vector<std::string> result;size_t start=0;
 while(start<text.size()){auto end=text.find('\n',start);auto line=text.substr(start,end==std::string::npos?std::string::npos:end-start);if(!line.empty()&&line.back()=='\r')line.pop_back();
  if(line.find_first_not_of(" \t")!=std::string::npos){if(strict&&(line.size()>360||result.size()>=24))throw std::runtime_error(tr(TextId::DialogueLimit));if(result.size()<24){line=clipUtf8(line,360);if(std::find(result.begin(),result.end(),line)==result.end())result.push_back(line);}}
  if(end==std::string::npos)break;
  start=end+1;
 }return result;
}
inline std::vector<std::string> validDialogueList(const Json&list){std::string joined;for(auto&item:list.items()){auto line=item.str();if(!line.empty()){joined+=line;joined+='\n';}}return dialogueLines(joined);}
inline std::string chooseDialogue(const std::vector<std::string>&lines,std::string&previous,unsigned random){
 if(lines.empty())return {};
 std::vector<size_t> eligible;for(size_t i=0;i<lines.size();++i)if(lines[i]!=previous)eligible.push_back(i);
 auto index=eligible.empty()?0:eligible[random%eligible.size()];previous=lines[index];return previous;
}
struct CustomProfile{
 std::string name=std::string(tr(TextId::S002,Language::ZhTW));Json files,fileLabels,dialogues,dialogueLanguages;bool dialogueEnabled=true,genericFallback=true,nameDefault=true;
 const Json&dialogueFor(Language language)const{return language==Language::ZhTW?dialogues:dialogueLanguages[languageCode(language)];}
 void setDialogue(Language language,const std::string&state,const Json&lines){if(language==Language::ZhTW)dialogues[state]=lines;else dialogueLanguages[languageCode(language)][state]=lines;}
 bool ready()const{return customFileName(files["idle"].str());}
 void load(const Json&j,const std::string&fallback){name=clipUtf8(j["name"].str(fallback),120);if(name.empty())name=fallback;dialogueEnabled=j["dialogueEnabled"].boolean(true);genericFallback=j["genericFallback"].boolean(true);nameDefault=j["nameDefault"].boolean(j["name"].str().empty());dialogues=Json{};dialogueLanguages=Json{};for(int language=0;language<3;++language){auto code=languageCode(Language(language));const auto&source=j["dialogueLanguages"][code];for(auto state:dialogueStates){const auto&saved=language==0&&!source[state].array()?j["dialogues"][state]:source[state];Json::Array lines;for(auto&line:validDialogueList(saved))lines.emplace_back(line);setDialogue(Language(language),state,lines);}}fileLabels=Json::Object{};for(auto state:customStates){auto label=clipUtf8(j["fileLabels"][state].str(),240);if(!label.empty())fileLabels[state]=label;}files=Json{};for(auto state:customStates){auto file=j["files"][state].str();if(customFileName(file))files[state]=file;}}
 Json json()const{Json j;j["nameDefault"]=nameDefault;j["name"]=name;j["files"]=files;j["fileLabels"]=Json::Object{};for(auto state:customStates)if(!fileLabels[state].str().empty())j["fileLabels"][state]=fileLabels[state].str();j["dialogueLanguages"]=Json::Object{};for(int language=0;language<3;++language)for(auto state:dialogueStates){Json::Array lines;for(auto&line:validDialogueList(dialogueFor(Language(language))[state]))lines.emplace_back(line);if(language==0)j["dialogues"][state]=lines;else if(!lines.empty())j["dialogueLanguages"][languageCode(Language(language))][state]=lines;}j["dialogueEnabled"]=dialogueEnabled;j["genericFallback"]=genericFallback;return j;}
};
inline int builtinCoats(int species){return species==4?1:species==5||species==6?3:species==0||species==1||species==3?5:0;}
inline int builtinIdentity(int species,int coat){if(coat<0||coat>=builtinCoats(species))return -1;return species==0?coat:species==1?5+coat:species==3?10+coat:species==4?15:species==5?16+coat:19+coat;}
struct Config{
 int schema=1,pet=0,coat=0,x=0,y=0,scale=100,opacity=100;bool positioned=false,top=true,pass=false,autoHide=true,startup=false,autoUpdate=true;std::string name=std::string(tr(TextId::S003,Language::ZhTW)),monitor,builtinName=std::string(tr(TextId::S003,Language::ZhTW));Language language=Language::ZhTW;std::string dialogueLanguage="follow-ui";bool dialogueEnabled=true;int activeCustom=0;std::array<CustomProfile,3> profiles;Seconds lastCheck=0;Timer timer;TimerModes timers;bool independentTimers=false;
 void enableTimerModes(){if(!independentTimers){timers.migrate(timer);independentTimers=true;}}
 std::array<std::string,22> builtinNames{};bool showNameplate=true;int dialogueEditorWidth=640,dialogueEditorHeight=480;std::string syncedName=name;
 Config(){for(int i=0;i<3;++i)profiles[size_t(i)].name=std::string(tr(TextId::S004,Language::ZhTW))+std::to_string(i+1);syncName();}
 std::string profileName(int slot)const{const auto&p=profiles.at(size_t(slot));const auto legacy=std::string(tr(TextId::S004,Language::ZhTW))+std::to_string(slot+1);return p.nameDefault&&p.name==legacy?formatText(TextId::NameCustom,{std::to_string(slot+1)},language):p.name;}
 std::string effectiveName()const{if(pet==2)return profileName(activeCustom);int index=builtinIdentity(pet,coat);if(index<0)return name;return builtinNames[size_t(index)].empty()?tr(TextId(int(TextId::PetName0)+index),language):builtinNames[size_t(index)];}
 void syncName(){name=effectiveName();syncedName=name;if(pet!=2)builtinName=name;}
 void rememberName(){if(name==syncedName||name.empty())return;if(pet==2){profiles[size_t(activeCustom)].name=clipUtf8(name,120);profiles[size_t(activeCustom)].nameDefault=false;}else{int index=builtinIdentity(pet,coat);if(index>=0)builtinNames[size_t(index)]=clipUtf8(name,120);builtinName=name;}}
 void rename(const std::string&value){validatePetName(value);auto text=value;if(pet==2){profiles[size_t(activeCustom)].name=text;profiles[size_t(activeCustom)].nameDefault=false;}else{int index=builtinIdentity(pet,coat);if(index>=0)builtinNames[size_t(index)]=text;}syncName();}
 void resetName(){if(pet==2){auto&p=profiles[size_t(activeCustom)];p.name=std::string(tr(TextId::S004,Language::ZhTW))+std::to_string(activeCustom+1);p.nameDefault=true;}else{int index=builtinIdentity(pet,coat);if(index>=0)builtinNames[size_t(index)].clear();}syncName();}
 void setLanguage(Language value){rememberName();language=supportedLanguage(int(value));syncName();}
 bool selectBuiltin(int species,int color=0){if(!builtinCoats(species))return false;rememberName();pet=species;coat=std::clamp(color,0,builtinCoats(species)-1);syncName();return true;}
 bool selectCustom(int index){if(index<0||index>=3||!profiles[size_t(index)].ready())return false;rememberName();activeCustom=index;pet=2;syncName();return true;}
 void clearProfile(int index){if(index<0||index>=3)return;profiles[size_t(index)]=CustomProfile{};profiles[size_t(index)].name=std::string(tr(TextId::S004,Language::ZhTW))+std::to_string(index+1);if(pet==2&&activeCustom==index)selectBuiltin(0);}
 void loadBuiltinNames(const Json&j){
  builtinNames.fill({});bool indexed=j["builtinNames"].array();
  if(indexed){const auto&list=j["builtinNames"].items();for(size_t i=0;i<std::min(list.size(),builtinNames.size());++i)builtinNames[i]=clipUtf8(list[i].str(),120);}
  // New saves explicitly distinguish an empty/default slot from a legacy omission.
  if(j["nameSchema"].num()>=1||(indexed&&pet==2))return;
  int index=pet==2?0:builtinIdentity(pet,coat);if(index<0||!builtinNames[size_t(index)].empty())return;
  auto legacy=pet==2?j["builtinName"].str():j["name"].str(j["builtinName"].str());if(legacy.empty())return;
  if(indexed){for(int language=0;language<3;++language)if(legacy==tr(TextId(int(TextId::PetName0)+index),Language(language)))return;}
  builtinNames[size_t(index)]=clipUtf8(legacy,120);
 }
 void load(const Json&j,bool restore){if(j["schema"].num(1)>schema)return;language=j["language"].str().empty()?Language::ZhTW:languageFromCode(j["language"].str());dialogueEnabled=j["dialogueEnabled"].boolean(true);dialogueLanguage=j["dialogueLanguage"].str("follow-ui");if(dialogueLanguage!="follow-ui")dialogueLanguage=languageCode(languageFromCode(dialogueLanguage));pet=bounded(j["pet"],0,0,6);coat=bounded(j["coat"],0,0,pet==2?4:builtinCoats(pet)-1);scale=bounded(j["scale"],100,50,300);opacity=bounded(j["opacity"],100,20,100);x=bounded(j["x"],0,-100000,100000);y=bounded(j["y"],0,-100000,100000);positioned=j["positioned"].boolean();top=j["top"].boolean(true);pass=j["pass"].boolean();autoHide=j["autoHide"].boolean(true);startup=j["startup"].boolean();autoUpdate=j["autoUpdate"].boolean(true);name=clipUtf8(j["name"].str(std::string(tr(TextId::S003,Language::ZhTW))),120);builtinName=clipUtf8(j["builtinName"].str(pet==2?std::string(tr(TextId::S003,Language::ZhTW)):name),120);activeCustom=bounded(j["activeCustom"],0,0,2);for(int i=0;i<3;++i){Json profile;if(j["customProfiles"].array()){if(size_t(i)<j["customProfiles"].items().size())profile=j["customProfiles"].items()[size_t(i)];}else if(i==0){profile["name"]=j["customName"].str(pet==2?name:std::string(tr(TextId::S005,Language::ZhTW)));profile["files"]=j["customFiles"];}profiles[size_t(i)].load(profile,std::string(tr(TextId::S004,Language::ZhTW))+std::to_string(i+1));}if(!j["customProfiles"].array())activeCustom=0;if(pet==2)name=profiles[size_t(activeCustom)].name;monitor=j["monitor"].str();lastCheck=Seconds(std::clamp(j["lastCheck"].num(),0.0,4102444800.0));timer.load(j["timer"],restore);independentTimers=j["timerModes"].object()&&j["timerModes"]["tasks"].array()&&j["timerModes"]["tasks"].items().size()==3;if(independentTimers)timers.load(j["timerModes"],restore);showNameplate=j["showNameplate"].boolean(true);dialogueEditorWidth=bounded(j["dialogueEditorWidth"],640,480,1600);dialogueEditorHeight=bounded(j["dialogueEditorHeight"],480,320,1200);loadBuiltinNames(j);syncName();}
 Json json()const{Config snapshot=*this;snapshot.rememberName();snapshot.syncName();Json j;Json::Array names;for(auto&value:snapshot.builtinNames)names.emplace_back(value);j["nameSchema"]=1;j["builtinNames"]=names;j["showNameplate"]=showNameplate;j["dialogueEditorWidth"]=dialogueEditorWidth;j["dialogueEditorHeight"]=dialogueEditorHeight;j["schema"]=schema;j["language"]=languageCode(language);j["dialogueLanguage"]=dialogueLanguage;j["dialogueEnabled"]=dialogueEnabled;j["pet"]=pet;j["coat"]=coat;j["x"]=x;j["y"]=y;j["scale"]=scale;j["opacity"]=opacity;j["positioned"]=positioned;j["top"]=top;j["pass"]=pass;j["autoHide"]=autoHide;j["startup"]=startup;j["autoUpdate"]=autoUpdate;j["name"]=snapshot.effectiveName();j["builtinName"]=snapshot.builtinName;j["customName"]=snapshot.profiles[0].name;j["customFiles"]=snapshot.profiles[0].files;j["activeCustom"]=activeCustom;Json::Array savedProfiles;for(auto&profile:snapshot.profiles)savedProfiles.push_back(profile.json());j["customProfiles"]=savedProfiles;j["monitor"]=monitor;j["lastCheck"]=lastCheck;j["timer"]=independentTimers?timers.at(timers.selected).json():timer.json();if(independentTimers)j["timerModes"]=timers.json();return j;}
};
// Species lookup never falls through to another animal or another language.
inline std::vector<std::string> dialogueCandidates(const Config&c,const Json&library,const std::string&state){
 if(!dialogueStateValid(state))return {};
 if(state!="remind"&&!c.dialogueEnabled)return {};
 if(c.pet==2){const auto&profile=c.profiles[size_t(c.activeCustom)];if(state!="remind"&&!profile.dialogueEnabled)return {};auto custom=validDialogueList(profile.dialogueFor(c.language)[state]);if(!custom.empty())return custom;}
 auto language=c.pet==2?c.language:effectiveDialogueLanguage(c.language,c.dialogueLanguage);const Json&pack=language==Language::ZhTW?library:library["languages"][languageCode(language)];
 auto lines=validDialogueList(pack[dialogueSpecies(c.pet,c.coat)][state]);if(lines.empty())lines=validDialogueList(pack["generic"][state]);
 if(lines.empty())lines.push_back(tr(state=="remind"?TextId::GenericReminder:TextId::GenericDialogue,language));
 return lines;
}
// Explicit allowlist shared by release tooling, extractor and transaction installer.
inline bool packagePath(const std::string&s){return s=="DesktopPet.exe"||s=="Updater.exe"||s=="data/dialogue.json"||s=="README.md"||s=="CHANGELOG.md";}
inline uint32_t crc32(const unsigned char*p,size_t n){uint32_t c=~0u;while(n--){c^=*p++;for(int i=0;i<8;++i)c=(c>>1)^(0xedb88320u&uint32_t(-int(c&1)));}return ~c;}
}
