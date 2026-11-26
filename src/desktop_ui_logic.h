#pragma once
#include "core.h"
namespace pet {
struct UiRect{int x=0,y=0,w=0,h=0;};
inline bool overlaps(UiRect a,UiRect b){return a.x<b.x+b.w&&b.x<a.x+a.w&&a.y<b.y+b.h&&b.y<a.y+a.h;}
inline bool contains(UiRect outer,UiRect inner){return inner.w>0&&inner.h>0&&inner.x>=outer.x&&inner.y>=outer.y&&inner.x+inner.w<=outer.x+outer.w&&inner.y+inner.h<=outer.y+outer.h;}
inline int uiPixels(int logical,int dpi){return int((static_cast<long long>(logical)*dpi+48)/96);}
inline int languageTabWidth(int width,int dpi){return std::max(1,(width-2*uiPixels(4,dpi))/3);}
struct EditorLayout{UiRect tabs,instructions,input,preview,accept,cancel;};
inline EditorLayout dialogueEditorLayout(int width,int height,int dpi,int helpHeight){int m=uiPixels(16,dpi),button=uiPixels(104,dpi),gap=uiPixels(8,dpi),footer=height-uiPixels(48,dpi),top=m+uiPixels(46,dpi);return {{m,m,width-2*m,uiPixels(34,dpi)},{m,top,width-2*m,helpHeight},{m,top+helpHeight+gap,width-2*m,std::max(uiPixels(80,dpi),footer-gap-(top+helpHeight+gap))},{m,footer,button,uiPixels(32,dpi)},{width-m-2*button-gap,footer,button,uiPixels(32,dpi)},{width-m-button,footer,button,uiPixels(32,dpi)}};}
inline std::optional<UiRect> accessoryPlacement(UiRect pet,UiRect work,int width,int height,int gap,bool aboveFirst=false,std::optional<UiRect>avoid={}){
 int x=std::clamp(pet.x+(pet.w-width)/2,work.x,std::max(work.x,work.x+work.w-width));
 UiRect below{x,pet.y+pet.h+gap,width,height},above{x,pet.y-height-gap,width,height};
 if(avoid&&overlaps(below,*avoid))below.y=avoid->y+avoid->h+gap;
 std::array<UiRect,4> candidates{aboveFirst?above:below,aboveFirst?below:above,{pet.x+pet.w+gap,pet.y,width,height},{pet.x-width-gap,pet.y,width,height}};
 for(auto r:candidates)if(contains(work,r)&&!overlaps(r,pet)&&(!avoid||!overlaps(r,*avoid)))return r;
 return {};
}
inline int scalePercentage(const std::wstring&text){if(text.empty()||text.size()>3||std::any_of(text.begin(),text.end(),[](wchar_t c){return c<L'0'||c>L'9';}))throw std::runtime_error(tr(TextId::S021));int value=std::stoi(text);if(value<50||value>300)throw std::runtime_error(tr(TextId::S021));return value;}
}
