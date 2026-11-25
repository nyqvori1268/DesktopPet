#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <algorithm>
namespace pet {
enum class Action{Idle,Blink,Sleep,Wake,Yawn,Stretch,Belly,Happy,Shy,Angry,Surprise,Pet,Drag,Remind};
struct Sprite{
 std::array<uint32_t,48*48> pixels{};
 void dot(int x,int y,uint32_t c){if(x>=0&&x<48&&y>=0&&y<48)pixels[y*48+x]=c;}
 void box(int x,int y,int w,int h,uint32_t c){for(int j=y;j<y+h;++j)for(int i=x;i<x+w;++i)dot(i,j,c);}
 void oval(int x,int y,int w,int h,uint32_t c){for(int j=0;j<h;++j)for(int i=0;i<w;++i){double a=(i+.5-w/2.)/(w/2.),b=(j+.5-h/2.)/(h/2.);if(a*a+b*b<=1)dot(x+i,y+j,c);}}
 void heart(int x,int y){box(x,y,2,2,0xffef88a4);box(x+3,y,2,2,0xffef88a4);box(x,y+2,5,2,0xffef88a4);box(x+1,y+4,3,1,0xffef88a4);dot(x+2,y+5,0xffef88a4);}
 void make(int species,int coat,Action action,double t){
  pixels.fill(0);const uint32_t outline=0xff534954,cream=0xffffefd9,ink=0xff423d4b,pink=0xffeaa3b3;
  static constexpr uint32_t cat[]={0xffe9ad70,0xfffaf0de,0xfff3eee8,0xff696574,0xffaa9c91};
  static constexpr uint32_t dog[]={0xffdca06c,0xff686574,0xffeee9df,0xffdaa477,0xffd9c3a0};
  uint32_t fur=species?dog[coat]:cat[coat];int breath=int(std::sin(t*2.5)>.6);bool sleep=action==Action::Sleep;bool belly=action==Action::Belly;
  int tail=int(std::sin(t*(sleep?1:6))*2);
  // Cat has long upright tail; dogs have a curled or short tail and a broad muzzle.
  if(!species){oval(35+tail,23,7,18,outline);oval(36+tail,23,5,16,fur);}
  else if(coat==3){oval(36,32+tail,7,6,outline);oval(37,33+tail,5,4,cream);}
  else {oval(34+tail,25,10,11,outline);oval(35+tail,26,8,9,fur);oval(37+tail,28,4,5,cream);}
  oval(10,25-breath,29,18+breath,outline);oval(11,26-breath,27,16+breath,fur);
  oval(17,29-breath,16,13,cream);box(13,39,8,4,outline);box(14,39,6,3,fur);box(29,39,8,4,outline);box(30,39,6,3,fur);
  if(action==Action::Stretch){box(6,39,14,4,fur);box(29,39,13,4,fur);}
  if(belly){oval(13,21,25,22,outline);oval(14,22,23,20,cream);oval(12,23,6,6,fur);oval(33,23,6,6,fur);}
  int hy=sleep?21:12-breath;
  if(species&&coat==4){oval(7,hy-1,10,21,outline);oval(8,hy,8,19,fur);oval(32,hy-1,10,21,outline);oval(33,hy,8,19,fur);}
  else {for(int j=0;j<11;++j){int w=species?3+j/2:1+j/2;box(12-j/3,hy-7+j,w,1,outline);box(35+j/3-w,hy-7+j,w,1,outline);}box(13,hy-2,5,7,pink);box(30,hy-2,5,7,pink);}
  oval(9,hy,31,23,outline);oval(10,hy+1,29,21,fur);
  if(!species&&coat==1){oval(10,hy+1,12,10,0xffdca06c);oval(30,hy+2,8,11,0xff70616a);}
  if((!species&&(coat==0||coat==4))||(species&&coat==1)){box(22,hy+1,2,4,outline);box(26,hy+1,2,3,outline);if(!species){box(10,hy+12,5,2,outline);box(34,hy+12,5,2,outline);}}
  if(species){oval(14,hy+11,22,10,cream);if(coat==1){box(16,hy+6,4,2,0xffdcac7c);box(29,hy+6,4,2,0xffdcac7c);}}
  bool shut=sleep||action==Action::Blink||action==Action::Happy||action==Action::Pet;
  if(shut){box(16,hy+10,4,1,ink);box(29,hy+10,4,1,ink);if(!sleep){dot(17,hy+9,ink);dot(30,hy+9,ink);}}
  else if(action==Action::Angry){box(16,hy+9,4,2,ink);box(29,hy+9,4,2,ink);dot(16,hy+8,ink);dot(32,hy+8,ink);}
  else {int h=action==Action::Surprise?5:3;box(17,hy+9,2,h,ink);box(30,hy+9,2,h,ink);dot(17,hy+9,cream);dot(30,hy+9,cream);}
  box(23,hy+13,species?4:3,2,species?ink:pink);dot(24,hy+15,ink);
  if(action==Action::Yawn)oval(22,hy+15,6,6,ink);else{dot(23,hy+16,ink);dot(25,hy+16,ink);}
  if(action==Action::Shy||action==Action::Happy||action==Action::Pet){box(13,hy+14,4,2,pink);box(33,hy+14,4,2,pink);}
  if(!species){box(7,hy+14,5,1,outline);box(37,hy+14,5,1,outline);}
  if(action==Action::Pet||action==Action::Happy){heart(4,4-int(std::sin(t*3)*2));heart(38,5);}
  if(sleep){box(37,6,6,1,cream);box(41,7,1,1,cream);box(39,8,2,1,cream);box(37,9,1,1,cream);box(37,10,6,1,cream);}
  if(action==Action::Wake||action==Action::Surprise){box(42,8,2,5,pink);box(42,15,2,2,pink);}
 }
};
struct Spring{
 double x=1,y=1,vx=0,vy=0;
 void step(double dt){dt=std::clamp(dt,0.,.04);vx+=(-140*(x-1)-17*vx)*dt;vy+=(-140*(y-1)-17*vy)*dt;x+=vx*dt;y+=vy*dt;}
 void drag(double dx,double dy){x=std::clamp(1.+std::abs(dx)*.003-std::abs(dy)*.001,.7,1.35);y=std::clamp(1.+std::abs(dy)*.003-std::abs(dx)*.001,.7,1.35);vx=vy=0;}
 bool moving()const{return std::abs(x-1)+std::abs(y-1)+std::abs(vx)+std::abs(vy)>.005;}
};
}
