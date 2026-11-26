#include "renewed_pet.h"
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <set>
#ifndef PET_TEST_ROOT
#define PET_TEST_ROOT "."
#endif
using namespace pet;
namespace fs=std::filesystem;
static int checks=0;
static void check(bool condition,const char*text){++checks;if(!condition)throw std::runtime_error(text);}
static void reject(const std::function<void()>&operation){bool failed=false;try{operation();}catch(const std::runtime_error&){failed=true;}check(failed,"malformed or incompatible animation must be rejected");}
static std::vector<unsigned char>read(const fs::path&path){std::ifstream input(path,std::ios::binary);if(!input)throw std::runtime_error("animation input missing");return {std::istreambuf_iterator<char>(input),{}};}
static Json json(const fs::path&path){auto data=read(path);return Json::parse(std::string(data.begin(),data.end()));}
static const Action actions[]={Action::Idle,Action::Blink,Action::Sleep,Action::Happy,Action::Angry,Action::Pet,Action::Remind,Action::Yawn,Action::Stretch,Action::Belly};
static const char*names[]={"idle","blink","sleep","cuddle","angry","heart","remind","roll","bubbles","special"};
static void exportClip(const fs::path&path,const ImageClip&clip,const std::vector<size_t>&frames,Action action,int delay,int species=-1){
 fs::create_directories(path.parent_path());std::ofstream output(path,std::ios::binary);uint32_t header[]={144,144,12,uint32_t(delay)};output.write(reinterpret_cast<char*>(header),sizeof(header));
 for(int f=0;f<12;++f){std::vector<uint32_t>pixels(144*144);double time=(100000+f*delay)/1000.;int width=96,height=96,ox=24,oy=24,sway=0;if(species>=0&&species<5){height=int(96*(action==Action::Sleep?.93:1.)*(1+std::sin(time*(action==Action::Sleep?1.5:2.5))*.015));oy=(144-height)/2;if(action!=Action::Sleep)sway=int(std::lround(std::sin(time*2.8)*2));}if(action==Action::Remind)oy-=int(std::abs(std::sin((1800-f*delay)/1800.*3.14159265*3))*96*.16);
  blitNativePet(clip,frames[size_t(f)],pixels.data(),144,ox,oy,width,height,sway);
  Sprite effect;if(action==Action::Pet||action==Action::Happy||action==Action::Shy){effect.heart(4,5);effect.heart(37,4);}if(action==Action::Sleep){effect.box(37,3,6,1,0xffead9bd);effect.box(41,4,1,1,0xffead9bd);effect.box(39,5,2,1,0xffead9bd);effect.box(37,6,1,1,0xffead9bd);effect.box(37,7,6,1,0xffead9bd);}if(action==Action::Angry){effect.box(38,5,2,3,0xffe68892);effect.box(41,8,3,2,0xffe68892);effect.box(38,10,2,3,0xffe68892);effect.box(34,8,3,2,0xffe68892);}if(species==4&&(action==Action::Happy||action==Action::Pet)){effect.box(34,25,3,20,0xff98b77d);effect.box(34,29,3,1,0xff534954);effect.box(34,37,3,1,0xff534954);effect.oval(37,26,8,4,0xffa9c58a);effect.oval(28,32,8,4,0xffa9c58a);}
  for(int y=0;y<48;++y)for(int x=0;x<48;++x)if(auto color=effect.pixels[size_t(y)*48+x])for(int yy=0;yy<2;++yy)for(int xx=0;xx<2;++xx)pixels[size_t(24+y*2+yy)*144+24+x*2+xx]=color;
  output.write(reinterpret_cast<char*>(pixels.data()),std::streamsize(pixels.size()*4));
 }check(bool(output),"native review export written");
}
static void benchmark(const fs::path&root){
 volatile uint32_t checksum=0;
 for(int index=0;index<6;++index)for(const char*kind:{"ocean","marine"}){auto data=read(root/"assets/ocean"/(std::string(kind)+"-"+std::to_string(index)+".ocp"));auto clip=decodeOcean(data);std::vector<uint32_t>pixels(144*144);double aspect=double(clip.width)/clip.height;int width=std::max(1,int(96*std::min(1.,aspect))),height=std::max(1,int(96/std::max(1.,aspect)));auto before=std::clock();
  for(int f=0;f<5000;++f){std::fill(pixels.begin(),pixels.end(),0);blitNativePet(clip,size_t(f)%clip.frames.size(),pixels.data(),144,(144-width)/2,(144-height)/2,width,height);checksum^=pixels[72*144+72];}
  double microseconds=double(std::clock()-before)/CLOCKS_PER_SEC*1000000/5000;std::cout<<kind<<'-'<<index<<" cpu_us_per_frame="<<microseconds<<" decoded_bytes="<<clip.frames.size()*size_t(clip.width)*clip.height*4<<"\n";
 }std::cout<<"benchmark checksum="<<checksum<<"; Linux/offscreen only, not Windows GUI CPU or process memory\n";
}
int main(int argc,char**argv){try{
 if(argc==2&&std::string(argv[1])=="--benchmark"){benchmark(fs::path(PET_TEST_ROOT));return 0;}

 auto root=fs::path(PET_TEST_ROOT);
 // All 22 unchanged appearances: stable alpha union encloses every decoded visible pixel.
 for(int species:{0,1,3,4,5,6})for(int coat=0;coat<builtinCoats(species);++coat){auto path=species>=5?root/"assets/ocean"/("marine-"+std::to_string((species-5)*3+coat)+".ocp"):root/"assets/renewed"/("pet-"+std::to_string(species)+"-"+std::to_string(coat)+".ocp");auto art=decodeOcean(read(path));auto unionBounds=art.visibleBounds();check(!unionBounds.empty()&&unionBounds.x>=0&&unionBounds.y>=0&&unionBounds.x+unionBounds.w<=art.width&&unionBounds.y+unionBounds.h<=art.height,"all 22 stable bounds fit their original canvases");for(size_t f=0;f<art.frames.size();++f){auto frame=art.visibleBounds(f,1);check(!frame.empty()&&frame.x>=unionBounds.x&&frame.y>=unionBounds.y&&frame.x+frame.w<=unionBounds.x+unionBounds.w&&frame.y+frame.h<=unionBounds.y+unionBounds.h,"stable anchor encloses every animation frame");}}
 auto folder=root/"assets/ocean";auto manifest=json(folder/"animations.json");auto baseline=json(root/"tests/fixtures/v150_art_baseline.json");check(manifest["pets"].items().size()==6,"six marine appearances retained");size_t total=0;std::set<uint32_t>appearances;
 for(size_t index=0;index<6;++index){const auto&record=manifest["pets"].items()[index];auto bytes=read(folder/record["file"].str());total+=bytes.size();auto clip=decodeOcean(bytes);check(clip.width==128&&clip.height==128&&clip.frames.size()==120,"exact expanded resource layout");check(clip.frames.size()*128*128*4==7864320,"only selected marine cache has bounded 7.5MiB pixels");check(record["resource"].num()==201+index&&record["pet"].num()==(index<3?5:6)&&record["coat"].num()==index%3,"old species coat and resource IDs unchanged");
  for(size_t f=0;f<120;++f){const auto&frame=clip.frames[f];auto crc=crc32(reinterpret_cast<const unsigned char*>(frame.pixels.data()),frame.pixels.size()*4);check(crc==uint32_t(record["argbCRC32"].items()[f].num()),"decoded native pixels match source manifest");if(!f)appearances.insert(crc);for(int p=0;p<128;++p)check(frame.pixels[size_t(p)]==0&&frame.pixels[127*128+size_t(p)]==0&&frame.pixels[size_t(p)*128]==0&&frame.pixels[size_t(p)*128+127]==0,"clear transparent frame margins");for(auto color:frame.pixels)check(color==0||(color>>24)==255,"transparent pixels have no RGB debris or fractional white fringe");}
  for(int slot=0;slot<10;++slot){OceanPet current;current.select(index<3?5:6,int(index%3),bytes);check(current.identity()==int(index),"current appearance selected");std::vector<size_t>indices;int delay=int(record["clips"][names[slot]].items()[2].num());check(current.duration(actions[slot])==unsigned(12*delay),"native action lasts long enough to complete the blink and authored cycle");for(int f=0;f<12;++f){auto selected=current.frame(actions[slot],100000+f*delay);check(selected==size_t(slot*12+f),"runtime action routes exactly to its authored twelve-frame clip");indices.push_back(selected);}check(current.frame(actions[slot],100000+12*delay)==size_t(slot==6?83:slot*12),"seamless action loop or reminder settles after jump");if(slot==2)check(current.frame(Action::Sleep,9999999)>=24&&current.frame(Action::Sleep,9999999)<36,"sleep never returns to an awake clip");if(slot==6)check(current.frame(Action::Remind,9999999)==83,"pending reminder never restarts jumps");if(argc==3&&std::string(argv[1])=="--export")exportClip(fs::path(argv[2])/("marine-"+std::to_string(index))/(std::string(names[slot])+".raw"),current.clip(),indices,actions[slot],delay);}
  for(size_t n=0;n<bytes.size();n+=8191){reject([&]{decodeOcean({bytes.begin(),bytes.begin()+n});});}
  auto bad=bytes;bad[8]=121;reject([&]{decodeOcean(bad);});bad=bytes;bad[4]=129;reject([&]{decodeOcean(bad);});bad=bytes;bad.push_back(0);reject([&]{decodeOcean(bad);});OceanPet release;release.select(index<3?5:6,int(index%3),bytes);release.clear();check(release.identity()==-1&&release.clip().frames.empty(),"inactive resource cache is released");
 }
 check(total<4*1024*1024&&appearances.size()==6,"six distinct marine coats with compact embedded data");
 auto cats=json(root/"assets/renewed/manifest.json");for(int coat=0;coat<5;++coat){auto bytes=read(root/"assets/renewed"/cats.items()[size_t(coat)]["file"].str());RenewedPet art;art.select(0,coat,bytes);check(art.clip().frames.size()==30,"cat has six compatible poses plus two animated clips");for(int pose:{0,1,2,4}){const auto&pixels=art.clip().frames[size_t(pose)].pixels;check(crc32(reinterpret_cast<const unsigned char*>(pixels.data()),pixels.size()*4)==uint32_t(baseline["catOriginalCRC32"].items()[size_t(coat)].items()[size_t(pose)].num()),"normal cat poses are byte-identical to v1.4.0");}for(int slot=0;slot<2;++slot){std::vector<size_t>frames;auto action=slot?Action::Belly:Action::Yawn;for(int f=0;f<12;++f){auto selected=art.frame(action,100000+f*100);check(selected==size_t((slot?18:6)+f),"all five cat coats select corrected native clips");frames.push_back(selected);}if(argc==3&&std::string(argv[1])=="--export")exportClip(fs::path(argv[2])/("land-0-"+std::to_string(coat))/(slot?"belly.raw":"yawn.raw"),art.clip(),frames,action,100,0);}reject([&]{OceanPet wrong;wrong.select(5,0,bytes);});reject([&]{RenewedPet wrong;wrong.select(1,0,bytes);});}
 if(argc==3&&std::string(argv[1])=="--export"){
  const Action landActions[]={Action::Idle,Action::Blink,Action::Sleep,Action::Yawn,Action::Stretch,Action::Belly,Action::Happy,Action::Pet};const char*landNames[]={"idle","blink","sleep","yawn","stretch","belly","happy","pet"};
  for(int pet:{0,1,3,4})for(int coat=0;coat<builtinCoats(pet);++coat){RenewedPet art;art.select(pet,coat,read(root/"assets/renewed"/("pet-"+std::to_string(pet)+"-"+std::to_string(coat)+".ocp")));for(int a=0;a<8;++a){if(pet==0&&(a==3||a==5))continue;int delay=a==2?200:100;std::vector<size_t>frames;for(int f=0;f<12;++f)frames.push_back(art.frame(landActions[a],100000+f*delay));exportClip(fs::path(argv[2])/("land-"+std::to_string(pet)+"-"+std::to_string(coat))/(std::string(landNames[a])+".raw"),art.clip(),frames,landActions[a],delay,pet);}}
 }
 std::vector<uint32_t>buffer(144*144,0x12345678);ImageClip empty;blitNativePet(empty,0,buffer.data(),144,0,0,0,96);check(buffer[0]==0x12345678,"zero width never divides by zero");
 auto clip=decodeOcean(read(folder/"marine-0.ocp"));std::vector<uint32_t>guarded(8+64*64+8,0x12345678);blitNativePet(clip,0,guarded.data()+8,64,-40,-20,96,96,2);for(int i=0;i<8;++i)check(guarded[size_t(i)]==0x12345678&&guarded[guarded.size()-1-size_t(i)]==0x12345678,"negative offsets cannot write outside the native canvas");auto before=buffer;blitNativePet(empty,0,buffer.data(),144,0,0,96,96);blitNativePet(clip,120,buffer.data(),144,0,0,96,96);check(buffer==before,"empty or invalid frames leave destination unchanged");
 std::cout<<"PASS: "<<checks<<" animation checks; native clip routing, exact pixels, transparent margins, bounds, legacy cat poses and cache release\n";return 0;
 }catch(const std::exception&error){std::cerr<<"FAIL: "<<error.what()<<" (check "<<checks<<")\n";return 1;}}
