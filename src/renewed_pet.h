#pragma once
#include "core.h"
#include "ocean.h"
namespace pet {
// These are existing config IDs, not new species. All five coats remain distinct.
inline int renewedAppearance(int pet,int coat){return coat<0?-1:pet==0&&coat<5?coat:pet==1&&coat<5?5+coat:pet==3&&coat<5?10+coat:pet==4&&coat==0?15:-1;}
class RenewedPet{
 ImageClip image;int selected=-1;Action active=Action::Idle;uint64_t started=0;
public:
 int identity()const{return selected;}
 void clear(){image=ImageClip{};selected=-1;active=Action::Idle;started=0;}
 void select(int pet,int coat,const std::vector<unsigned char>&bytes){int next=renewedAppearance(pet,coat);if(next<0)throw std::runtime_error(tr(TextId::BuiltinResourceError));ImageClip decoded;try{decoded=decodeOcean(bytes);}catch(...){throw std::runtime_error(tr(TextId::BuiltinResourceError));}if(decoded.width!=128||decoded.height!=128)throw std::runtime_error(tr(TextId::BuiltinResourceError));image=std::move(decoded);selected=next;active=Action::Idle;started=0;}
 const ImageClip&clip()const{return image;}
 size_t frame(Action action,uint64_t time){
  if(action!=active){active=action;started=time;}
  auto elapsed=time-started;
  switch(action){
   case Action::Blink:case Action::Shy:return 1;
   case Action::Sleep:return 2;
   case Action::Yawn:return elapsed<180?1:elapsed<1800?3:1;
   case Action::Stretch:return elapsed<120?0:4;
   case Action::Belly:return 5; // Birds' sixth pose is a wing flap, never belly-up.
   case Action::Wake:return elapsed<180?1:elapsed<600?3:elapsed<1200?4:0;
   case Action::Happy:case Action::Pet:return selected>=10&&selected<15?((elapsed/240)%2?4:5):1;
   default:return time%5200<150?1:0;
  }
 }
};
}
