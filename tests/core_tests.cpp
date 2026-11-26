#include "core.h"
#include "update_logic.h"
#include "trust_logic.h"
#include "sprite.h"
#include "ocean.h"
#include "zip.h"
#include <iostream>
#include <set>
#include <random>
#include <functional>
#include <fstream>
#include <filesystem>
#ifndef PET_TEST_FIXTURES
#define PET_TEST_FIXTURES "tests/fixtures"
#endif
using namespace pet;
static int checks=0;
static void check(bool ok,const char*name){++checks;if(!ok)throw std::runtime_error(name);}
static void rejects(const std::function<void()>&f,const char*name){bool rejected=false;try{f();}catch(...){rejected=true;}check(rejected,name);}
static void p16(std::vector<unsigned char>&b,unsigned v){b.push_back(v&255);b.push_back((v>>8)&255);}
static void p32(std::vector<unsigned char>&b,uint32_t v){p16(b,v&65535);p16(b,v>>16);}
static std::vector<unsigned char> zip(const std::vector<std::string>&names){std::vector<unsigned char>b,cd;for(auto&name:names){auto offset=b.size();std::string data="pet: "+name;auto crc=crc32(reinterpret_cast<const unsigned char*>(data.data()),data.size());p32(b,0x04034b50);p16(b,20);p16(b,0);p16(b,0);p32(b,0);p32(b,crc);p32(b,uint32_t(data.size()));p32(b,uint32_t(data.size()));p16(b,unsigned(name.size()));p16(b,0);b.insert(b.end(),name.begin(),name.end());b.insert(b.end(),data.begin(),data.end());p32(cd,0x02014b50);p16(cd,20);p16(cd,20);p16(cd,0);p16(cd,0);p32(cd,0);p32(cd,crc);p32(cd,uint32_t(data.size()));p32(cd,uint32_t(data.size()));p16(cd,unsigned(name.size()));p16(cd,0);p16(cd,0);p16(cd,0);p16(cd,0);p32(cd,0);p32(cd,uint32_t(offset));cd.insert(cd.end(),name.begin(),name.end());}auto offset=b.size();b.insert(b.end(),cd.begin(),cd.end());p32(b,0x06054b50);p16(b,0);p16(b,0);p16(b,unsigned(names.size()));p16(b,unsigned(names.size()));p32(b,uint32_t(cd.size()));p32(b,uint32_t(offset));p16(b,0);return b;}
// All clocks below are simulated seconds; no sleeps or real-time waits.
static void timerRegressions(){
 for(auto mode:{Mode::Countdown,Mode::Interval,Mode::Alarm}){
  for(int minutes:{5,10,15,17}){
   Timer t;t.mode=mode;t.interval=1800;t.repeat=0;
   t.start(1000);const Seconds first=t.due;
   check(t.tick(first)&&t.pending,"initial reminder");
   const Seconds phase=t.due;t.defer(first,minutes);
   const Seconds deadline=first+minutes*60;
   check(!t.pending&&t.running&&t.snooze==deadline,"defer clears pending and schedules chosen duration");
   check(!t.tick(deadline-1)&&!t.pending,"snooze never early");
   check(t.tick(deadline)&&t.pending&&!t.snooze,"snooze delivered exactly at deadline");
   check(!t.tick(deadline),"no duplicate at same timestamp");
   if(mode==Mode::Interval)check(t.due==phase,"defer retains interval phase");
   t.acknowledge();check(!t.pending,"deferred reminder can be acknowledged");
   t.defer(deadline,minutes);check(!t.pending&&t.tick(deadline+minutes*60),"reminder can be deferred again");
  }
 }
 // Interval rounds inside a snooze window merge into that one deferred reminder.
 Timer t;t.mode=Mode::Interval;t.interval=60;t.start(1000);check(t.tick(1060),"short interval initial reminder");
 t.defer(1060,5);check(!t.tick(1120)&&t.due==1180&&!t.pending,"next round cannot interrupt snooze");
 check(!t.tick(1300)&&t.due==1360,"multiple covered rounds advance silently");
 check(t.tick(1360)&&t.due==1420&&!t.snooze,"coincident snooze and round fire once");
 check(!t.tick(1360),"coincident timestamp not repeated");t.acknowledge();check(t.tick(1420),"next original round retained");
 t.defer(1420,5);check(t.tick(5000)&&t.due==5020&&!t.snooze,"wake after overlapping deadlines coalesces once");
 check(!t.tick(5000),"wake does not replay overdue rounds");
 t.stop();t.interval=180;t.start(1000);check(t.tick(1180),"offset interval first round");
 t.defer(1180,5);check(!t.tick(1360)&&t.due==1540,"round before nonaligned snooze is merged");
 check(t.tick(1480)&&t.due==1540,"nonaligned snooze deadline delivered");
 t.acknowledge();check(t.tick(1540),"round after snooze not lost or phase shifted");
 // Pausing a deferred single task must not resurrect its consumed due timestamp.
 for(auto mode:{Mode::Countdown,Mode::Alarm}){
  Timer single;single.mode=mode;single.interval=10;single.start(1000);auto first=single.due;
  check(single.tick(first)&&single.due==0,"single task original deadline consumed");single.defer(first,5);
  single.pause(first+30);check(!single.tick(first+10000),"deferred task stays paused");
  single.resume(first+1000);check(single.due==0&&!single.tick(first+1000),"resume does not resurrect original deadline");
  check(!single.tick(first+1269)&&single.tick(first+1270),"paused snooze keeps remaining duration");
 }
 // Repeating alarms also merge overlapping scheduled and deferred deadlines.
 for(int repeat:{1,2}){
  Timer alarm;alarm.mode=Mode::Alarm;alarm.alarmHour=9;alarm.alarmMinute=0;alarm.repeat=repeat;alarm.weekdays=127;
  std::tm tm{};tm.tm_year=126;tm.tm_mon=0;tm.tm_mday=5;tm.tm_hour=8;tm.tm_isdst=-1;
  auto clock=Seconds(std::mktime(&tm));alarm.start(clock);auto first=alarm.due;check(alarm.tick(first),"repeating alarm first reminder");
  auto next=alarm.due;alarm.defer(first+60,1440);check(!alarm.tick(next)&&!alarm.pending,"alarm round during snooze coalesces");
  auto snooze=alarm.snooze;check(alarm.tick(snooze)&&!alarm.snooze&&alarm.due>snooze,"daily or weekday alarm deferred delivery");
 }
 // Existing JSON fields suffice for update/restart recovery during a snooze.
 t.mode=Mode::Interval;t.interval=60;t.start(1000);t.tick(1060);t.defer(1060,10);
 Timer restored;restored.load(Json::parse(t.json().dump()),true);
 check(!restored.tick(1600)&&restored.tick(1660)&&restored.due==1720,"restore keeps snooze and original phase");
 auto legacyOverlap=t.json();legacyOverlap["pending"]=true;restored.load(legacyOverlap,true);
 check(!restored.pending&&!restored.tick(1600)&&restored.tick(1660),"legacy overlap pending merges into existing snooze");
 restored.stop();check(!restored.tick(9999),"stop cancels deferred reminder");
 // UI and core share the same interval bound; other modes keep seven days.
 check(Timer::maxInterval(Mode::Interval)==86400&&Timer::maxInterval(Mode::Countdown)==604800,"mode-specific limits");
 for(int duration:{0,1,86399,86400,86401,86460,90000,604800}){
  Timer interval;interval.mode=Mode::Interval;interval.interval=duration;
  check(interval.validInterval()==(duration>=1&&duration<=86400),"interval validity boundary");
  interval.start(1000);check(interval.interval==std::clamp(duration,1,86400),"start clamps invalid core interval");
  Timer loaded;loaded.load(interval.json(),false);check(loaded.interval==interval.interval,"valid interval roundtrip");
 }
 Json legacy;legacy["mode"]=int(Mode::Interval);legacy["interval"]=604800;legacy["running"]=true;legacy["due"]=605800;legacy["remaining"]=604700;
 Timer migrated;migrated.load(legacy,true);check(migrated.interval==86400&&migrated.due==87400&&migrated.remaining==86400,"legacy interval and active phase normalized");
 legacy["mode"]=int(Mode::Countdown);migrated=Timer{};migrated.load(legacy,true);check(migrated.interval==604800&&migrated.due==605800,"countdown legacy limit unchanged");
 legacy["mode"]=int(Mode::Alarm);migrated=Timer{};migrated.load(legacy,true);check(migrated.interval==604800,"alarm configuration limit unchanged");
}
static std::vector<unsigned char> fixture(const std::string&name){
 std::ifstream file(std::filesystem::u8path(PET_TEST_FIXTURES)/name,std::ios::binary);
 if(!file)throw std::runtime_error("Missing image test fixture");
 return {std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
}
static void imageRegressions(){
 auto png=fixture("alpha.png");auto meta=inspectImage(png);check(meta.width==16&&meta.height==16&&meta.frames==1&&!meta.gif,"PNG header");
 auto gif=fixture("disposal.gif");meta=inspectImage(gif);check(meta.width==8&&meta.height==8&&meta.frames==4&&meta.gif,"GIF partial-frame header and extensions");
 for(size_t i=0;i<gif.size();++i){auto cut=std::vector<unsigned char>(gif.begin(),gif.begin()+i);rejects([&]{inspectImage(cut);},"truncated GIF rejected");}
 auto oversized=png;oversized[16]=0x7f;rejects([&]{inspectImage(oversized);},"oversize PNG rejected before decode");
 auto wrong=png;wrong[12]='X';rejects([&]{inspectImage(wrong);},"invalid IHDR");rejects([]{inspectImage({'J','P','E','G'});},"unapproved format");
 rejects([]{inspectImage(std::vector<unsigned char>(8*1024*1024+1));},"encoded size limit");
 auto hugeGif=gif;hugeGif[6]=0;hugeGif[7]=8;hugeGif[8]=0;hugeGif[9]=8;rejects([&]{inspectImage(hugeGif);},"GIF aggregate decoded budget");
 ImageClip clip;clip.width=5;clip.height=4;clip.frames.resize(2);for(auto&f:clip.frames)f.pixels.resize(20);
 clip.frames[0].delay=100;clip.frames[1].delay=200;clip.frames[0].pixels[6]=0x80800000;clip.frames[0].pixels[7]=0xff00ff00;clip.frames[1].pixels[13]=0xff0000ff;
 clip.findCrop();check(clip.crop.x==1&&clip.crop.y==1&&clip.crop.w==3&&clip.crop.h==2,"union alpha crop across all frames");
 check(clip.sample(0,1.5,1)==0xc0408000,"bilinear premultiplied alpha has no black halo");
 auto trimmed=trimClip(clip);check(trimmed.width==3&&trimmed.height==2&&trimmed.frames[0].pixels[0]==0x80800000&&trimmed.frames[1].pixels[5]==0xff0000ff,"crop does not pixelize or discard late-frame bounds");
 check(trimmed.frameAt(0)==0&&trimmed.frameAt(99)==0&&trimmed.frameAt(100)==1&&trimmed.frameAt(299)==1&&trimmed.frameAt(300)==0,"GIF timeline exact boundary and loop");
 ImageClip empty;empty.width=empty.height=1;empty.frames.push_back({{0},100});rejects([&]{trimClip(empty);},"fully transparent image rejected");empty.frames[0].pixels.clear();rejects([&]{empty.findCrop();},"invalid pixel count rejected");
 ImageClip whitespace;whitespace.width=whitespace.height=512;whitespace.frames.push_back({std::vector<uint32_t>(512*512),100});whitespace.frames[0].pixels[250*512+250]=0xffffffff;auto small=trimClip(std::move(whitespace));check(small.width==1&&small.height==1,"trim before resampling preserves small artwork");
 check(gifDelay(0)==40&&gifDelay(1)==40&&gifDelay(8)==80&&gifDelay(999999)==10000,"bounded GIF timing");
 rejects([]{decodeSide(0);},"zero frames rejected");rejects([]{decodeSide(121);},"frame count limit");for(unsigned count=1;count<=120;++count){int side=decodeSide(count);check(side<=256&&uint64_t(side)*side*count*4<=2*1024*1024,"bounded frame cache");}
 check(customFileName(std::string(64,'a')+".png")&&customFileName(std::string(64,'f')+".gif"),"content hash filenames allowed");
 for(auto name:{"../idle.png","C:/idle.gif","https://example.com/a.png","idle.png","a.png:evil"})check(!customFileName(name),"arbitrary paths rejected");
 check(customState(Action::Blink)==1&&customState(Action::Sleep)==2&&customState(Action::Happy)==3&&customState(Action::Angry)==4&&customState(Action::Pet)==5&&customState(Action::Remind)==6&&customState(Action::Drag)==0,"seven custom action slots");
 Config custom;custom.pet=2;custom.profiles[0].name="小桃";custom.name=custom.profiles[0].name;custom.builtinName="小橘";custom.profiles[0].files["idle"]=std::string(64,'a')+".png";custom.timer.start(1000);
 Config restored;restored.load(Json::parse(custom.json().dump()),true);check(restored.pet==2&&restored.name=="小桃"&&restored.builtinName=="小橘"&&restored.profiles[0].files["idle"].str()==custom.profiles[0].files["idle"].str()&&restored.timer.due==custom.timer.due,"custom profile and timer roundtrip");
 auto bad=custom.json();auto profiles=bad["customProfiles"].items();profiles[0]["files"]["idle"]="../../config.json";bad["customProfiles"]=profiles;restored.load(bad,true);check(restored.profiles[0].files["idle"].str().empty(),"bad asset path ignored without changing timer");
}
static void v111Regressions(){
 Json legacy;legacy["pet"]=2;legacy["name"]="小桃";legacy["customName"]="小桃";legacy["customFiles"]["idle"]=std::string(64,'a')+".png";legacy["customFiles"]["sleep"]=std::string(64,'b')+".gif";
 legacy["timer"]["mode"]=1;legacy["timer"]["running"]=true;legacy["timer"]["due"]=4000;legacy["x"]=-1500;legacy["scale"]=175;
 Config c;c.load(legacy,true);check(c.activeCustom==0&&c.profiles[0].name=="小桃"&&c.profiles[0].files["sleep"].str()==std::string(64,'b')+".gif","v1.1.0 profile migrates intact to slot one");
 check(!c.profiles[1].ready()&&!c.profiles[2].ready(),"migration leaves other slots independent");
 for(int i=0;i<3;++i){c.profiles[size_t(i)].name="角色 "+std::to_string(i);for(auto state:customStates)c.profiles[size_t(i)].files[state]=std::string(64,char('a'+i))+".png";}
 auto timer=c.timer.json().dump();int x=c.x,scale=c.scale;
 for(int i=0;i<3;++i){check(c.selectCustom(i)&&c.activeCustom==i&&c.name=="角色 "+std::to_string(i),"select independent custom slot");check(c.timer.json().dump()==timer&&c.x==x&&c.scale==scale,"switch preserves timer and desktop");}
 auto json=c.json();Config restored;restored.load(json,true);check(restored.activeCustom==2&&restored.name=="角色 2"&&restored.profiles[0].files["heart"].str()!=restored.profiles[2].files["heart"].str(),"three seven-action profiles roundtrip independently");
 c.clearProfile(0);check(!c.profiles[0].ready()&&c.pet==2&&c.activeCustom==2&&c.profiles[2].ready(),"clear inactive slot does not affect active slot");
 c.clearProfile(2);check(!c.profiles[2].ready()&&c.pet==0&&c.timer.json().dump()==timer,"clear active slot returns to builtin without stopping timer");
 check(!c.selectCustom(3)&&!c.selectCustom(-1)&&!c.selectCustom(0)&&!c.selectBuiltin(2),"invalid or empty selection rejected");
 for(int pet:{0,1,3,4}){check(c.selectBuiltin(pet,4)&&c.pet==pet&&c.timer.json().dump()==timer,"all builtin species preserve running timer");}
 auto malformed=json;malformed["activeCustom"]=99;restored.load(malformed,true);check(restored.activeCustom==2,"active profile clamped");
 std::string notes="# 更新紀錄\n## 1.1.10\n不屬於本版\n## 1.1.1（未發布）\n- **修正更新**\n- [說明](https://example.com)\n### 寵物\n新增熊貓\n## 1.1.0\n舊版文字\n";
 auto text=readableNotes(notes,"1.1.1");check(text.find("修正更新")!=std::string::npos&&text.find("新增熊貓")!=std::string::npos&&text.find("舊版文字")==std::string::npos&&text.find("不屬於本版")==std::string::npos&&text.find("未發布")==std::string::npos&&text.find("**")==std::string::npos,"only current release plain text displayed");
 check(text.find("說明（https://example.com）")!=std::string::npos,"markdown links remain readable");check(versionNotes(notes,"1.0.9").find("未提供")!=std::string::npos,"missing release does not show unrelated versions");check(readableNotes("- 新功能","1.1.1").find("新功能")!=std::string::npos,"already isolated notes supported");
 check(parentDecision(false,true,false)==ParentDecision::Reject,"exited unrelated PID never accepted");check(parentDecision(true,false,false)==ParentDecision::Wait,"verified live parent must exit normally");check(parentDecision(true,true,true)==ParentDecision::Exited,"already exited verified handle wins over timeout");check(parentDecision(true,false,true)==ParentDecision::Timeout,"timeout never permits replacement");
 Sprite image;std::set<uint32_t> identities;for(int pet:{0,1,3,4})for(int coat=0;coat<(pet==4?1:5);++coat){image.make(pet,coat,Action::Idle,0);identities.insert(crc32(reinterpret_cast<unsigned char*>(image.pixels.data()),image.pixels.size()*4));if(pet<3)continue;for(int a=0;a<=int(Action::Remind);++a){image.make(pet,coat,Action(a),1.3);int visible=0;for(auto color:image.pixels){check(!color||(color>>24)==255,"new species alpha");visible+=bool(color);}check(visible>200&&visible<1800,"new species visible in 48px canvas");}}
 check(identities.size()==16,"six new and ten existing appearances are distinct");image.make(3,3,Action::Idle,0);check(std::count(image.pixels.begin(),image.pixels.end(),0xffe7a079)>0,"cockatiel orange cheeks");image.make(4,0,Action::Happy,0);check(std::count(image.pixels.begin(),image.pixels.end(),0xff98b77d)>0,"panda bamboo pose");
}
static void trustRegressions(){
 const std::string primary(128,'a'),backup(128,'b'),rotated(128,'c'),nextBackup(128,'d');TrustState old;old.primary=primary;old.backup=backup;old.highest="1.1.1";
 auto verifier=[&](const std::string&,const std::vector<unsigned char>&sig,const std::string&key){return sig.size()==64&&sig[0]==0xbb&&key==backup;};
 Json policy;policy["schema"]=1;policy["epoch"]=1;policy["primary"]=rotated;policy["backup"]=backup;policy["minimumVersion"]="1.1.1";policy["revoked"]=Json::Array{};Json envelope;envelope["payload"]=policy.dump();envelope["signature"]=std::string(128,'b');
 auto state=applyPolicy(old,envelope,verifier);check(state.accepts(rotated)&&state.accepts(backup)&&!state.accepts(primary),"authorized primary rotation and revocation");check(!state.permits("1.1.0")&&state.permits("1.1.1"),"trust version floor");state.highest="1.2.0";check(!state.permits("1.1.9")&&state.permits("1.2.0"),"highest installed version prevents downgrade");
 auto bad=envelope;bad["signature"]=std::string(128,'a');rejects([&]{applyPolicy(old,bad,verifier);},"primary alone cannot rotate an offline-backup-protected ring");
 rejects([&]{applyPolicy(state,envelope,verifier);},"old policy epoch cannot replay");
 auto changed=policy;changed["epoch"]=2;bad["payload"]=changed.dump();bad["signature"]=std::string(128,'b');rejects([&]{applyPolicy(old,bad,verifier);},"policy cannot skip epochs");
 changed=policy;changed["epoch"]=2;changed["primary"]=primary;bad["payload"]=changed.dump();rejects([&]{applyPolicy(state,bad,verifier);},"revoked key cannot be reintroduced");
 changed["primary"]=rotated;changed["minimumVersion"]="1.0.0";bad["payload"]=changed.dump();rejects([&]{applyPolicy(state,bad,verifier);},"policy floor cannot decrease");
 changed=policy;changed["epoch"]=2;changed["backup"]=nextBackup;bad["payload"]=changed.dump();auto backupRotated=applyPolicy(state,bad,verifier);check(backupRotated.accepts(nextBackup)&&!backupRotated.accepts(backup),"old offline authority can rotate and revoke itself");
 Json chain=Json::Array{envelope},extended=Json::Array{envelope,bad};check(chainExtends(chain,extended)&&!chainExtends(extended,chain),"trust chain is append only");auto rewritten=chain;rewritten=Json::Array{bad};check(!chainExtends(chain,rewritten),"existing policy prefix cannot be rewritten");
 TrustState single;single.primary=primary;auto bootstrap=[&](const std::string&,const std::vector<unsigned char>&sig,const std::string&key){return sig.size()==64&&key==primary;};check(applyPolicy(single,envelope,bootstrap).backup==backup,"single-key root can explicitly authorize first backup");
 rejects([&]{applyPolicy(single,envelope,verifier);},"unknown backup cannot bootstrap itself");
 check(old.accepts(primary)&&old.accepts(backup),"legacy main and emergency backup signatures both trusted");
 rejects([]{publicPoint("unknown");},"invalid point encoding rejected");bad=envelope;bad["signature"]="00";rejects([&]{applyPolicy(old,bad,verifier);},"truncated policy signature rejected");
}
static void v120Regressions(){
 std::set<std::string> ids;
 for(const auto&entry:translations){check(ids.insert(entry.id).second,"unique translation ID");for(int lang=0;lang<3;++lang){check(std::string(entry.value[size_t(lang)]).size()>0,"translation complete");Json text=std::string(entry.value[size_t(lang)]);check(Json::parse(text.dump()).str()==text.str(),"translation valid Unicode");for(int arg=0;arg<5;++arg){auto token="{"+std::to_string(arg)+"}";check((std::string(entry.value[0]).find(token)!=std::string::npos)==(std::string(entry.value[size_t(lang)]).find(token)!=std::string::npos),"translation placeholders consistent");}}}
 check(detectDisplayLanguage("zh-TW")==Language::ZhTW&&detectDisplayLanguage("zh-Hant-HK")==Language::ZhTW&&detectDisplayLanguage("ja-JP")==Language::JaJP&&detectDisplayLanguage("fr-FR")==Language::EnUS,"display language detection and fallback");
 for(int lang=0;lang<3;++lang)for(int mask=0;mask<128;++mask)check(weekdayMask(weekdayText(mask,Language(lang)),Language(lang))==mask,"localized weekday masks");
 Config old;old.load(Json::parse("{\"pet\":2,\"activeCustom\":2,\"customProfiles\":[{\"name\":\"一\",\"files\":{\"idle\":\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa.png\"}},{\"name\":\"二\",\"files\":{\"idle\":\"bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb.png\"}},{\"name\":\"三\",\"files\":{\"idle\":\"cccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc.png\"}}]}"),true);
 check(old.language==Language::ZhTW&&old.dialogueLanguage=="follow-ui"&&old.dialogueEnabled,"v111 language and dialogue migration");check(old.activeCustom==2&&old.profiles[2].name=="三"&&old.profiles[2].ready()&&old.profiles[2].dialogueEnabled&&old.profiles[2].genericFallback,"v111 profiles preserved with safe dialogue defaults");
 std::ifstream input(std::filesystem::path(PET_TEST_FIXTURES).parent_path().parent_path()/"data/dialogue.json");check(bool(input),"dialogue data available");std::string bytes((std::istreambuf_iterator<char>(input)),{});auto library=Json::parse(bytes);
 for(int lang=0;lang<3;++lang){const Json&pack=lang==0?library:library["languages"][languageCode(Language(lang))];for(auto species:{"cat","dog","budgie","cockatiel","panda","generic"})for(auto state:dialogueStates){const auto lines=validDialogueList(pack[species][state]);check(lines.size()>=2,"all species states have multiple lines in all languages");check(std::set<std::string>(lines.begin(),lines.end()).size()==lines.size(),"dialogue has no duplicate entries");}}
 Config c;int appearances=0;
 for(int species:{0,1,3,4})for(int coat=0;coat<(species==4?1:5);++coat){++appearances;c.pet=species;c.coat=coat;for(int lang=0;lang<3;++lang){c.language=Language(lang);for(auto state:dialogueStates){auto lines=dialogueCandidates(c,library,state);const Json&pack=lang==0?library:library["languages"][languageCode(Language(lang))];check(lines==validDialogueList(pack[dialogueSpecies(species,coat)][state]),"all 16 appearance routes remain within species");if(species==3)for(auto&line:lines)for(auto bark:{"汪","Woof","woof","ワン"})check(line.find(bark)==std::string::npos,"bird never uses dog bark");std::string previous;for(unsigned n=0;n<20;++n){auto before=previous;check(!chooseDialogue(lines,previous,n).empty()&&previous!=before,"dialogue avoids consecutive repetition");}}}}
 check(appearances==16,"all existing appearances covered");check(std::string(dialogueSpecies(3,0))=="budgie"&&std::string(dialogueSpecies(3,2))=="budgie"&&std::string(dialogueSpecies(3,3))=="cockatiel"&&std::string(dialogueSpecies(3,4))=="cockatiel","bird species split by existing coat identity");
 Json missing;missing["dog"]["general"]=Json::Array{"DOG_ONLY"};c.pet=3;c.coat=0;c.language=Language::EnUS;check(dialogueCandidates(c,missing,"general")[0]==tr(TextId::GenericDialogue,Language::EnUS),"missing bird or language never falls back to dog or different locale");missing["languages"]["en-US"]["generic"]["general"]=Json::Array{"NEUTRAL"};check(dialogueCandidates(c,missing,"general")[0]=="NEUTRAL","missing species uses neutral fallback");
 c.timer.mode=Mode::Interval;c.timer.interval=60;c.timer.start(1000);c.timer.tick(1060);c.timer.title="我的 {0} Reminder";c.timer.text="自訂提醒 🐾";
 for(int i=0;i<3;++i){auto&profile=c.profiles[size_t(i)];profile.files["idle"]=std::string(64,char('a'+i))+".png";profile.name="角色 "+std::to_string(i);for(auto state:dialogueStates)profile.dialogues[state]=Json::Array{"繁體中文 にゃ 🐾 "+std::to_string(i),"Custom original "+std::to_string(i)};}
 auto timerBefore=c.timer.json().dump();for(int i=0;i<3;++i){check(c.selectCustom(i),"switch custom profile");auto original=dialogueCandidates(c,library,"general");for(int lang=0;lang<3;++lang){c.language=Language(lang);setUILanguage(c.language);check(dialogueCandidates(c,library,"general")==original,"custom dialogue never translated");check(c.timer.json().dump()==timerBefore,"language and profile switches preserve timer and custom reminder");}}
 c.language=Language::JaJP;c.dialogueLanguage="en-US";check(effectiveDialogueLanguage(c.language,c.dialogueLanguage)==Language::EnUS&&effectiveDialogueLanguage(c.language,"follow-ui")==Language::JaJP,"independent or following dialogue language");Config restored;restored.load(Json::parse(c.json().dump()),true);check(restored.json().dump()==c.json().dump(),"all three dialogue profiles and settings survive restart and update");
 c.dialogueEnabled=false;check(dialogueCandidates(c,library,"general").empty()&&!dialogueCandidates(c,library,"remind").empty()&&c.timer.pending,"disable ordinary dialogue preserves reminder");c.dialogueEnabled=true;c.profiles[2].dialogueEnabled=false;check(dialogueCandidates(c,library,"sleep").empty()&&!dialogueCandidates(c,library,"remind").empty(),"profile dialogue toggle preserves reminders");c.profiles[2].dialogueEnabled=true;c.profiles[2].dialogues=Json{};c.profiles[2].genericFallback=false;check(dialogueCandidates(c,library,"general").empty(),"empty custom dialogue can be silent");c.profiles[2].genericFallback=true;check(dialogueCandidates(c,library,"general")==validDialogueList(library["languages"]["en-US"]["generic"]["general"]),"empty custom dialogue chooses generic only");check(dialogueCandidates(c,library,"unknown").empty(),"unknown interaction safely ignored");
 check(dialogueLines("第一句\r\n\n Second line \nにゃ 🐾\n第一句",true)==std::vector<std::string>({"第一句"," Second line ","にゃ 🐾"}),"multiline Unicode import preserves original text and removes blank duplicates");rejects([]{dialogueLines(std::string(361,'x'),true);},"overlong dialogue rejected");std::string tooMany;for(int i=0;i<25;++i)tooMany+=std::to_string(i)+"\n";rejects([&]{dialogueLines(tooMany,true);},"too many lines rejected");check(dialogueLines(tooMany).size()==24,"corrupt saved line count bounded");
 auto sibling=c.profiles[0].json().dump();c.clearProfile(2);check(c.profiles[2].dialogues.items().empty()&&c.profiles[0].json().dump()==sibling&&c.timer.json().dump()==timerBefore,"clear one profile preserves sibling dialogues and timer");setUILanguage(Language::ZhTW);
}
static void v130Regressions(){
 auto root=std::filesystem::path(PET_TEST_FIXTURES).parent_path().parent_path();auto read=[](const std::filesystem::path&file){std::ifstream in(file,std::ios::binary);if(!in)throw std::runtime_error("test file missing");return std::vector<unsigned char>((std::istreambuf_iterator<char>(in)),{});};auto text=[&](const std::filesystem::path&file){auto bytes=read(file);return Json::parse(std::string(bytes.begin(),bytes.end()));};
 auto provenance=text(root/"assets/ocean/provenance.json"),library=text(root/"data/dialogue.json");std::set<uint32_t>identities;size_t stored=0;
 for(int index=0;index<6;++index){auto bytes=read(root/"assets/ocean"/("ocean-"+std::to_string(index)+".ocp"));stored+=bytes.size();auto clip=decodeOcean(bytes);const auto&record=provenance.items()[size_t(index)];check(clip.width==record["size"].items()[0].num()&&clip.height==record["size"].items()[1].num()&&clip.frames.size()==6,"all six supplied assets decode at original cropped dimensions");
  for(size_t f=0;f<6;++f){const auto&frame=clip.frames[f];auto crc=crc32(reinterpret_cast<const unsigned char*>(frame.pixels.data()),frame.pixels.size()*4);check(crc==uint32_t(record["argbCRC32"].items()[f].num()),"native decoding exactly preserves each of 36 cleaned original frames");check(frame.delay==180&&frame.pixels.size()==size_t(clip.width)*clip.height,"original frame timing and fixed geometry");if(!f)identities.insert(crc);bool transparent=false,opaque=false;for(auto color:frame.pixels){check(!color||(color>>24)==255,"ocean premultiplied transparent pixels");transparent|=color==0;opaque|=color!=0;}check(transparent&&opaque,"transparent background and visible ocean artwork");}
  check(oceanSample(clip,0,0,0)==clip.frames[0].pixels[0]&&!oceanSample(clip,99,0,0)&&!oceanSample(clip,0,-1,0),"nearest sampler and bounds");check(clip.frameAt(0)==0&&clip.frameAt(180)==1&&clip.frameAt(1080)==0,"original six-frame timeline");
  for(size_t n=0;n<bytes.size();n+=997){auto cut=std::vector<unsigned char>(bytes.begin(),bytes.begin()+n);rejects([&]{decodeOcean(cut);},"truncated ocean asset rejected");}auto changed=bytes;changed[0]^=1;rejects([&]{decodeOcean(changed);},"wrong ocean magic rejected");changed=bytes;changed.push_back(0);rejects([&]{decodeOcean(changed);},"trailing ocean data rejected");changed=bytes;changed[4]=changed[5]=255;rejects([&]{decodeOcean(changed);},"ocean dimensions bounded");changed=bytes;changed[8]=255;rejects([&]{decodeOcean(changed);},"ocean frame count bounded");changed=bytes;changed[12]=changed[13]=0;rejects([&]{decodeOcean(changed);},"empty palette rejected");changed=bytes;size_t packet=18+4*(unsigned(bytes[12])|(unsigned(bytes[13])<<8));changed[packet]=128;changed[packet+1]=255;rejects([&]{decodeOcean(changed);},"out of range palette index rejected");changed=bytes;changed[21]=128;rejects([&]{decodeOcean(changed);},"non-premultiplied or fractional-alpha palette rejected");
  OceanPet current;current.select(index<3?5:6,index%3,bytes);check(current.identity()==index,"ocean appearance mapping");check(current.frame(Action::Sleep,0)==current.frame(Action::Sleep,999999),"sleep stops swimming with original closed-eye frame");check(current.frame(Action::Blink,17)==size_t(index==1||index==4||index==5?3:2),"each appearance uses its own supplied blink frame");for(int action=0;action<=int(Action::Remind);++action)check(current.frame(Action(action),273)<6,"every interaction retains valid original artwork");current.clear();check(current.identity()==-1&&current.clip().frames.empty(),"inactive marine cache released");
 }
 check(identities.size()==6&&stored<2*1024*1024,"six distinct original appearances fit under 2 MiB embedded");Sprite existing;for(int pet:{0,1,3,4})for(int coat=0;coat<builtinCoats(pet);++coat){existing.make(pet,coat,Action::Idle,0);identities.insert(crc32(reinterpret_cast<const unsigned char*>(existing.pixels.data()),existing.pixels.size()*4));}check(identities.size()==22,"all 22 builtin appearances have distinct actual raster data");
 auto first=read(root/"assets/ocean/ocean-0.ocp");OceanPet dolphin,whale;dolphin.select(5,0,first);whale.select(6,0,first);check(dolphin.frame(Action::Idle,360)==2&&whale.frame(Action::Idle,360)==1,"dolphin swims faster than whale shark");rejects([&]{dolphin.select(5,3,first);},"invalid marine coat rejected");
 Config c;c.timer.interval=60;c.timer.start(1000);c.timer.defer(1060,5);c.name="自訂海洋 🐬";c.scale=175;c.opacity=65;c.x=-900;c.y=120;c.profiles[1].dialogues["general"]=Json::Array{"原文 preserved"};c.load(c.json(),true);auto timer=c.timer.json().dump(),profiles=c.profiles[1].json().dump();int appearances=0;std::set<std::string>species;
 for(int pet:{0,1,3,4,5,6})for(int coat=0;coat<builtinCoats(pet);++coat){++appearances;check(c.selectBuiltin(pet,coat),"22 builtin appearances selectable");check(c.timer.json().dump()==timer&&c.profiles[1].json().dump()==profiles&&c.name=="自訂海洋 🐬"&&c.x==-900&&c.scale==175&&c.opacity==65,"all switches preserve timer desktop custom names and profile dialogue");species.insert(dialogueSpecies(pet,coat));
  for(int language=0;language<3;++language){c.language=Language(language);const Json&pack=language==0?library:library["languages"][languageCode(c.language)];for(auto state:dialogueStates){auto lines=dialogueCandidates(c,library,state);check(lines.size()>=2&&lines==validDialogueList(pack[dialogueSpecies(pet,coat)][state]),"all 22 appearances route eight states to their own species in all three languages");if(pet>=5)for(auto&line:lines)for(auto forbidden:{"汪","喵","肉球","頭冠","竹子","Woof","Meow","ワン","にゃ","冠羽","肉球"})check(line.find(forbidden)==std::string::npos,"marine dialogue never borrows land animal lines");}}
  auto saved=c.json();Config restored;restored.load(Json::parse(saved.dump()),true);check(restored.json().dump()==saved.dump(),"new and legacy IDs roundtrip without settings or timer loss");}
 check(appearances==22&&species.size()==7,"22 builtin appearances and seven isolated animal species");Json absent;absent["dog"]["general"]=Json::Array{"DOG_ONLY"};for(int pet:{5,6}){c.pet=pet;auto lines=dialogueCandidates(c,absent,"general");check(lines.size()==1&&lines[0]==tr(TextId::GenericDialogue,c.language),"missing marine dialogue uses neutral localized fallback only");}
 for(int pet:{5,6}){Json bad=c.json();bad["pet"]=pet;bad["coat"]=4;Config loaded;loaded.load(bad,true);check(loaded.pet==pet&&loaded.coat==2,"saved marine coat range clamped consistently");}
 check(!oceanSpecies(2)&&oceanAppearance(3,0)==-1&&!c.selectBuiltin(2),"existing custom profile ID remains reserved");
}
int main(){try{
 v130Regressions();v120Regressions();trustRegressions();v111Regressions();imageRegressions();timerRegressions();
 auto j=Json::parse("{\"text\":\"繁體中文\\n\\uD83D\\uDC31\",\"n\":-1.25e2,\"b\":true,\"a\":[null,2]}");check(j["n"].num()==-125,"json number");check(Json::parse(j.dump()).dump()==j.dump(),"json roundtrip");check(j["missing"].str("fallback")=="fallback","missing default");
 for(auto s:{"", "01","1.","1e", "NaN","1e999", "[1,]", "{\"a\":1,\"a\":2}","\"\\uD800\"", "true false", "\"x\n\""})rejects([&]{Json::parse(s);},"strict JSON rejection");
 rejects([]{Json::parse(std::string(40,'[')+std::string(40,']'));},"JSON nesting bound");rejects([]{Json::parse(std::string(1024*1024+1,' '));},"JSON size bound");
 rejects([]{Json::parse(std::string("\"\xc0\xaf\""));},"invalid raw UTF8 rejected");check(clipUtf8("小橘",4)=="小","UTF8 safe truncation");
 Config c;check(c.pet==0&&c.coat==0&&c.name=="小橘"&&!c.timer.running,"defaults");Json bad;bad["scale"]=1e20;bad["opacity"]=-1;bad["timer"]["interval"]=0;bad["timer"]["running"]=true;c.load(bad,false);check(c.scale==300&&c.opacity==20&&c.timer.interval==1&&!c.timer.running,"config clamp and normal launch stops task");c.timer.interval=300;c.timer.start(1000);c.timer.pending=true;Config r;r.load(Json::parse(c.json().dump()),true);check(r.timer.running&&r.timer.pending&&r.timer.due==1300,"cross-update state roundtrip");Json future;future["schema"]=99;Config untouched;untouched.load(future,true);check(untouched.name=="小橘","future schema safe default");
 Timer t;t.interval=60;t.start(1000);check(!t.tick(1059),"countdown before deadline");check(t.tick(1060)&&t.pending&&t.due==0,"countdown at deadline");check(!t.tick(2000)&&!t.running,"single task completes and stops");t.acknowledge();check(!t.pending,"acknowledge");t.start(1000);t.pause(1020);check(!t.tick(9999)&&t.remaining==40,"pause");t.resume(2000);check(t.due==2040&&t.tick(2040),"resume");t.defer(2040,5);check(!t.tick(2339)&&t.tick(2340),"snooze countdown");
 t.mode=Mode::Interval;t.interval=1800;t.start(1000);check(t.tick(10000)&&t.due==11800,"sleep catchup retains phase");check(!t.tick(10001),"no catchup storm");auto phase=t.due;t.defer(10001,5);check(t.tick(10301)&&t.due==phase,"snooze preserves interval");t.stop();check(!t.running&&!t.pending&&!t.snooze,"stop clears state");
 auto base=local(now());base.tm_hour=8;base.tm_min=0;base.tm_sec=0;base.tm_isdst=-1;auto morning=Seconds(std::mktime(&base));t.mode=Mode::Alarm;t.alarmHour=9;t.alarmMinute=30;t.repeat=0;t.start(morning);check(local(t.due).tm_hour==9&&local(t.due).tm_min==30&&t.due>morning,"local one-shot alarm");check(t.tick(t.due)&&!t.due,"single alarm only once");t.repeat=1;t.start(morning);auto first=t.due;check(t.tick(first+3*86400)&&t.due>first+3*86400,"daily sleep correction");t.repeat=2;t.weekdays=1<<1;t.start(morning);check(local(t.due).tm_wday==1,"specified weekday");
 check(Version::parse("v1.2.3").has_value(),"release tag");check(*Version::parse("1.9.9")<*Version::parse("1.10.0"),"numeric semver");for(auto s:{"1.0.0-beta","1.0.0+meta","01.0.0","1.0","1.0.0.1","-1.0.0","1.0.1000001"})check(!Version::parse(s),"reject non-stable version");
 Sprite sp;std::set<uint32_t> identities;for(int pet=0;pet<2;++pet)for(int coat=0;coat<5;++coat){sp.make(pet,coat,Action::Idle,0);identities.insert(crc32(reinterpret_cast<unsigned char*>(sp.pixels.data()),sp.pixels.size()*4));for(int a=0;a<=int(Action::Remind);++a){sp.make(pet,coat,Action(a),1.3);int opaque=0;for(auto p:sp.pixels){check(p==0||(p>>24)==255,"sprite alpha");if(p)++opaque;}check(opaque>200&&opaque<1800,"visible pixel sprite");}}check(identities.size()==10,"ten distinct coats");sp.make(0,0,Action::Idle,0);auto cat=sp.pixels;sp.make(1,0,Action::Idle,0);int silhouettes=0;for(size_t i=0;i<cat.size();++i)if(bool(cat[i])!=bool(sp.pixels[i]))++silhouettes;check(silhouettes>15,"different silhouettes");Spring spring;spring.drag(120,-90);for(int i=0;i<500;++i)spring.step(.016);check(!spring.moving()&&std::abs(spring.x-1)<.001,"spring settles");
 std::vector<std::string> names={"DesktopPet.exe","Updater.exe","data/dialogue.json","README.md","CHANGELOG.md"};auto b=zip(names);check(inspectZip(b).size()==5,"safe ZIP");auto corrupt=b;corrupt[50]^=1;rejects([&]{inspectZip(corrupt);},"ZIP corruption");for(auto path:{"../config.json","C:/evil.exe","data/../../evil","config.json","Data/dialogue.json","data\\dialogue.json","//server/file","data/dialogue.json:evil"}){auto n=names;n[2]=path;rejects([&]{inspectZip(zip(n));},"unsafe ZIP path");}auto duplicate=names;duplicate[1]=duplicate[0];rejects([&]{inspectZip(zip(duplicate));},"duplicate ZIP");for(size_t n=0;n<b.size();++n){auto cut=std::vector<unsigned char>(b.begin(),b.begin()+n);rejects([&]{inspectZip(cut);},"ZIP truncation");}auto symlink=b;size_t cd=0;for(size_t n=0;n+4<b.size();++n)if(b[n]==0x50&&b[n+1]==0x4b&&b[n+2]==1&&b[n+3]==2){cd=n;break;}symlink[cd+41]=0xa0;rejects([&]{inspectZip(symlink);},"ZIP symlink");std::mt19937 rng(7);for(int i=0;i<2000;++i){std::string input;for(unsigned n=rng()%90;n;--n)input+=char(rng()%128);try{Json::parse(input);}catch(const std::runtime_error&){} }for(int i=0;i<2000;++i){auto mutated=b;mutated[rng()%mutated.size()]^=static_cast<unsigned char>(1u<<(rng()%8));try{inspectZip(mutated);}catch(const std::runtime_error&){} }check(crc32(reinterpret_cast<const unsigned char*>("123456789"),9)==0xcbf43926,"CRC vector");std::cout<<"PASS: "<<checks<<" checks; JSON/config/timers/alarm/semver/sprites/spring/ZIP\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
