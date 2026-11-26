#pragma once
#include "platform.h"
#include <commctrl.h>
#include <dwmapi.h>
#include <uxtheme.h>
namespace pet {
inline std::wstring controlText(HWND control){int length=GetWindowTextLengthW(control);std::wstring text(size_t(length)+1,0);GetWindowTextW(control,text.data(),length+1);text.resize(length);return text;}
inline void positionChild(HDWP&batch,HWND child,int x,int y,int width,int height){if(!child)return;constexpr UINT flags=SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOREDRAW|SWP_NOCOPYBITS;if(batch){batch=DeferWindowPos(batch,child,nullptr,x,y,width,height,flags);if(batch)return;}SetWindowPos(child,nullptr,x,y,width,height,flags);}
inline void setTextIfChanged(HWND control,const std::wstring&text){if(controlText(control)!=text)SetWindowTextW(control,text.c_str());}
struct UITheme{
 bool dark=false,contrast=false;COLORREF background=0,card=0,foreground=0,muted=0,accent=0,border=0;HBRUSH backBrush=nullptr,cardBrush=nullptr;
 ~UITheme(){if(backBrush)DeleteObject(backBrush);if(cardBrush)DeleteObject(cardBrush);}
 void sync(HWND window){
  HIGHCONTRASTW hc{sizeof(hc)};contrast=SystemParametersInfoW(SPI_GETHIGHCONTRAST,sizeof(hc),&hc,0)&&(hc.dwFlags&HCF_HIGHCONTRASTON);DWORD light=1,bytes=sizeof(light);RegGetValueW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",L"AppsUseLightTheme",RRF_RT_REG_DWORD,nullptr,&light,&bytes);dark=!contrast&&!light;
  background=contrast?GetSysColor(COLOR_WINDOW):dark?RGB(25,28,35):RGB(243,245,249);card=contrast?GetSysColor(COLOR_WINDOW):dark?RGB(37,42,52):RGB(255,255,255);foreground=contrast?GetSysColor(COLOR_WINDOWTEXT):dark?RGB(237,240,247):RGB(32,42,58);muted=contrast?foreground:dark?RGB(178,188,208):RGB(87,102,123);accent=contrast?GetSysColor(COLOR_HIGHLIGHT):dark?RGB(113,168,240):RGB(39,105,190);border=contrast?foreground:dark?RGB(75,84,101):RGB(209,217,229);
  if(backBrush)DeleteObject(backBrush);if(cardBrush)DeleteObject(cardBrush);backBrush=CreateSolidBrush(background);cardBrush=CreateSolidBrush(card);BOOL enabled=dark;DwmSetWindowAttribute(window,DWMWINDOWATTRIBUTE(20),&enabled,sizeof(enabled));int corner=contrast?1:2;DwmSetWindowAttribute(window,DWMWINDOWATTRIBUTE(33),&corner,sizeof(corner));RedrawWindow(window,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
 }
 HBRUSH color(HDC dc,bool input=false){SetTextColor(dc,foreground);SetBkColor(dc,input?card:background);return input?cardBrush:backBrush;}
 void panel(HDC dc,RECT r,int radius){auto pen=CreatePen(PS_SOLID,1,border);auto oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,cardBrush);RoundRect(dc,r.left,r.top,r.right,r.bottom,radius,radius);SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(pen);}
 void button(DRAWITEMSTRUCT*item,const std::wstring&text,bool primary=false,bool selected=false){HDC dc=item->hDC;auto previousFont=SelectObject(dc,reinterpret_cast<HFONT>(SendMessageW(item->hwndItem,WM_GETFONT,0,0)));RECT r=item->rcItem;bool disabled=(item->itemState&ODS_DISABLED)!=0;auto fill=CreateSolidBrush(primary&&!disabled?accent:selected||(item->itemState&ODS_SELECTED)?border:card);auto pen=CreatePen(PS_SOLID,1,border);auto oldPen=SelectObject(dc,pen);auto oldBrush=SelectObject(dc,fill);RoundRect(dc,r.left,r.top,r.right-1,r.bottom-1,10,10);SelectObject(dc,oldBrush);SelectObject(dc,oldPen);DeleteObject(fill);DeleteObject(pen);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,disabled?muted:primary?(contrast?GetSysColor(COLOR_HIGHLIGHTTEXT):RGB(255,255,255)):foreground);InflateRect(&r,-6,-3);DrawTextW(dc,text.c_str(),-1,&r,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);if(item->itemState&ODS_FOCUS)DrawFocusRect(dc,&r);SelectObject(dc,previousFont);}
};
}
