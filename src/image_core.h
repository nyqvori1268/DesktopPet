#pragma once
#include "sprite.h"
#include <string>
#include <vector>
#include <stdexcept>
#include <limits>
namespace pet {
inline constexpr const char* customStates[]={"idle","blink","sleep","happy","angry","heart","remind"};
inline bool customFileName(const std::string&s){if(s.size()!=68)return false;return s.substr(64)==".png"||s.substr(64)==".gif"?s.substr(0,64).find_first_not_of("0123456789abcdef")==std::string::npos:false;}
inline int customState(Action a){switch(a){case Action::Blink:return 1;case Action::Sleep:return 2;case Action::Happy:case Action::Shy:return 3;case Action::Angry:return 4;case Action::Pet:return 5;case Action::Remind:return 6;default:return 0;}}
struct EncodedImage{unsigned width=0,height=0,frames=0;bool gif=false;};
inline EncodedImage inspectImage(const std::vector<unsigned char>&b){
 if(b.empty()||b.size()>8*1024*1024)throw std::runtime_error(tr(TextId::S027));
 auto fail=[](){throw std::runtime_error(tr(TextId::S028));};
 auto le16=[&](size_t i){if(i>b.size()||b.size()-i<2)fail();return unsigned(b[i])|(unsigned(b[i+1])<<8);};
 auto be32=[&](size_t i){if(i>b.size()||b.size()-i<4)fail();return (unsigned(b[i])<<24)|(unsigned(b[i+1])<<16)|(unsigned(b[i+2])<<8)|unsigned(b[i+3]);};
 EncodedImage out;
 if(b.size()>=33&&std::equal(b.begin(),b.begin()+8,std::array<unsigned char,8>{137,80,78,71,13,10,26,10}.begin())){
  if(be32(8)!=13||std::string(b.begin()+12,b.begin()+16)!="IHDR")fail();
  out={be32(16),be32(20),1,false};
 }else if(b.size()>=13&&(std::string(b.begin(),b.begin()+6)=="GIF89a"||std::string(b.begin(),b.begin()+6)=="GIF87a")){
  out={le16(6),le16(8),0,true};
  if(!out.width||!out.height||out.width>2048||out.height>2048)fail();
  size_t p=13;if(b[10]&128)p+=3u*(1u<<((b[10]&7)+1));
  auto blocks=[&]{for(;;){if(p>=b.size())fail();unsigned n=b[p++];if(!n)break;if(n>b.size()-p)fail();p+=n;}};
  bool ended=false;while(p<b.size()){
   unsigned tag=b[p++];if(tag==0x3b){ended=true;break;}
   if(tag==0x21){if(p>=b.size())fail();
   ++p;blocks();continue;}
   if(tag!=0x2c||p>b.size()||b.size()-p<9)fail();
   unsigned x=le16(p),y=le16(p+2),w=le16(p+4),h=le16(p+6);unsigned packed=b[p+8];p+=9;
   if(!w||!h||w>out.width||h>out.height||x>out.width-w||y>out.height-h||++out.frames>120)fail();
   if(packed&128)p+=3u*(1u<<((packed&7)+1));
   if(p>=b.size()||b[p]<2||b[p]>8)fail();
   ++p;blocks();
  }
  if(!ended||p!=b.size()||!out.frames)fail();
 }else fail();
 if(!out.width||!out.height||out.width>2048||out.height>2048)fail();
 if(uint64_t(out.width)*out.height*out.frames*4>16*1024*1024)throw std::runtime_error(tr(TextId::S029));
 return out;
}
struct PixelRect{int x=0,y=0,w=0,h=0;bool empty()const{return w<=0||h<=0;}};
struct ImageFrame{std::vector<uint32_t> pixels;unsigned delay=100;};
struct ImageClip{
 int width=0,height=0;PixelRect crop;std::vector<ImageFrame> frames;
 size_t frameAt(uint64_t elapsed)const{if(frames.empty())return 0;uint64_t total=0;for(auto&f:frames)total+=f.delay;if(!total)return 0;elapsed%=total;for(size_t i=0;i<frames.size();++i){if(elapsed<frames[i].delay)return i;elapsed-=frames[i].delay;}return 0;}
 void findCrop(){crop={};int l=width,t=height,r=-1,b=-1;for(auto&f:frames){if(width<=0||height<=0||f.pixels.size()!=size_t(width)*height)throw std::runtime_error(tr(TextId::S030));for(int y=0;y<height;++y)for(int x=0;x<width;++x)if(f.pixels[size_t(y)*width+x]>>24){l=std::min(l,x);r=std::max(r,x);t=std::min(t,y);b=std::max(b,y);}}if(r>=l&&b>=t)crop={l,t,r-l+1,b-t+1};else throw std::runtime_error(tr(TextId::S031));}
 uint32_t sample(size_t frame,double x,double y)const{
  if(frame>=frames.size()||crop.empty())return 0;
  x=std::clamp(x,double(crop.x),double(crop.x+crop.w-1));y=std::clamp(y,double(crop.y),double(crop.y+crop.h-1));
  int x0=int(x),y0=int(y),x1=std::min(x0+1,crop.x+crop.w-1),y1=std::min(y0+1,crop.y+crop.h-1);
  double fx=x-x0,fy=y-y0;const auto&p=frames[frame].pixels;uint32_t out=0;
  for(unsigned shift:{0u,8u,16u,24u}){double a=((p[size_t(y0)*width+x0]>>shift)&255)*(1-fx)+((p[size_t(y0)*width+x1]>>shift)&255)*fx;double b=((p[size_t(y1)*width+x0]>>shift)&255)*(1-fx)+((p[size_t(y1)*width+x1]>>shift)&255)*fx;out|=uint32_t(std::clamp(a*(1-fy)+b*fy+.5,0.,255.))<<shift;}return out;
 }
};
inline ImageClip trimClip(ImageClip original){
 original.findCrop();const auto crop=original.crop;const int side=std::min(256,int(std::sqrt((2.*1024*1024)/(4.*original.frames.size()))));
 double ratio=std::min(1.,double(side)/std::max(crop.w,crop.h));ImageClip out;out.width=std::max(1,int(crop.w*ratio));out.height=std::max(1,int(crop.h*ratio));out.crop={0,0,out.width,out.height};out.frames.reserve(original.frames.size());
 for(size_t index=0;index<original.frames.size();++index){ImageFrame frame;frame.delay=original.frames[index].delay;frame.pixels.resize(size_t(out.width)*out.height);
  for(int y=0;y<out.height;++y)for(int x=0;x<out.width;++x)frame.pixels[size_t(y)*out.width+x]=original.sample(index,crop.x+(x+.5)*crop.w/out.width-.5,crop.y+(y+.5)*crop.h/out.height-.5);
  out.frames.push_back(std::move(frame));
 }
 return out;
}
inline unsigned gifDelay(unsigned hundredths){return std::clamp(hundredths,4u,1000u)*10;}
inline int decodeSide(unsigned frames){if(!frames||frames>120)throw std::runtime_error(tr(TextId::S032));return std::min(256,int(std::sqrt((2.*1024*1024)/(4.*frames))));}
}
