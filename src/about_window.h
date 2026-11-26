#pragma once
#include "platform.h"
#include "build_config.h"
#include <functional>
namespace pet {
class AboutWindow{
 HWND window=nullptr,icon=nullptr,heading=nullptr,version=nullptr,details=nullptr,project=nullptr,issues=nullptr,history=nullptr,update=nullptr,close=nullptr;
 HFONT font=nullptr,titleFont=nullptr;int dpi=96;std::function<void()>checkUpdate;
 int px(int x)const{return MulDiv(x,dpi,96);}
 void layout(){
  if(!window)return;RECT r{};GetClientRect(window,&r);int w=r.right,h=r.bottom,margin=px(20),gap=px(10);
  if(font)DeleteObject(font);if(titleFont)DeleteObject(titleFont);font=CreateFontW(-px(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());titleFont=CreateFontW(-px(24),0,0,0,FW_BOLD,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());
  auto position=[&](HWND child,int x,int y,int width,int height){SetWindowPos(child,nullptr,x,y,std::max(1,width),std::max(1,height),SWP_NOZORDER);SendMessageW(child,WM_SETFONT,WPARAM(font),TRUE);};
  position(icon,margin,px(20),px(48),px(48));position(heading,px(84),px(16),w-px(104),px(34));SendMessageW(heading,WM_SETFONT,WPARAM(titleFont),TRUE);position(version,px(84),px(54),w-px(104),px(24));
  position(details,margin,px(94),w-2*margin,h-px(210));int button=(w-2*margin-2*gap)/3;
  position(project,margin,h-px(104),button,px(32));position(issues,margin+button+gap,h-px(104),button,px(32));position(history,margin+2*(button+gap),h-px(104),button,px(32));
  position(update,w-margin-px(300),h-px(56),px(184),px(32));position(close,w-margin-px(106),h-px(56),px(106),px(32));
 }
 void localize(){
  if(!window)return;SetWindowTextW(window,tw(TextId::AboutTitle));SetWindowTextW(heading,L"DesktopPet");SetWindowTextW(version,wf(TextId::AboutVersion,{wide(PET_VERSION),wide(PET_ARCH)}).c_str());
  std::wstring body=std::wstring(tw(TextId::AboutDescription))+L"\r\n\r\n"+wf(TextId::AboutDeveloper,{wide(PET_DEVELOPER)})+L"\r\n"+wide(PET_COPYRIGHT)+L"\r\n\r\n"+tw(TextId::AboutLanguages)+L"\r\n\r\n"+tw(TextId::AboutLicense)+L"\r\n\r\n"+tw(TextId::AboutComponents)+L"\r\n\r\n";
  auto license=wide(PET_LICENSE_TEXT);for(auto c:license){if(c==L'\n')body+=L'\r';body+=c;}SetWindowTextW(details,body.c_str());SendMessageW(details,EM_SETSEL,0,0);SendMessageW(details,EM_SCROLLCARET,0,0);
  SetWindowTextW(project,tw(TextId::ProjectLink));SetWindowTextW(issues,tw(TextId::IssuesLink));SetWindowTextW(history,tw(TextId::ChangelogLink));SetWindowTextW(update,tw(TextId::CheckUpdates));SetWindowTextW(close,tw(TextId::Close));layout();
 }
 void link(const char*suffix){std::wstring url=wide(std::string(PET_PROJECT_URL)+suffix);if(INT_PTR(ShellExecuteW(window,L"open",url.c_str(),nullptr,nullptr,SW_SHOWNORMAL))<=32)SetWindowTextW(version,tw(TextId::LinkFailed));}
 static LRESULT CALLBACK proc(HWND w,UINT message,WPARAM wp,LPARAM lp){
  auto self=reinterpret_cast<AboutWindow*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(message==WM_NCCREATE){self=static_cast<AboutWindow*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=w;SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(w,message,wp,lp);
  if(message==LanguageChanged){self->localize();return 0;}if(message==WM_SIZE){self->layout();return 0;}
  if(message==WM_GETMINMAXINFO){auto limits=reinterpret_cast<MINMAXINFO*>(lp);limits->ptMinTrackSize={self->px(520),self->px(440)};return 0;}
  if(message==WM_DPICHANGED){self->dpi=HIWORD(wp);auto r=reinterpret_cast<RECT*>(lp);SetWindowPos(w,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);self->layout();return 0;}
  if(message==WM_COMMAND){switch(LOWORD(wp)){case 1:self->link("");return 0;case 2:ShowWindow(w,SW_HIDE);return 0;case 3:self->link("/issues");return 0;case 4:self->link("/releases");return 0;case 5:if(self->checkUpdate)self->checkUpdate();return 0;}}
  if(message==WM_CLOSE){ShowWindow(w,SW_HIDE);return 0;}if(message==WM_DESTROY){self->window=nullptr;if(self->font)DeleteObject(self->font);if(self->titleFont)DeleteObject(self->titleFont);self->font=self->titleFont=nullptr;return 0;}return DefWindowProcW(w,message,wp,lp);
 }
public:
 ~AboutWindow(){if(window)DestroyWindow(window);}
 void bind(std::function<void()>callback){checkUpdate=std::move(callback);}
 bool dialog(MSG&message){return window&&IsWindowVisible(window)&&(message.hwnd==window||IsChild(window,message.hwnd))&&IsDialogMessageW(window,&message);}
 void show(HWND owner,HINSTANCE instance){
  if(!window){WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=proc;wc.lpszClassName=L"DesktopPetAbout";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);dpi=int(GetDpiForWindow(owner));
   RECT size{0,0,px(600),px(560)};DWORD style=WS_OVERLAPPEDWINDOW;AdjustWindowRectExForDpi(&size,style,FALSE,WS_EX_TOOLWINDOW,dpi);POINT cursor{};GetCursorPos(&cursor);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST),&mi);
   CreateWindowExW(WS_EX_TOOLWINDOW,L"DesktopPetAbout",L"",style,(mi.rcWork.left+mi.rcWork.right-size.right+size.left)/2,(mi.rcWork.top+mi.rcWork.bottom-size.bottom+size.top)/2,size.right-size.left,size.bottom-size.top,owner,nullptr,instance,this);
   if(!window)throw std::runtime_error(tr(TextId::S247));auto child=[&](const wchar_t*type,DWORD flags,int id=0){return CreateWindowExW(0,type,L"",WS_CHILD|WS_VISIBLE|flags,0,0,1,1,window,reinterpret_cast<HMENU>(INT_PTR(id)),instance,nullptr);};
   icon=child(L"STATIC",SS_ICON);SendMessageW(icon,STM_SETICON,WPARAM(LoadIconW(instance,MAKEINTRESOURCEW(1))),0);heading=child(L"STATIC",0);version=child(L"STATIC",0);details=child(L"EDIT",WS_BORDER|WS_TABSTOP|WS_VSCROLL|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL);SendMessageW(details,EM_SETLIMITTEXT,32768,0);
   project=child(L"BUTTON",WS_TABSTOP,1);issues=child(L"BUTTON",WS_TABSTOP,3);history=child(L"BUTTON",WS_TABSTOP,4);update=child(L"BUTTON",WS_TABSTOP,5);close=child(L"BUTTON",WS_TABSTOP|BS_DEFPUSHBUTTON,2);
  }
  localize();ShowWindow(window,SW_SHOW);SetForegroundWindow(window);
 }
};
}
