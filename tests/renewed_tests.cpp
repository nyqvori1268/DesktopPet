#include "renewed_pet.h"
#include "zip.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#ifndef PET_TEST_ROOT
#define PET_TEST_ROOT "."
#endif
using namespace pet;
static int checks=0;
static void check(bool b){++checks;if(!b)throw std::runtime_error("renewed art regression");}
static std::vector<unsigned char>read(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);if(!f)throw std::runtime_error("missing art fixture");return std::vector<unsigned char>((std::istreambuf_iterator<char>(f)),{});}
int main(){try{
 auto folder=std::filesystem::path(PET_TEST_ROOT)/"assets/renewed";auto bytes=read(folder/"manifest.json");auto manifest=Json::parse(std::string(bytes.begin(),bytes.end()));check(manifest.items().size()==16);std::set<uint32_t>identities;int appearances=0;size_t stored=0;
 Config config;config.timer.start(1000);config.timer.defer(1100,5);config.dialogueLanguage="ja-JP";config.name="保留的名稱 🐾";config.scale=150;config.opacity=80;config.x=-800;config.profiles[2].dialogues["general"]=Json::Array{"My original 台詞"};auto timer=config.timer.json().dump(),profile=config.profiles[2].json().dump();
 for(int pet:{0,1,3,4})for(int coat=0;coat<builtinCoats(pet);++coat){int index=renewedAppearance(pet,coat);check(index==appearances++);const auto&entry=manifest.items()[size_t(index)];check(entry["pet"].num()==pet&&entry["coat"].num()==coat&&entry["resource"].num()==301+index);auto data=read(folder/entry["file"].str());stored+=data.size();RenewedPet art;art.select(pet,coat,data);check(art.identity()==index);auto&clip=art.clip();check(clip.width==128&&clip.height==128&&clip.frames.size()==size_t(pet==0?30:6));std::set<uint32_t>poses;
  for(size_t pose=0;pose<6;++pose){auto&frame=clip.frames[pose];auto crc=crc32(reinterpret_cast<const unsigned char*>(frame.pixels.data()),frame.pixels.size()*4);check(crc==uint32_t(entry["argbCRC32"].items()[pose].num()));poses.insert(crc);if(!pose)identities.insert(crc);int visible=0;for(auto color:frame.pixels){check(!color||(color>>24)==255);visible+=color!=0;}check(visible>600&&visible<15000);for(int x=0;x<128;++x){check(frame.pixels[size_t(x)]==0&&frame.pixels[size_t(127)*128+x]==0);check(frame.pixels[size_t(x)*128]==0&&frame.pixels[size_t(x)*128+127]==0);}}
  check(poses.size()==6);for(int a=0;a<=int(Action::Remind);++a)check(art.frame(Action(a),1000)<clip.frames.size());check(art.frame(Action::Sleep,2000)==2);check(art.frame(Action::Sleep,99999)==2);check(art.frame(Action::Wake,100000)==1);check(art.frame(Action::Wake,100800)==4);check(art.frame(Action::Wake,101500)==0);check(art.frame(Action::Belly,102000)==size_t(pet==0?18:5));if(pet==3)check(entry["poses"].items()[5].str()=="wing-flap");
  check(config.selectBuiltin(pet,coat)&&config.pet==pet&&config.coat==coat);check(config.timer.json().dump()==timer&&config.profiles[2].json().dump()==profile&&config.name==((pet==0&&coat==0)?"保留的名稱 🐾":tr(TextId(int(TextId::PetName0)+builtinIdentity(pet,coat)),config.language))&&config.scale==150&&config.opacity==80&&config.x==-800&&config.dialogueLanguage=="ja-JP");Config restored;restored.load(config.json(),true);check(restored.pet==pet&&restored.coat==coat&&restored.timer.json().dump()==timer);art.clear();check(art.identity()==-1&&art.clip().frames.empty());
 }
 check(appearances==16&&identities.size()==16&&stored<2*1024*1024);for(int pet:{2,5,6,-1,99})check(renewedAppearance(pet,0)==-1);check(renewedAppearance(0,5)==-1&&renewedAppearance(4,1)==-1&&renewedAppearance(1,-1)==-1);
 // Combined production art: all sixteen replacements plus the six expanded ocean appearances (legacy sources remain retained).
 for(int index=0;index<6;++index){auto clip=decodeOcean(read(std::filesystem::path(PET_TEST_ROOT)/"assets/ocean"/("marine-"+std::to_string(index)+".ocp")));auto&pixels=clip.frames[0].pixels;identities.insert(crc32(reinterpret_cast<const unsigned char*>(pixels.data()),pixels.size()*4));}check(identities.size()==22);
 std::cout<<"PASS: "<<checks<<" renewed art checks; 16 coats, six distinct poses, 22 combined appearances, native timelines/cache and timer/config compatibility\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<" after "<<checks<<"\n";return 1;}}
