#pragma once
#include "update.h"
#include "update_logic.h"
#include "build_config.h"
#include <commctrl.h>
namespace pet {
class PetUpdateWindow{
 HWND window=nullptr,summary=nullptr,notes=nullptr,status=nullptr,progress=nullptr,nowButton=nullptr,laterButton=nullptr,skipButton=nullptr;
 HFONT font=nullptr;int dpi=96;bool working=false;std::string version;
 std::function<void()> install,later,skip,cancel;
 int px(int x)const{return MulDiv(x,dpi,96);}
 void layout(){
  if(!window)return;RECT r{};GetClientRect(window,&r);int w=r.right,h=r.bottom,margin=px(16),bottom=h-px(46);
  if(font)DeleteObject(font);font=CreateFontW(-px(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Microsoft JhengHei UI");
  auto place=[&](HWND control,int x,int y,int width,int height){SetWindowPos(control,nullptr,x,y,std::max(1,width),std::max(1,height),SWP_NOZORDER);SendMessageW(control,WM_SETFONT,WPARAM(font),TRUE);};
  place(summary,margin,px(14),w-2*margin,px(46));
  // At 640x480 the reading area is 306 logical pixels (> half the client).
  place(notes,margin,px(66),w-2*margin,h-px(174));
  place(progress,margin,h-px(98),w-2*margin,px(16));place(status,margin,h-px(76),w-2*margin,px(22));
  int button=px(116),gap=px(10);place(nowButton,w-margin-3*button-2*gap,bottom,button,px(30));place(laterButton,w-margin-2*button-gap,bottom,button,px(30));place(skipButton,w-margin-button,bottom,button,px(30));
 }
 static LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
  auto self=reinterpret_cast<PetUpdateWindow*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(message==WM_NCCREATE){self=static_cast<PetUpdateWindow*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=w;SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(w,message,wp,lp);
  if(message==WM_SIZE){self->layout();return 0;}
  if(message==WM_GETMINMAXINFO){auto limit=reinterpret_cast<MINMAXINFO*>(lp);limit->ptMinTrackSize={self->px(560),self->px(420)};return 0;}
  if(message==WM_DPICHANGED){self->dpi=HIWORD(wp);auto r=reinterpret_cast<RECT*>(lp);SetWindowPos(w,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);self->layout();return 0;}
  if(message==WM_COMMAND){switch(LOWORD(wp)){case 1:if(self->working){if(self->cancel)self->cancel();}else if(self->install)self->install();return 0;case 2:if(self->working)return 0;if(self->later)self->later();ShowWindow(w,SW_HIDE);return 0;case 3:if(self->working)return 0;if(self->skip)self->skip();ShowWindow(w,SW_HIDE);return 0;}}
  if(message==WM_CLOSE){if(self->working&&self->cancel)self->cancel();else if(self->later)self->later();ShowWindow(w,SW_HIDE);return 0;}
  if(message==WM_DESTROY){if(self->font)DeleteObject(self->font);self->font=nullptr;self->window=nullptr;return 0;}
  return DefWindowProcW(w,message,wp,lp);
 }
public:
 ~PetUpdateWindow(){if(window)DestroyWindow(window);}
 void bind(std::function<void()>start,std::function<void()>defer,std::function<void()>ignore,std::function<void()>stop){install=std::move(start);later=std::move(defer);skip=std::move(ignore);cancel=std::move(stop);}
 bool dialog(MSG&message){return exists()&&IsWindowVisible(window)&&(message.hwnd==window||IsChild(window,message.hwnd))&&IsDialogMessageW(window,&message);}
 bool exists()const{return window&&IsWindow(window);}
 void show(HWND owner,HINSTANCE instance,const UpdateInfo&info){
  if(!exists()){
   WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=proc;wc.lpszClassName=L"DesktopPetUpdate";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);dpi=int(GetDpiForWindow(owner));
   RECT r{0,0,px(640),px(480)};DWORD style=WS_OVERLAPPEDWINDOW;AdjustWindowRectExForDpi(&r,style,FALSE,WS_EX_TOOLWINDOW,dpi);
   POINT cursor{};GetCursorPos(&cursor);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST),&mi);
   CreateWindowExW(WS_EX_TOOLWINDOW,L"DesktopPetUpdate",L"DesktopPet 軟體更新",style,(mi.rcWork.left+mi.rcWork.right-r.right+r.left)/2,(mi.rcWork.top+mi.rcWork.bottom-r.bottom+r.top)/2,r.right-r.left,r.bottom-r.top,owner,nullptr,instance,this);
   if(!window)throw std::runtime_error("無法建立更新視窗");
   auto add=[&](const wchar_t*type,const wchar_t*text,DWORD flags,int id=0){return CreateWindowExW(0,type,text,WS_CHILD|WS_VISIBLE|flags,0,0,1,1,window,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);};
   summary=add(L"STATIC",L"",0);notes=add(L"EDIT",L"",WS_BORDER|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_TABSTOP);
   progress=add(PROGRESS_CLASSW,L"",0);status=add(L"STATIC",L"",0);nowButton=add(L"BUTTON",L"立即更新",WS_TABSTOP|BS_DEFPUSHBUTTON,1);laterButton=add(L"BUTTON",L"稍後更新",WS_TABSTOP,2);skipButton=add(L"BUTTON",L"跳過此版本",WS_TABSTOP,3);SendMessageW(notes,EM_SETLIMITTEXT,65536,0);layout();
  }
  std::wstring text=L"目前版本："+wide(PET_VERSION)+L"    最新版本："+wide(info.version)+L"    下載容量："+std::to_wstring(info.size/1024)+L" KB";SetWindowTextW(summary,text.c_str());
  if(version!=info.version){version=info.version;SetWindowTextW(notes,wide(readableNotes(info.notes,info.version)).c_str());SendMessageW(notes,EM_SETSEL,0,0);SendMessageW(notes,EM_SCROLLCARET,0,0);}
  ShowWindow(window,SW_SHOW);SetForegroundWindow(window);
 }
 void state(int percent,const std::wstring&text,bool busy){if(!exists())return;working=busy;SendMessageW(progress,PBM_SETPOS,std::clamp(percent,0,100),0);SetWindowTextW(status,text.c_str());SetWindowTextW(nowButton,busy?L"取消更新":L"立即更新");EnableWindow(laterButton,!busy);EnableWindow(skipButton,!busy);}
};
}
