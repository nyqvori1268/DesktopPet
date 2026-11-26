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
 if(clip.width<1||clip.width>512||clip.height<1||clip.height>627||count!=6||uint64_t(clip.width)*clip.height*count*4>6*1024*1024)fail();
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
class OceanPet{
 ImageClip image;int selected=-1;
public:
 int identity()const{return selected;}
 void clear(){image=ImageClip{};selected=-1;}
 void select(int pet,int coat,const std::vector<unsigned char>&data){int next=oceanAppearance(pet,coat);if(next<0)throw std::runtime_error(tr(TextId::OceanResourceError));auto decoded=decodeOcean(data);image=std::move(decoded);selected=next;}
 const ImageClip&clip()const{return image;}
 size_t frame(Action action,uint64_t time)const{
  if(image.frames.empty())return 0;
  // Sleep rests in place. Swim timing alone changes; artwork and proportions do not.
  if(action==Action::Sleep||action==Action::Blink)return selected==1||selected==4||selected==5?3:2;
  auto elapsed=selected>=3?time*3/5:time;
  if(action==Action::Happy||action==Action::Wake)elapsed=elapsed*6/5;
  return image.frameAt(elapsed);
 }
};
}
