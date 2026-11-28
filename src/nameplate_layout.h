#pragma once
#include "desktop_ui_logic.h"
#include <cmath>
namespace pet {
struct NameplateMetrics{int font,padX,padY,diameter,guard=1;};
inline NameplateMetrics nameplateMetrics(int scale,int dpi,int fittedFont=0){
 scale=std::clamp(scale,50,300);dpi=std::max(1,dpi);
 int font=fittedFont>0?fittedFont:std::max(1,int((static_cast<long long>(std::max(1000,14*scale))*dpi+4800)/9600));
 return {font,std::max(uiPixels(6,dpi),int(std::lround(font*.65))),std::max(uiPixels(3,dpi),int(std::lround(font*.30))),std::max(uiPixels(8,dpi),font),1};
}
inline UiRect nameplateSize(double textWidth,double textHeight,const NameplateMetrics&m){return {0,0,std::max(1,int(std::ceil(textWidth)))+2*(m.padX+m.guard),std::max(m.font,int(std::ceil(textHeight)))+2*(m.padY+m.guard)};}
inline int nameplateReservedHeight(int scale,int dpi){auto m=nameplateMetrics(scale,dpi);return std::max(m.font*3/2,m.font+uiPixels(8,dpi))+2*(m.padY+m.guard);}
}
