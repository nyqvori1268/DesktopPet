#pragma once
#include "image_core.h"
#include "sprite.h"
namespace pet {
inline bool oceanSpecies(int pet){return pet==5||pet==6;}
inline int oceanAppearance(int pet,int coat){return oceanSpecies(pet)&&coat>=0&&coat<3?(pet-5)*3+coat:-1;}
// Bounded, lossless palette/PackBits reader. No GIF decoder or external runtime.
inline ImageClip decodeOcean(const std::vector<unsigned char>&data){
 auto fail=[](){throw std::runtime_error(tr(TextId::OceanResourceError));};size_t pos=0;
 auto take=[&](unsigned bytes)->uint32_t{if(pos>data.size()||data.size()-pos<bytes)fail();uint32_t value=0;for(unsigned n=0;n<bytes;++n)value|=uint32_t(data[pos++])<<(n*8);return value;};
 if(data.size()>4*1024*1024||take(4)!=0x3150434f)fail();
 ImageClip clip;clip.width=int(take(2));clip.height=int(take(2));auto count=take(2);
 const bool legacy=count==6;const bool expanded=clip.width==128&&clip.height==128&&(count==30||count==120);
 if(clip.width<1||clip.width>512||clip.height<1||clip.height>627||(!legacy&&!expanded)||uint64_t(clip.width)*clip.height*count*4>(legacy?6u:8u)*1024*1024)fail();
 clip.crop={0,0,clip.width,clip.height};clip.frames.reserve(count);size_t pixels=size_t(clip.width)*clip.height;
 for(unsigned f=0;f<count;++f){ImageFrame frame;frame.delay=take(2);auto colors=take(2),size=take(4);if(frame.delay<40||frame.delay>2000||!colors||colors>256)fail();std::vector<uint32_t>palette;palette.reserve(colors);for(unsigned c=0;c<colors;++c){auto color=take(4);if(color&&color>>24!=255)fail();palette.push_back(color);}if(pos>data.size()||size>data.size()-pos)fail();size_t end=pos+size;frame.pixels.reserve(pixels);
  while(pos<end){auto tag=data[pos++];size_t length=(tag&127)+1;if(length>pixels-frame.pixels.size())fail();if(tag&128){if(pos>=end)fail();auto index=data[pos++];if(index>=colors)fail();frame.pixels.insert(frame.pixels.end(),length,palette[index]);}else{if(length>end-pos)fail();for(size_t n=0;n<length;++n){auto index=data[pos++];if(index>=colors)fail();frame.pixels.push_back(palette[index]);}}}
  if(frame.pixels.size()!=pixels)fail();
  clip.frames.push_back(std::move(frame));
 }
 if(pos!=data.size())fail();
 return clip;
}
inline uint32_t oceanSample(const ImageClip&clip,size_t frame,int x,int y){if(frame>=clip.frames.size()||x<0||y<0||x>=clip.width||y>=clip.height)return 0;return clip.frames[frame].pixels[size_t(y)*clip.width+x];}
// Shared by the layered-window render path and offscreen animation review.
inline void blitNativePet(const ImageClip&clip,size_t frame,uint32_t*destination,int canvas,int ox,int oy,int width,int height,int sway=0){
 if(!destination||canvas<1||width<1||height<1||clip.width<1||clip.height<1||frame>=clip.frames.size())return;
 for(int y=0;y<height;++y){int yy=oy+y;if(yy<0||yy>=canvas)continue;for(int x=0;x<width;++x){int xx=ox+x;if(xx<0||xx>=canvas)continue;destination[size_t(yy)*canvas+xx]=oceanSample(clip,frame,std::clamp(x*clip.width/width-sway*y/height,0,clip.width-1),std::min(clip.height-1,y*clip.height/height));}}
}
class OceanPet{
 ImageClip image;int selected=-1;Action active=Action::Idle;uint64_t started=0;bool first=true;
public:
 static constexpr int actionSlot(Action action){switch(action){case Action::Blink:return 1;case Action::Sleep:return 2;case Action::Happy:case Action::Shy:return 3;case Action::Angry:return 4;case Action::Pet:return 5;case Action::Remind:case Action::Wake:case Action::Surprise:return 6;case Action::Yawn:return 7;case Action::Stretch:return 8;case Action::Belly:return 9;default:return 0;}}
 int identity()const{return selected;}
 void clear(){image=ImageClip{};selected=-1;first=true;}
 void select(int pet,int coat,const std::vector<unsigned char>&data){int next=oceanAppearance(pet,coat);if(next<0)throw std::runtime_error(tr(TextId::OceanResourceError));auto decoded=decodeOcean(data);if(decoded.frames.size()!=6&&decoded.frames.size()!=120)throw std::runtime_error(tr(TextId::OceanResourceError));image=std::move(decoded);selected=next;first=true;}
 const ImageClip&clip()const{return image;}
 unsigned duration(Action action,unsigned fallback=2300)const{return image.frames.size()==120?12*image.frames[size_t(actionSlot(action))*12].delay:fallback;}
 size_t frame(Action action,uint64_t time){
  if(image.frames.empty())return 0;
  if(image.frames.size()==120){
   if(first||action!=active){active=action;started=time;first=false;}auto elapsed=time>=started?time-started:0;
   int slot=actionSlot(action);if(action==Action::Wake&&elapsed>=600){slot=0;elapsed-=600;}
   const size_t offset=size_t(slot)*12;if(action==Action::Remind&&elapsed>=1800)return offset+11;
   return offset+size_t(elapsed/image.frames[offset].delay%12);
  }
  // Sleep rests in place. Swim timing alone changes; artwork and proportions do not.
  if(action==Action::Sleep||action==Action::Blink)return selected==1||selected==4||selected==5?3:2;
  auto elapsed=selected>=3?time*3/5:time;
  if(action==Action::Happy||action==Action::Wake)elapsed=elapsed*6/5;
  return image.frameAt(elapsed);
 }
};
}
