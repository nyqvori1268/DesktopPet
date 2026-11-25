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
 void extraEmote(Action action,double t){
  if(action==Action::Pet||action==Action::Happy||action==Action::Shy){heart(3,3-int(std::sin(t*3)*2));heart(39,4);}
  if(action==Action::Sleep){box(36,4,6,1,0xffffefd9);box(40,5,1,1,0xffffefd9);box(38,6,2,1,0xffffefd9);box(36,7,1,1,0xffffefd9);box(36,8,6,1,0xffffefd9);}
  if(action==Action::Surprise||action==Action::Wake){box(42,10,2,5,0xffeaa3b3);box(42,17,2,2,0xffeaa3b3);}
 }
 void bird(int coat,Action action,double t){
  pixels.fill(0);coat=std::clamp(coat,0,4);bool crest=coat>=3,sleep=action==Action::Sleep;
  const uint32_t outline=0xff534954,ink=0xff423d4b,beak=0xffe7bb74;
  const uint32_t colors[]={0xffa6c586,0xff99c5db,0xffead88f,0xffa8a7ae,0xffeee0af};
  const uint32_t wings[]={0xff83a877,0xff78a5bf,0xffd0bc76,0xff85838f,0xffcfc492};
  uint32_t fur=colors[coat],wing=wings[coat];int breath=int(std::sin(t*(sleep?1.6:2.5))>.6);
  bool flap=action==Action::Happy||action==Action::Pet||action==Action::Stretch||action==Action::Remind;
  int spread=flap?2+int((std::sin(t*8)+1)*2):0;
  // Long tail feathers, two wings and a hooked beak distinguish every bird.
  box(21,33,4,13,outline);box(22,34,2,12,wing);box(26,33,3,12,outline);box(26,34,2,10,fur);
  oval(13,22-breath,22,21+breath,outline);oval(14,23-breath,20,19+breath,fur);
  oval(10-spread,25-spread,9+spread,13,outline);oval(11-spread,26-spread,7+spread,11,wing);
  oval(30,25-spread,8+spread,13,outline);oval(31,26-spread,6+spread,11,wing);
  box(12-spread,30-spread,5,1,outline);box(13-spread,33-spread,5,1,outline);box(32,30-spread,5,1,outline);
  int tilt=(action==Action::Shy||action==Action::Happy)?int(std::sin(t*3)*2):0;
  int hy=(sleep?17:10)-breath,hx=12+tilt;
  if(crest){box(hx+9,hy-7,2,9,outline);box(hx+12,hy-9,2,10,outline);box(hx+15,hy-6,2,7,outline);box(hx+10,hy-6,1,7,beak);}
  oval(hx,hy,25,23,outline);oval(hx+1,hy+1,23,21,crest?(coat==3?0xffe6dfad:0xfff5e7b6):(coat==1?0xffeeeadd:0xffefe1a1));
  if(!crest){box(hx+4,hy+3,5,1,wing);box(hx+15,hy+3,5,1,wing);box(hx+2,hy+6,4,1,wing);box(hx+19,hy+6,4,1,wing);}
  bool shut=sleep||action==Action::Blink||action==Action::Pet;
  if(shut){box(hx+5,hy+10,4,1,ink);box(hx+16,hy+10,4,1,ink);}
  else{box(hx+6,hy+9,2,4,ink);box(hx+17,hy+9,2,4,ink);dot(hx+6,hy+9,0xffffefd9);dot(hx+17,hy+9,0xffffefd9);}
  if(action==Action::Angry){box(hx+4,hy+7,4,1,ink);dot(hx+8,hy+8,ink);box(hx+17,hy+7,4,1,ink);dot(hx+16,hy+8,ink);}
  oval(hx+10,hy+12,6,5,outline);box(hx+11,hy+12,4,3,beak);box(hx+12,hy+15,2,3,beak);
  if(crest){oval(hx+2,hy+14,6,4,0xffe7a079);oval(hx+18,hy+14,6,4,0xffe7a079);}
  else {box(hx+3,hy+15,3,2,0xff8c9bc5);box(hx+20,hy+15,3,2,0xff8c9bc5);}
  box(16,41,2,4,beak);box(29,41,2,4,beak);box(14,44,7,1,outline);box(27,44,7,1,outline);
  extraEmote(action,t);
 }
 void panda(Action action,double t){
  pixels.fill(0);const uint32_t outline=0xff534954,black=0xff625e68,white=0xfff4eee3,ink=0xff423d4b;
  bool sleep=action==Action::Sleep;int breath=int(std::sin(t*(sleep?1.5:2.5))>.6);bool belly=action==Action::Belly;
  oval(10,24-breath,29,21+breath,outline);oval(11,25-breath,27,19+breath,white);
  oval(7,30,10,12,black);oval(32,30,10,12,black);oval(12,39,10,7,black);oval(28,39,10,7,black);
  if(belly){oval(11,22,27,23,white);oval(10,24,8,7,black);oval(32,24,8,7,black);oval(15,32,18,11,0xffe9dece);}
  if(action==Action::Stretch){oval(3,35,16,7,black);oval(30,35,15,7,black);}
  int hy=(sleep?17:11)-breath;
  oval(9,hy-5,10,10,outline);oval(10,hy-4,8,8,black);oval(30,hy-5,10,10,outline);oval(31,hy-4,8,8,black);
  oval(8,hy,33,26,outline);oval(9,hy+1,31,24,white);oval(13,hy+8,9,11,black);oval(27,hy+8,9,11,black);
  bool shut=sleep||action==Action::Blink||action==Action::Pet;
  if(shut){box(15,hy+12,5,1,white);box(29,hy+12,5,1,white);}
  else{box(17,hy+11,2,4,white);box(30,hy+11,2,4,white);dot(17,hy+13,ink);dot(30,hy+13,ink);}
  if(action==Action::Angry){box(14,hy+9,5,1,ink);box(30,hy+9,5,1,ink);}
  oval(22,hy+17,5,3,ink);dot(24,hy+20,ink);box(22,hy+21,5,1,ink);
  if(action==Action::Yawn)oval(22,hy+19,5,5,ink);
  if(action==Action::Happy||action==Action::Pet){box(34,24,3,20,0xff98b77d);box(34,28,3,1,outline);box(34,36,3,1,outline);oval(37,25,8,4,0xffa9c58a);oval(28,31,8,4,0xffa9c58a);oval(30,35,8,6,black);}
  if(action==Action::Shy){box(12,hy+18,5,2,0xffeaa3b3);box(32,hy+18,5,2,0xffeaa3b3);}
  extraEmote(action,t);
 }
 void make(int species,int coat,Action action,double t){
  if(species==3){bird(coat,action,t);return;}if(species==4){panda(action,t);return;}
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
