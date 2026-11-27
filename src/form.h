#pragma once
#include "platform.h"
#include "ui_theme.h"
#include "desktop_ui_logic.h"
#include <imm.h>
#include <windowsx.h>
#include <functional>
namespace pet {
bool localizedMessage(HWND owner,HINSTANCE instance,TextId title,const std::function<std::wstring()>&body,bool question);
struct Field{
 std::wstring label,value;std::vector<std::wstring>options;TextId labelId=TextId::Count;std::vector<TextId>optionIds;
 std::vector<std::pair<TextId,std::function<void()>>>actions;std::vector<HWND>actionWindows;
 std::function<std::wstring()>labelFactory;int textLimit=500,multilineHeight=70;bool check=false,multiline=false,readOnly=false;HWND input=nullptr,labelWindow=nullptr;
 Field(std::wstring l,std::wstring v,bool checked=false):label(std::move(l)),value(std::move(v)),check(checked){}
 Field(TextId id,std::wstring v,bool checked=false):value(std::move(v)),labelId(id),check(checked){}
 Field(TextId id,std::wstring v,std::vector<TextId>ids):value(std::move(v)),labelId(id),optionIds(std::move(ids)){}
 std::wstring caption()const{return labelFactory?labelFactory():labelId!=TextId::Count?tw(labelId):label;}
 bool combo()const{return !optionIds.empty()||!options.empty();}
};
struct Form{
 bool multiEditor=false;HWND contentWindow=nullptr,helpWindow=nullptr;TextId helpId=TextId::Count;int contentScroll=0,contentWheel=0,activeField=0;
 bool nameMode=false;HWND nameCounter=nullptr;
 bool resizable=false;int logicalWidth=640,logicalHeight=480;
 UITheme theme;std::vector<TextId>tabIds;int selectedTab=0;HWND tabs=nullptr;std::function<void(int,int)>onTab;
 std::vector<Field>fields;std::function<bool(std::vector<Field>&)>apply;std::wstring title;TextId titleId=TextId::Count;
 std::function<std::wstring()>titleFactory;std::function<void(Language)>onLanguage;std::function<void()>preview;HWND hwnd=nullptr;HFONT font=nullptr;bool closed=false,messageMode=false,showCancel=true;TextId okId=TextId::S018,cancelId=TextId::S019;int dpi=96;
 int px(int n)const{return MulDiv(n,dpi,96);}
 void updateNameCounter(){if(!nameMode||!nameCounter||fields.empty()||!fields[0].input)return;NameMetrics metrics;try{metrics=petNameMetrics(utf8(controlText(fields[0].input)));}catch(const std::exception&){/* Incomplete UTF-16/IME input stays editable, never enters config. */}SetWindowTextW(nameCounter,wf(TextId::NameRemaining,{std::to_wstring(12-metrics.units)}).c_str());EnableWindow(GetDlgItem(hwnd,IDOK),metrics.valid);}
 void completeIME(){for(auto&field:fields)if(!field.check&&!field.combo()){HIMC context=ImmGetContext(field.input);if(context){ImmNotifyIME(context,NI_COMPOSITIONSTR,CPS_COMPLETE,0);ImmReleaseContext(field.input,context);}}}
 void localize(Language previous){
  if(onLanguage)onLanguage(previous);
  auto heading=titleFactory?titleFactory():titleId!=TextId::Count?std::wstring(tw(titleId)):title;SetWindowTextW(hwnd,heading.c_str());
  for(auto&field:fields){SetWindowTextW(field.labelWindow,field.caption().c_str());for(size_t i=0;i<field.actionWindows.size();++i)SetWindowTextW(field.actionWindows[i],tw(field.actions[i].first));if(field.check)SetWindowTextW(field.input,tw(TextId::S017));
   if(field.combo()){int selection=int(SendMessageW(field.input,CB_GETCURSEL,0,0));if(selection<0)selection=std::stoi(field.value);SendMessageW(field.input,CB_RESETCONTENT,0,0);
    if(!field.optionIds.empty()){for(auto id:field.optionIds)SendMessageW(field.input,CB_ADDSTRING,0,LPARAM(tw(id)));}else for(auto&option:field.options)SendMessageW(field.input,CB_ADDSTRING,0,LPARAM(option.c_str()));SendMessageW(field.input,CB_SETCURSEL,selection,0);
   }
  }
  if(tabs){TabCtrl_DeleteAllItems(tabs);for(size_t i=0;i<tabIds.size();++i){TCITEMW item{};item.mask=TCIF_TEXT;item.pszText=const_cast<wchar_t*>(tw(tabIds[i]));TabCtrl_InsertItem(tabs,int(i),&item);}TabCtrl_SetCurSel(tabs,selectedTab);}
  SetWindowTextW(GetDlgItem(hwnd,IDHELP),tw(TextId::Preview));SetWindowTextW(GetDlgItem(hwnd,IDOK),tw(okId));SetWindowTextW(GetDlgItem(hwnd,IDCANCEL),tw(cancelId));layout();
 }
 void layout(){
  if(multiEditor){layoutMulti();return;}
  if(resizable&&fields.size()==1&&tabs){layoutEditor();return;}
  HFONT previousFont=font;font=CreateFontW(-px(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());HDC dc=GetDC(hwnd);auto old=SelectObject(dc,font);int y=px(16);if(tabs){SetWindowPos(tabs,nullptr,px(16),y,px(528),px(34),SWP_NOZORDER);SendMessageW(tabs,WM_SETFONT,WPARAM(font),TRUE);TabCtrl_SetItemSize(tabs,px(168),px(30));y+=px(46);}
  if(messageMode){auto&field=fields.front();ShowWindow(field.labelWindow,SW_HIDE);SetWindowPos(field.input,nullptr,px(16),y,px(528),px(156),SWP_NOZORDER);SendMessageW(field.input,WM_SETFONT,WPARAM(font),TRUE);y+=px(172);}
  else for(auto&field:fields){RECT measured{0,0,px(204),0};auto caption=field.caption();DrawTextW(dc,caption.c_str(),-1,&measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);int h=field.multiline?px(field.multilineHeight):px(28),row=std::max(h,int(measured.bottom)+px(4));
   SetWindowPos(field.labelWindow,nullptr,px(16),y+px(3),px(204),row,SWP_NOZORDER);SetWindowPos(field.input,nullptr,px(232),y,px(312),field.combo()?px(220):h,SWP_NOZORDER);SendMessageW(field.labelWindow,WM_SETFONT,WPARAM(font),TRUE);SendMessageW(field.input,WM_SETFONT,WPARAM(font),TRUE);y+=row+px(10);
  }
  if(nameCounter){SetWindowPos(nameCounter,nullptr,px(16),y,px(528),px(32),SWP_NOZORDER);SendMessageW(nameCounter,WM_SETFONT,WPARAM(font),TRUE);y+=px(38);updateNameCounter();}
  SelectObject(dc,old);ReleaseDC(hwnd,dc);for(int id:{IDHELP,IDOK,IDCANCEL}){if((id==IDCANCEL&&!showCancel)||(id==IDHELP&&!preview)){ShowWindow(GetDlgItem(hwnd,id),SW_HIDE);continue;}SetWindowPos(GetDlgItem(hwnd,id),nullptr,px(id==IDHELP?16:id==IDOK?328:440),y+px(4),px(104),px(32),SWP_NOZORDER);SendDlgItemMessageW(hwnd,id,WM_SETFONT,WPARAM(font),TRUE);}
  if(previousFont)DeleteObject(previousFont);
  RECT size{0,0,px(560),y+px(52)};AdjustWindowRectExForDpi(&size,WS_CAPTION|WS_SYSMENU,FALSE,WS_EX_DLGMODALFRAME,dpi);SetWindowPos(hwnd,nullptr,0,0,size.right-size.left,size.bottom-size.top,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);
 }
 void layoutEditor(){
  if(!hwnd||!tabs||fields.empty())return;RECT client{};GetClientRect(hwnd,&client);auto previous=font;font=CreateFontW(-px(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());HDC dc=GetDC(hwnd);auto old=SelectObject(dc,font);RECT help{0,0,std::max(1,int(client.right)-px(32)),0};auto caption=fields[0].caption();DrawTextW(dc,caption.c_str(),-1,&help,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);ReleaseDC(hwnd,dc);auto plan=dialogueEditorLayout(int(client.right),int(client.bottom),dpi,int(help.bottom));HDWP batch=BeginDeferWindowPos(7);auto place=[&](HWND child,UiRect r){positionChild(batch,child,r.x,r.y,r.w,r.h);SendMessageW(child,WM_SETFONT,WPARAM(font),FALSE);};
  place(tabs,plan.tabs);place(fields[0].labelWindow,plan.instructions);place(fields[0].input,plan.input);place(GetDlgItem(hwnd,IDHELP),plan.preview);place(GetDlgItem(hwnd,IDOK),plan.accept);place(GetDlgItem(hwnd,IDCANCEL),plan.cancel);if(batch)EndDeferWindowPos(batch);
  fitTabs();
  if(previous)DeleteObject(previous);RedrawWindow(hwnd,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN);
 }
 void fitTabs(){if(!tabs)return;
  // Size tabs only after the deferred layout has committed the actual client width.
  // Native button-style tabs include borders and inter-item spacing beyond item size.
  RECT tabClient{};GetClientRect(tabs,&tabClient);TabCtrl_SetPadding(tabs,0,0);
  int tabWidth=languageTabWidth(std::max(1,int(tabClient.right-tabClient.left)-px(24)),dpi);
  TabCtrl_SetItemSize(tabs,tabWidth,px(30));
  // Keep multiline layout as a real fit check, rather than hiding overflow.
  const int minTabWidth=px(72),step=std::max(1,px(4));
  while(TabCtrl_GetRowCount(tabs)>1&&tabWidth>minTabWidth){
   tabWidth=std::max(minTabWidth,tabWidth-step);TabCtrl_SetItemSize(tabs,tabWidth,px(30));
  }
 }
 static LRESULT CALLBACK contentProc(HWND window,UINT message,WPARAM wp,LPARAM lp,UINT_PTR,DWORD_PTR data){auto self=reinterpret_cast<Form*>(data);
  if(message==WM_COMMAND||message==WM_DRAWITEM||message==WM_CTLCOLORSTATIC||message==WM_CTLCOLOREDIT||message==WM_CTLCOLORBTN)return SendMessageW(self->hwnd,message,wp,lp);
  if(message==WM_PAINT){PAINTSTRUCT ps;auto dc=BeginPaint(window,&ps);RECT r{};GetClientRect(window,&r);FillRect(dc,&r,self->theme.backBrush);EndPaint(window,&ps);return 0;}if(message==WM_ERASEBKGND)return 1;
  if(message==WM_VSCROLL||message==WM_MOUSEWHEEL){if(message==WM_VSCROLL&&lp)return 0;SCROLLINFO info{sizeof(info),SIF_ALL};GetScrollInfo(window,SB_VERT,&info);int next=info.nPos;
   if(message==WM_MOUSEWHEEL){self->contentWheel+=GET_WHEEL_DELTA_WPARAM(wp);next-=self->contentWheel/WHEEL_DELTA*self->px(48);self->contentWheel%=WHEEL_DELTA;}
   else switch(LOWORD(wp)){case SB_LINEUP:next-=self->px(32);break;case SB_LINEDOWN:next+=self->px(32);break;case SB_PAGEUP:next-=int(info.nPage);break;case SB_PAGEDOWN:next+=int(info.nPage);break;case SB_THUMBTRACK:case SB_THUMBPOSITION:next=info.nTrackPos;break;case SB_TOP:next=0;break;case SB_BOTTOM:next=info.nMax;break;}
   self->contentScroll=std::max(0,next);self->layoutMulti();return 0;}
  if(message==WM_NCDESTROY)RemoveWindowSubclass(window,contentProc,1);return DefSubclassProc(window,message,wp,lp);
 }
 void layoutMulti(){if(!contentWindow)return;RECT r{};GetClientRect(hwnd,&r);auto previous=font;font=CreateFontW(-px(14),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());int margin=px(16),top=margin;HDWP batch=BeginDeferWindowPos(8);auto place=[&](HWND child,int x,int y,int w,int h){positionChild(batch,child,x,y,std::max(1,w),std::max(1,h));SendMessageW(child,WM_SETFONT,WPARAM(font),FALSE);};
  if(tabs){place(tabs,margin,top,int(r.right)-2*margin,px(34));top+=px(44);}
  if(helpWindow){auto text=std::wstring(tw(helpId));SetWindowTextW(helpWindow,text.c_str());RECT measured{0,0,std::max(1,int(r.right)-2*margin),0};auto dc=GetDC(hwnd);auto old=SelectObject(dc,font);DrawTextW(dc,text.c_str(),-1,&measured,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);ReleaseDC(hwnd,dc);place(helpWindow,margin,top,int(r.right)-2*margin,int(measured.bottom));top+=int(measured.bottom)+px(10);}
  int footer=int(r.bottom)-px(48);place(contentWindow,margin,top,int(r.right)-2*margin,std::max(px(80),footer-top-px(10)));int button=px(104),gap=px(8);place(GetDlgItem(hwnd,IDHELP),margin,footer,button,px(32));ShowWindow(GetDlgItem(hwnd,IDHELP),preview?SW_SHOWNA:SW_HIDE);place(GetDlgItem(hwnd,IDOK),int(r.right)-margin-2*button-gap,footer,button,px(32));place(GetDlgItem(hwnd,IDCANCEL),int(r.right)-margin-button,footer,button,px(32));if(batch)EndDeferWindowPos(batch);fitTabs();
  RECT viewport{};GetClientRect(contentWindow,&viewport);int total=px(12);for(auto&field:fields)total+=px(field.check?44:field.multiline?field.multilineHeight+42:field.actions.empty()?72:104);SCROLLINFO info{sizeof(info),SIF_RANGE|SIF_PAGE|SIF_POS,0,total,UINT(std::max(1,int(viewport.bottom))),contentScroll};SetScrollInfo(contentWindow,SB_VERT,&info,TRUE);info.fMask=SIF_POS;GetScrollInfo(contentWindow,SB_VERT,&info);contentScroll=info.nPos;
  GetClientRect(contentWindow,&viewport);int y=px(6)-contentScroll,width=int(viewport.right)-px(12);batch=BeginDeferWindowPos(int(fields.size()*5));
  for(auto&field:fields){int h=px(field.multiline?field.multilineHeight:28);if(field.check){ShowWindow(field.labelWindow,SW_HIDE);SetWindowTextW(field.input,field.caption().c_str());place(field.input,px(6),y,width,px(28));y+=px(44);continue;}place(field.labelWindow,px(6),y,width,px(24));int count=int(field.actions.size());place(field.input,px(6),y+px(28),width,h);if(count){int actionWidth=(width-px(8)*(count-1))/count;for(int i=0;i<count;++i)place(field.actionWindows[size_t(i)],px(6)+i*(actionWidth+px(8)),y+px(62),actionWidth,px(30));}y+=px(field.multiline?field.multilineHeight+42:count?104:72);}
  if(batch)EndDeferWindowPos(batch);if(previous)DeleteObject(previous);RedrawWindow(hwnd,nullptr,nullptr,RDW_INVALIDATE|RDW_ERASE|RDW_ALLCHILDREN|RDW_UPDATENOW);
 }
 static LRESULT CALLBACK proc(HWND window,UINT message,WPARAM wp,LPARAM lp){
  auto self=reinterpret_cast<Form*>(GetWindowLongPtrW(window,GWLP_USERDATA));if(message==WM_NCCREATE){self=static_cast<Form*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->hwnd=window;SetWindowLongPtrW(window,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(window,message,wp,lp);
  if(message==WM_GETMINMAXINFO&&self->resizable){auto limits=reinterpret_cast<MINMAXINFO*>(lp);RECT r{0,0,self->px(480),self->px(320)};AdjustWindowRectExForDpi(&r,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,FALSE,WS_EX_DLGMODALFRAME,self->dpi);limits->ptMinTrackSize={r.right-r.left,r.bottom-r.top};return 0;}
  if(message==WM_SIZE&&self->resizable){if((self->tabs||self->multiEditor)&&wp!=SIZE_MINIMIZED){if(wp!=SIZE_MAXIMIZED){RECT r{};GetClientRect(window,&r);self->logicalWidth=std::clamp(MulDiv(int(r.right),96,self->dpi),480,1600);self->logicalHeight=std::clamp(MulDiv(int(r.bottom),96,self->dpi),320,1200);}self->layout();}return 0;}
  if(message==WM_SETTINGCHANGE||message==WM_THEMECHANGED){self->theme.sync(window);InvalidateRect(window,nullptr,TRUE);return 0;}
  if(message==WM_ERASEBKGND){RECT r{};GetClientRect(window,&r);FillRect(HDC(wp),&r,self->theme.backBrush);return 1;}
  if(message==WM_CTLCOLORSTATIC||message==WM_CTLCOLORBTN||message==WM_CTLCOLOREDIT||message==WM_CTLCOLORLISTBOX)return LRESULT(self->theme.color(HDC(wp),message==WM_CTLCOLOREDIT||message==WM_CTLCOLORLISTBOX));
  if(message==WM_DRAWITEM){auto item=reinterpret_cast<DRAWITEMSTRUCT*>(lp);if(item->hwndItem==self->tabs){wchar_t text[100]{};TCITEMW tab{};tab.mask=TCIF_TEXT;tab.pszText=text;tab.cchTextMax=100;TabCtrl_GetItem(self->tabs,item->itemID,&tab);self->theme.button(item,text,false,int(item->itemID)==self->selectedTab);}else self->theme.button(item,controlText(item->hwndItem),item->CtlID==IDOK);return TRUE;}
  if(message==WM_NOTIFY&&self->tabs){auto header=reinterpret_cast<NMHDR*>(lp);if(header->hwndFrom==self->tabs&&header->code==TCN_SELCHANGE){self->completeIME();int next=TabCtrl_GetCurSel(self->tabs);if(next>=0){if(self->onTab)self->onTab(self->selectedTab,next);self->selectedTab=next;InvalidateRect(self->tabs,nullptr,TRUE);}return 0;}}
  if(message==LanguageChanged){self->localize(Language(wp));return 0;}
  if(message==WM_COMMAND){if(HIWORD(wp)==EN_SETFOCUS){for(size_t i=0;i<self->fields.size();++i)if(self->fields[i].input==HWND(lp)&&self->fields[i].multiline)self->activeField=int(i);}
   if(LOWORD(wp)>=1000&&LOWORD(wp)<1000+self->fields.size()*4){size_t row=(LOWORD(wp)-1000)/4,index=(LOWORD(wp)-1000)%4;try{if(index<self->fields[row].actions.size())self->fields[row].actions[index].second();}catch(const std::exception&e){auto text=std::string(e.what());localizedMessage(window,GetModuleHandleW(nullptr),TextId::S016,[text]{return wide(currentSystemText(text));},false);}return 0;}if(self->nameMode&&HIWORD(wp)==EN_CHANGE){self->updateNameCounter();return 0;}if(LOWORD(wp)==IDHELP&&self->preview){try{self->preview();}catch(const std::exception&e){auto text=std::string(e.what());localizedMessage(window,GetModuleHandleW(nullptr),TextId::S016,[text]{return wide(currentSystemText(text));},false);}return 0;}if(LOWORD(wp)==IDOK){self->completeIME();for(auto&field:self->fields){if(field.check)field.value=SendMessageW(field.input,BM_GETCHECK,0,0)==BST_CHECKED?L"1":L"0";else if(field.combo())field.value=std::to_wstring(SendMessageW(field.input,CB_GETCURSEL,0,0));else{int n=GetWindowTextLengthW(field.input);std::wstring value(size_t(n)+1,0);GetWindowTextW(field.input,value.data(),n+1);value.resize(n);field.value=std::move(value);}}try{if(self->nameMode)validatePetName(utf8(self->fields[0].value));if(self->apply(self->fields))DestroyWindow(window);}catch(const std::exception&e){auto message=std::string(e.what());localizedMessage(window,GetModuleHandleW(nullptr),TextId::S016,[message]{return wide(currentSystemText(message));},false);}return 0;}if(LOWORD(wp)==IDCANCEL){DestroyWindow(window);return 0;}}
  if(message==WM_CLOSE){DestroyWindow(window);return 0;}if(message==WM_DESTROY){self->closed=true;return 0;}
  if(message==WM_DPICHANGED){self->dpi=HIWORD(wp);auto r=reinterpret_cast<RECT*>(lp);SetWindowPos(window,nullptr,r->left,r->top,r->right-r->left,r->bottom-r->top,SWP_NOZORDER);brandWindow(window);self->layout();return 0;}return DefWindowProcW(window,message,wp,lp);
 }
 void show(HWND owner,HINSTANCE instance){
  static bool registered=false;if(!registered){WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=proc;wc.lpszClassName=L"PetForm";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);RegisterClassW(&wc);registered=true;}
  // Use a real scrollable child window, not a STATIC control with transparent hit testing.
  if(multiEditor){WNDCLASSW wc{};wc.hInstance=instance;wc.lpfnWndProc=DefWindowProcW;wc.lpszClassName=L"DesktopPetFormContent";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);if(!RegisterClassW(&wc)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error(tr(TextId::S247));}
  dpi=int(owner?GetDpiForWindow(owner):GetDpiForSystem());if(dpi<=0)dpi=96;POINT cursor{};GetCursorPos(&cursor);MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST),&mi);
  hwnd=CreateWindowExW(WS_EX_DLGMODALFRAME,L"PetForm",L"",(resizable?WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN:WS_CAPTION|WS_SYSMENU),mi.rcWork.left+px(24),mi.rcWork.top+px(24),px(560),px(400),owner,nullptr,instance,this);
  if(!hwnd)throw std::runtime_error(tr(TextId::S247));
  brandWindow(hwnd);
  if(resizable){RECT size{0,0,px(logicalWidth),px(logicalHeight)};AdjustWindowRectExForDpi(&size,WS_OVERLAPPEDWINDOW|WS_CLIPCHILDREN,FALSE,WS_EX_DLGMODALFRAME,dpi);SetWindowPos(hwnd,nullptr,0,0,std::min(int(size.right-size.left),int(mi.rcWork.right-mi.rcWork.left)),std::min(int(size.bottom-size.top),int(mi.rcWork.bottom-mi.rcWork.top)),SWP_NOMOVE|SWP_NOZORDER);}
  if(multiEditor){contentWindow=CreateWindowExW(WS_EX_CONTROLPARENT,L"DesktopPetFormContent",L"",WS_CHILD|WS_VISIBLE|WS_CLIPCHILDREN|WS_VSCROLL,0,0,1,1,hwnd,nullptr,instance,nullptr);if(!contentWindow||!SetWindowSubclass(contentWindow,contentProc,1,DWORD_PTR(this))){DestroyWindow(hwnd);throw std::runtime_error(tr(TextId::S247));}if(helpId!=TextId::Count)helpWindow=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,1,1,hwnd,nullptr,instance,nullptr);}
  HWND fieldParent=multiEditor?contentWindow:hwnd;int id=100;for(auto&field:fields){field.labelWindow=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,1,1,fieldParent,nullptr,instance,nullptr);DWORD style=WS_CHILD|WS_VISIBLE|WS_TABSTOP|WS_CLIPSIBLINGS;const wchar_t*cls=L"EDIT";
   if(field.check){cls=L"BUTTON";style|=BS_AUTOCHECKBOX;}else if(field.combo()){cls=L"COMBOBOX";style|=CBS_DROPDOWNLIST|WS_VSCROLL;}else style|=WS_BORDER|(field.multiline?ES_MULTILINE|ES_AUTOVSCROLL|ES_WANTRETURN|WS_VSCROLL:ES_AUTOHSCROLL);
   field.input=CreateWindowExW(0,cls,field.check?L"":field.value.c_str(),style,0,0,1,1,fieldParent,reinterpret_cast<HMENU>(INT_PTR(id++)),instance,nullptr);
   if(field.check)SendMessageW(field.input,BM_SETCHECK,field.value==L"1"?BST_CHECKED:BST_UNCHECKED,0);else if(!field.combo()){SendMessageW(field.input,EM_SETLIMITTEXT,field.readOnly?65536:field.multiline?field.textLimit:nameMode?2048:120,0);if(field.readOnly)SendMessageW(field.input,EM_SETREADONLY,TRUE,0);}
   for(size_t action=0;action<field.actions.size();++action)field.actionWindows.push_back(CreateWindowW(L"BUTTON",tw(field.actions[action].first),WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,fieldParent,reinterpret_cast<HMENU>(INT_PTR(1000+(&field-fields.data())*4+action)),instance,nullptr));
  }
  CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,hwnd,reinterpret_cast<HMENU>(IDOK),instance,nullptr);CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,hwnd,reinterpret_cast<HMENU>(IDCANCEL),instance,nullptr);
  CreateWindowW(L"BUTTON",L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_OWNERDRAW,0,0,1,1,hwnd,reinterpret_cast<HMENU>(IDHELP),instance,nullptr);
  if(nameMode)nameCounter=CreateWindowW(L"STATIC",L"",WS_CHILD|WS_VISIBLE,0,0,1,1,hwnd,nullptr,instance,nullptr);
  if(!tabIds.empty())tabs=CreateWindowW(WC_TABCONTROLW,L"",WS_CHILD|WS_VISIBLE|WS_TABSTOP|TCS_BUTTONS|TCS_FLATBUTTONS|TCS_FIXEDWIDTH|TCS_OWNERDRAWFIXED|TCS_MULTILINE,0,0,1,1,hwnd,nullptr,instance,nullptr);
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
