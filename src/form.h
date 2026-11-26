#pragma once
#include "platform.h"
#include "ui_theme.h"
#include <imm.h>
#include <functional>
namespace pet {
bool localizedMessage(HWND owner,HINSTANCE instance,TextId title,const std::function<std::wstring()>&body,bool question);
struct Field{
 std::wstring label,value;std::vector<std::wstring>options;TextId labelId=TextId::Count;std::vector<TextId>optionIds;
 std::function<std::wstring()>labelFactory;int textLimit=500,multilineHeight=70;bool check=false,multiline=false,readOnly=false;HWND input=nullptr,labelWindow=nullptr;
 Field(std::wstring l,std::wstring v,bool checked=false):label(std::move(l)),value(std::move(v)),check(checked){}
 Field(TextId id,std::wstring v,bool checked=false):value(std::move(v)),labelId(id),check(checked){}
 Field(TextId id,std::wstring v,std::vector<TextId>ids):value(std::move(v)),labelId(id),optionIds(std::move(ids)){}
 std::wstring caption()const{return labelFactory?labelFactory():labelId!=TextId::Count?tw(labelId):label;}
 bool combo()const{return !optionIds.empty()||!options.empty();}
};
struct Form{
 UITheme theme;std::vector<TextId>tabIds;int selectedTab=0;HWND tabs=nullptr;std::function<void(int,int)>onTab;
 std::vector<Field>fields;std::function<bool(std::vector<Field>&)>apply;std::wstring title;TextId titleId=TextId::Count;
 std::function<std::wstring()>titleFactory;std::function<void(Language)>onLanguage;std::function<void()>preview;HWND hwnd=nullptr;HFONT font=nullptr;bool closed=false,messageMode=false,showCancel=true;TextId okId=TextId::S018,cancelId=TextId::S019;int dpi=96;
 int px(int n)const{return MulDiv(n,dpi,96);}
 void completeIME(){for(auto&field:fields)if(!field.check&&!field.combo()){HIMC context=ImmGetContext(field.input);if(context){ImmNotifyIME(context,NI_COMPOSITIONSTR,CPS_COMPLETE,0);ImmReleaseContext(field.input,context);}}}
 void localize(Language previous){
  if(onLanguage)onLanguage(previous);
  auto heading=titleFactory?titleFactory():titleId!=TextId::Count?std::wstring(tw(titleId)):title;SetWindowTextW(hwnd,heading.c_str());
  for(auto&field:fields){SetWindowTextW(field.labelWindow,field.caption().c_str());if(field.check)SetWindowTextW(field.input,tw(TextId::S017));
   if(field.combo()){int selection=int(SendMessageW(field.input,CB_GETCURSEL,0,0));if(selection<0)selection=std::stoi(field.value);SendMessageW(field.input,CB_RESETCONTENT,0,0);
    if(!field.optionIds.empty()){for(auto id:field.optionIds)SendMessageW(field.input,CB_ADDSTRING,0,LPARAM(tw(id)));}else for(auto&option:field.options)SendMessageW(field.input,CB_ADDSTRING,0,LPARAM(option.c_str()));SendMessageW(field.input,CB_SETCURSEL,selection,0);
   }
  }
  if(tabs){TabCtrl_DeleteAllItems(tabs);for(size_t i=0;i<tabIds.size();++i){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(tw(tabIds[i]));TabCtrl_InsertItem(tabs,int(i),&item);}TabCtrl_SetCurSel(tabs,selectedTab);}
  SetWindowTextW(GetDlgItem(hwnd,IDHELP),tw(TextId::Preview));SetWindowTextW(GetDlgItem(hwnd,IDOK),tw(okId));SetWindowTextW(GetDlgItem(hwnd,IDCANCEL),tw(cancelId));layout();
 }
 void layout(){
  HFONT previousFont=font;font=CreateFontW(-px(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());HDC dc=GetDC(hwnd);auto old=SelectObject(dc,font);int y=px(16);if(tabs){SetWindowPos(tabs,nullptr,px(16),y,px(528),px(34),SWP_NOZORDER);SendMessageW(tabs,WM_SETFONT,WPARAM(font),TRUE);TabCtrl_SetItemSize(tabs,px(168),px(30));y+=px(46);}
  if(messageMode){auto&field=fields.front();ShowWindow(field.labelWindow,SW_HIDE);SetWindowPos(field.input,nullptr,px(16),y,px(528),px(156),SWP_NOZORDER);SendMessageW(field.input,WM_SETFONT,WPARAM(font),TRUE);y+=px(172);}
  else for(auto&field:fields){RECT measured{0,0,px(204),0};auto caption=field.caption();DrawTextW(dc,caption.c_str(),-1,&measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);int h=field.multiline?px(field.multilineHeight):px(28),row=std::max(h,int(measured.bottom)+px(4));
   SetWindowPos(field.labelWindow,nullptr,px(16),y+px(3),px(204),row,SWP_NOZORDER);SetWindowPos(field.input,nullptr,px(232),y,px(312),field.combo()?px(220):h,SWP_NOZORDER);SendMessageW(field.labelWindow,WM_SETFONT,WPARAM(font),TRUE);SendMessageW(field.input,WM_SETFONT,WPARAM(font),TRUE);y+=row+px(10);
  }
  SelectObject(dc,old);ReleaseDC(hwnd,dc);for(int id:{IDHELP,IDOK,IDCANCEL}){if((id==IDCANCEL&&!showCancel)||(id==IDHELP&&!preview)){ShowWindow(GetDlgItem(hwnd,id),SW_HIDE);continue;}SetWindowPos(GetDlgItem(hwnd,id),nullptr,px(id==IDHELP?16:id==IDOK?328:440),y+px(4),px(104),px(32),SWP_NOZORDER);SendDlgItemMessageW(hwnd,id,WM_SETFONT,WPARAM(font),TRUE);}
  if(previousFont)DeleteObject(previousFont);
  RECT size{0,0,px(560),y+px(52)};AdjustWindowRectExForDpi(&size,WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_DLGMODALFRAME,dpi);SetWindowPos(hwnd,nullptr,0,0,size.right-size.left,size.bottom-size.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
 }
 static LRESULT CALLBACK proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
  auto self=reinterpret_cast<Form*>(GetWindowLongPtrW(window,GWLP_USERDATA));if(message==WM_NCCREATE){self=static_cast<Form*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->hwnd=window;SetWindowLongPtrW(window,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(window,message,wp,lp);
  if(message==WM_SETTINGCHANGE||message==WM_THEMECHANGED){self->theme.sync(window);InvalidateRect(window,nullptr,TRUE);return 0;}
  if(message==WM_ERASEBKGND){RECT r{};GetClientRect(window,&r);FillRect(HDC(wp),&r,self->theme.backBrush);return 1;}
  if(message==WM_CTLCOLORSTATIC||message==WM_CTLCOLORBTN||message==WM_CTLCOLOREDIT||message==WM_CTLCOLORLISTBOX)return LRESULT(self->theme.color(HDC(wp),message==WM_CTLCOLOREDIT||message==WM_CTLCOLORLISTBOX));
  if(message==WM_DRAWITEM){auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);if(item->hwndItem==self->tabs){wchar_t text[100]{};TCITEMW tab{};tab.mask=TCIF_TEXT;tab.pszText=text;tab.cchTextMax=100;TabCtrl_GetItem(self->tabs,item->itemID,&tab);self->theme.button(item,text,false,int(item->itemID)==self->selectedTab);}else self->theme.button(item,controlText(item->hwndItem),item->CtlID==IDOK);return TRUE;}
  if(message==WM_NOTIFY&&self->tabs){auto header=reinterpret_cast<NMHDR*>(lp);if(header->hwndFrom==self->tabs&&header->code==TCN_SELCHANGE){self->completeIME();int next=TabCtrl_GetCurSel(self->tabs);if(next>=0){if(self->onTab)self->onTab(self->selectedTab,next);self->selectedTab=next;InvalidateRect(self->tabs,nullptr,TRUE);}return 0;}}
  if(message==LanguageChanged){self->localize(Language(wp));return 0;}
  if(message==WM_COMMAND){if(LOWORD(wp)==IDHELP&&self->preview){try{self->preview();}catch(const std::exception&e){auto text=std::string(e.what());localizedMessage(window,GetModuleHandleW(nullptr),TextId::S016,[text]{return wide(currentSystemText(text));},false);}return 0;}if(LOWORD(wp)==IDOK){self->completeIME();for(auto&field:self->fields){if(field.check)field.value=SendMessageW(field.input,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0";else if(field.combo())field.value=std::to_wstring(SendMessageW(field.input,CB_GETCURSEL,0,0));else{int n=GetWindowTextLengthW(field.input);std::wstring value(size_t(n)+1,0);GetWindowTextW(field.input,value.data(),n+1);value.resize(n);field.value=std::move(value);}}try{if(self->apply(self->fields))DestroyWindow(window);}catch(const std::exception&e){auto message=std::string(e.what());localizedMessage(window,GetModuleHandleW(nullptr),TextId::S016,[message]{return wide(currentSystemText(message));},false);}return 0;}if(LOWORD(wp)==IDCANCEL){DestroyWindow(window);return 0;}}
  if(message==WM_CLOSE){DestroyWindow(window);return 0;}if(message==WM_DESTROY){self->closed=true;return 0;}
  if(message==WM_DPICHANGED){self->dpi=HIWORD(wp);auto r=reinterpret_cast<RECT*>(lp);SetWindowPos(window,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);self->layout();return 0;}return DefWindowProcW(window,message,wp,lp);
 }
 void show(HWND owner,HINSTANCE instance){
  static bool registered=false;if(!registered){WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=proc;wc.lpszClassName=L"PetForm";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);registered=true;}
  dpi=int(owner?GetDpiForWindow(owner):GetDpiForSystem());if(dpi<=0)dpi=96;POINT cursor{};GetCursorPos(&cursor);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST),&mi);
  hwnd=CreateWindowExW(WS_EX_DLGMODALFRAME,L"PetForm",L"",WS_CAPTION|WS_SYSMENU,mi.rcWork.left+px(24),mi.rcWork.top+px(24),px(560),px(400),owner,nullptr,instance,this);
  int id=100;for(auto&field:fields){field.labelWindow=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,1,1,hwnd,nullptr,instance,nullptr);DWORD style=WS_CHILD|WS_VISIBLE|WS_TABSTOP;const wchar_t*cls=L"EDIT";
   if(field.check){cls=L"BUTTON";style|=BS_AUTOCHECKBOX;}else if(field.combo()){cls=L"COMBOBOX";style|=CBS_DROPDOWNLIST|WS_VSCROLL;}else style|=WS_BORDER|(field.multiline?ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL:ES_AUTOHSCROLL);
   field.input=CreateWindowExW(0,cls,field.check?L"":field.value.c_str(),style,0,0,1,1,hwnd,reinterpret_cast<HMENU>(INT_PTR(id++)),instance,nullptr);
   if(field.check)SendMessageW(field.input,BM_SETCHECK,field.value==L"1"?BST_CHECKED:BST_UNCHECKED,0);else if(!field.combo()){SendMessageW(field.input,EM_SETLIMITTEXT,field.readOnly?65536:field.multiline?field.textLimit:120,0);if(field.readOnly)SendMessageW(field.input,EM_SETREADONLY,TRUE,0);}
  }
  CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,hwnd,reinterpret_cast<HMENU>(IDOK),instance,nullptr);CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,hwnd,reinterpret_cast<HMENU>(IDCANCEL),instance,nullptr);
  CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,hwnd,reinterpret_cast<HMENU>(IDHELP),instance,nullptr);
  if(!tabIds.empty())tabs=CreateWindowW(WC_TABCONTROLW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TCS_BUTTONS|TCS_FLATBUTTONS|TCS_FIXEDWIDTH|TCS_OWNERDRAWFIXED,0,0,1,1,hwnd,nullptr,instance,nullptr);
  theme.sync(hwnd);for(auto&field:fields)SetWindowTheme(field.input,L"",L"");
  // Creation must not reinterpret values in an old UI language.
  localize(uiLanguage());RECT r{};GetWindowRect(hwnd,&r);int w=r.right-r.left,h=r.bottom-r.top;SetWindowPos(hwnd,nullptr,std::max(int(mi.rcWork.left),int((mi.rcWork.left+mi.rcWork.right-w)/2)),std::max(int(mi.rcWork.top),int((mi.rcWork.top+mi.rcWork.bottom-h)/2)),0,0,SWP_NOSIZE|SWP_NOZORDER);
  EnableWindow(owner,FALSE);ShowWindow(hwnd,SW_SHOW);SetForegroundWindow(hwnd);SetFocus(fields.empty()?GetDlgItem(hwnd,IDOK):fields[0].input);MSG message{};while(!closed){int result=GetMessageW(&message,nullptr,0,0);if(result<=0){if(!result)PostQuitMessage(int(message.wParam));break;}if(!IsDialogMessageW(hwnd,&message)){TranslateMessage(&message);DispatchMessageW(&message);}}
  EnableWindow(owner,TRUE);if(font){DeleteObject(font);font=nullptr;}
 }
};
inline int number(const std::wstring&s,int lo,int hi){size_t n=0;int value;try{value=std::stoi(s,&n);}catch(...){throw std::runtime_error(tr(TextId::S020));}if(n!=s.size()||value<lo||value>hi)throw std::runtime_error(tr(TextId::S021));return value;}
inline bool localizedMessage(HWND owner,HINSTANCE instance,TextId title,const std::function<std::wstring()>&body,bool question){
 Form dialog;dialog.titleId=title;dialog.messageMode=true;dialog.showCancel=question;dialog.okId=question?TextId::Yes:TextId::Close;dialog.cancelId=TextId::No;
 dialog.fields.emplace_back(std::wstring{},body());dialog.fields[0].multiline=dialog.fields[0].readOnly=true;bool accepted=false;
 dialog.onLanguage=[&](Language){SetWindowTextW(dialog.fields[0].input,body().c_str());};dialog.apply=[&](auto&){accepted=true;return true;};dialog.show(owner,instance);return accepted;
}

}
