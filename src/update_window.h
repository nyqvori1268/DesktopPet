#pragma once
#include "update.h"
#include "update_logic.h"
#include "build_config.h"
#include <commctrl.h>
#include "ui_theme.h"
namespace pet {
class PetUpdateWindow{
 UITheme theme;
 HWND window=nullptr,summary=nullptr,notes=nullptr,status=nullptr,progress=nullptr,nowButton=nullptr,laterButton=nullptr;
 HFONT font=nullptr;int dpi=96;bool working=false;std::string version;UpdateInfo shownInfo;std::wstring statusText;int percentage=0;
 std::function<void()> install,later,cancel;
 int px(int x)const{return MulDiv(x,dpi,96);}
 void layout(){
  if(!window)return;RECT r{};GetClientRect(window,&r);int w=r.right,h=r.bottom,margin=px(16),bottom=h-px(46);
  if(font)DeleteObject(font);font=CreateFontW(-px(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());
  auto place=[&](HWND control,int x,int y,int width,int height){SetWindowPos(control,nullptr,x,y,std::max(1,width),std::max(1,height),SWP_NOZORDER);SendMessageW(control,WM_SETFONT,WPARAM(font),TRUE);};
  place(summary,margin,px(14),w-2*margin,px(46));
  // At 640x480 the reading area is 306 logical pixels (> half the client).
  place(notes,margin,px(66),w-2*margin,h-px(174));
  place(progress,margin,h-px(98),w-2*margin,px(16));place(status,margin,h-px(76),w-2*margin,px(22));
  int button=px(116),gap=px(10);place(nowButton,w-margin-2*button-gap,bottom,button,px(30));place(laterButton,w-margin-button,bottom,button,px(30));
 }
 void localize(){
  if(!window)return;SetWindowTextW(window,tw(TextId::S246));SetWindowTextW(summary,wf(TextId::UpdateSummary,{wide(PET_VERSION),wide(shownInfo.version),std::to_wstring(shownInfo.size/1024)}).c_str());
  SetWindowTextW(laterButton,tw(TextId::S249));SetWindowTextW(notes,wide(readableNotes(shownInfo.notes,shownInfo.version)).c_str());
  auto translated=wide(currentSystemText(utf8(statusText)));state(percentage,translated,working);layout();
 }
 static LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
  auto self=reinterpret_cast<PetUpdateWindow*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(message==WM_NCCREATE){self=static_cast<PetUpdateWindow*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=w;SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(w,message,wp,lp);
  if(message==WM_SETTINGCHANGE||message==WM_THEMECHANGED){self->theme.sync(w);InvalidateRect(w,nullptr,TRUE);return 0;}
  if(message==WM_ERASEBKGND){RECT r{};GetClientRect(w,&r);FillRect(HDC(wp),&r,self->theme.backBrush);return 1;}
  if(message==WM_CTLCOLORSTATIC||message==WM_CTLCOLOREDIT||message==WM_CTLCOLORBTN)return LRESULT(self->theme.color(HDC(wp),message==WM_CTLCOLOREDIT));
  if(message==WM_DRAWITEM){auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);self->theme.button(item,controlText(item->hwndItem),item->CtlID==1);return TRUE;}
  if(message==LanguageChanged){self->localize();return 0;}
  if(message==WM_SIZE){self->layout();return 0;}
  if(message==WM_GETMINMAXINFO){auto limit=reinterpret_cast<MINMAXINFO*>(lp);limit->ptMinTrackSize={self->px(560),self->px(420)};return 0;}
  if(message==WM_DPICHANGED){self->dpi=HIWORD(wp);auto r=reinterpret_cast<RECT*>(lp);SetWindowPos(w,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);self->layout();return 0;}
  if(message==WM_COMMAND){switch(LOWORD(wp)){case 1:if(self->working){if(self->cancel)self->cancel();}else if(self->install)self->install();return 0;case 2:if(self->working)return 0;if(self->later)self->later();ShowWindow(w,SW_HIDE);return 0;}}
  if(message==WM_CLOSE){if(self->working&&self->cancel)self->cancel();else if(self->later)self->later();ShowWindow(w,SW_HIDE);return 0;}
  if(message==WM_DESTROY){if(self->font)DeleteObject(self->font);self->font=nullptr;self->window=nullptr;return 0;}
  return DefWindowProcW(w,message,wp,lp);
 }
public:
 ~PetUpdateWindow(){if(window)DestroyWindow(window);}
 void bind(std::function<void()>start,std::function<void()>defer,std::function<void()>stop){install=std::move(start);later=std::move(defer);cancel=std::move(stop);}
 bool dialog(MSG&message){return exists()&&IsWindowVisible(window)&&(message.hwnd==window||IsChild(window,message.hwnd))&&IsDialogMessageW(window,&message);}
 bool exists()const{return window&&IsWindow(window);}
 void show(HWND owner,HINSTANCE instance,const UpdateInfo&info){
  if(!exists()){
   WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=proc;wc.lpszClassName=L"DesktopPetUpdate";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);dpi=int(GetDpiForWindow(owner));
   RECT r{0,0,px(640),px(480)};DWORD style=WS_OVERLAPPEDWINDOW;AdjustWindowRectExForDpi(&r,style,FALSE,WS_EX_TOOLWINDOW,dpi);
   POINT cursor{};GetCursorPos(&cursor);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST),&mi);
   CreateWindowExW(WS_EX_TOOLWINDOW,L"DesktopPetUpdate",tw(TextId::S246),style,(mi.rcWork.left+mi.rcWork.right-r.right+r.left)/2,(mi.rcWork.top+mi.rcWork.bottom-r.bottom+r.top)/2,r.right-r.left,r.bottom-r.top,owner,nullptr,instance,this);
   if(!window)throw std::runtime_error(tr(TextId::S247));theme.sync(window);
   auto add=[&](const wchar_t*type,const wchar_t*text,DWORD flags,int id=0){return CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|flags,0,0,1,1,window,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);};
   summary=add(L"STATIC",L"",0);notes=add(L"EDIT",L"",WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_TABSTOP);
   SetWindowTheme(notes,L"",L"");progress=add(PROGRESS_CLASSW,L"",0);status=add(L"STATIC",L"",0);nowButton=add(L"BUTTON",tw(TextId::S248),WS_TABSTOP|BS_OWNERDRAW,1);laterButton=add(L"BUTTON",tw(TextId::S249),WS_TABSTOP|BS_OWNERDRAW,2);SendMessageW(notes,EM_SETLIMITTEXT,65536,0);layout();
  }
  shownInfo=info;bool changed=version!=info.version;version=info.version;localize();if(changed){SendMessageW(notes,EM_SETSEL,0,0);SendMessageW(notes,EM_SCROLLCARET,0,0);}
  ShowWindow(window,SW_SHOW);SetForegroundWindow(window);
 }
 void state(int percent,const std::wstring&text,bool busy){if(!exists())return;working=busy;statusText=text;percentage=percent;SendMessageW(progress,PBM_SETPOS,std::clamp(percent,0,100),0);SetWindowTextW(status,text.c_str());SetWindowTextW(nowButton,busy?tw(TextId::S254):tw(TextId::S248));EnableWindow(laterButton,!busy);}
};
}
