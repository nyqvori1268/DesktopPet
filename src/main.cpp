#include "platform.h"
#include "sprite.h"
#include "update.h"
#include "form.h"
#include "build_config.h"
#include "custom_pet.h"
#include "handoff.h"
#include "update_window.h"
#include "trust_store.h"
#include "zip.h"
#include <commdlg.h>
#include <windowsx.h>
#include <commctrl.h>
#include <random>
#include <memory>
using namespace pet;
namespace {
constexpr UINT TRAY=WM_APP+1;
enum Command{Countdown=100,Interval,Alarm,Pause,Stop,Reset,Acknowledge,Snooze5,Snooze10,Snooze15,SnoozeCustom,Cat,Dog,Rename,Size,Opacity,Top,Pass,Visibility,Preferences,CheckUpdate,About,Exit,ImportCustom,SelectCustom,Parrot,Panda,Coat0=200,Quick0=300,CustomAction0=400,Profile0=500};
const wchar_t* cats[]={L"橘貓",L"三花貓",L"白貓",L"黑貓",L"虎斑貓"};
const wchar_t* dogs[]={L"赤色柴犬",L"黑色柴犬",L"白色柴犬",L"柯基犬",L"米色垂耳犬"};
const wchar_t* parrots[]={L"綠色虎皮鸚鵡",L"藍色虎皮鸚鵡",L"黃色虎皮鸚鵡",L"灰色玄鳳鸚鵡",L"奶黃色玄鳳鸚鵡"};
struct App{
 HINSTANCE instance;HWND window=nullptr,bubble=nullptr;HANDLE singleton=nullptr;Config c;fs::path config;Json dialogue;UpdateClient updates;UpdateHandoff handoff;PetUpdateWindow updateWindow;std::mt19937 rng{std::random_device{}()};std::string lastLine;Sprite sprite;CustomPet custom;Spring spring;Action action=Action::Idle;
 bool manualHidden=false,fullscreenHidden=false,dragging=false,pressed=false,tracking=false,updating=false,loading=true,manualCheck=false,updateNotified=false,downloadRequested=false,formOpen=false,menuOpen=false,reminderQueued=false;
 POINT down{},lastMouse{},startPos{};int dpi=96,canvas=144,petSize=96,clicks=0,strokes=0,lastDirection=0;ULONGLONG nextUpdateNotice=0,lastInput=0,actionEnd=0,bubbleEnd=0,lastClick=0,lastStroke=0,nextIdle=0,frameTime=0,lastMove=0;std::wstring speech,handoffError;std::string observedStatus;fs::path observedStage;UINT taskbar=0;HDC memory=nullptr;HBITMAP bitmap=nullptr,oldBitmap=nullptr;uint32_t*bits=nullptr;int allocated=0;
 explicit App(HINSTANCE i):instance(i){}
 ~App(){if(memory){SelectObject(memory,oldBitmap);DeleteObject(bitmap);DeleteDC(memory);}if(singleton)CloseHandle(singleton);}
 bool hidden()const{return manualHidden||fullscreenHidden;}
 void error(const std::wstring&s){MessageBoxW(window,s.c_str(),L"DesktopPet",MB_OK|MB_ICONWARNING);}
 void persist(){if(loading)return;try{saveConfig(config,c);}catch(...){error(L"設定無法儲存。請確認資料夾權限與可用空間。");}}
 void rememberMonitor(){MONITORINFOEXW mi{};mi.cbSize=sizeof(mi);GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),reinterpret_cast<MONITORINFO*>(&mi));c.monitor=utf8(mi.szDevice);}
 void clampPosition(){RECT r{c.x,c.y,c.x+canvas,c.y+canvas};MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST),&mi);c.x=std::clamp(c.x,int(mi.rcWork.left),std::max(int(mi.rcWork.left),int(mi.rcWork.right)-canvas));c.y=std::clamp(c.y,int(mi.rcWork.top),std::max(int(mi.rcWork.top),int(mi.rcWork.bottom)-canvas));c.positioned=true;SetWindowPos(window,c.top?HWND_TOPMOST:HWND_NOTOPMOST,c.x,c.y,canvas,canvas,SWP_NOACTIVATE);rememberMonitor();}
 void sizeChanged(){dpi=int(GetDpiForWindow(window));petSize=MulDiv(96*c.scale/100,dpi,96);canvas=petSize*3/2;clampPosition();render();placeBubble();}
 void tray(bool add=false){NOTIFYICONDATAW n{};n.cbSize=sizeof(n);n.hWnd=window;n.uID=1;n.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;n.uCallbackMessage=TRAY;n.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));auto tip=wide(c.name)+L" · DesktopPet";if(c.timer.pending)tip+=L" · 待確認提醒";else if(updates.busy())tip+=L" · 更新作業 "+std::to_wstring(updates.progress())+L"%";wcsncpy_s(n.szTip,tip.c_str(),_TRUNCATE);Shell_NotifyIconW(add?NIM_ADD:NIM_MODIFY,&n);if(add){n.uVersion=NOTIFYICON_VERSION_4;Shell_NotifyIconW(NIM_SETVERSION,&n);}}
 std::wstring say(const std::string&state){auto&list=dialogue[c.pet==2?"custom":c.pet==3?"parrot":c.pet==4?"panda":c.pet?"dog":"cat"][state].items();if(list.empty()&&c.pet==2)return L"我在這裡陪你。";if(list.empty())return c.pet?L"汪～我在這裡。":L"喵～我在這裡。";size_t k=rng()%list.size();if(list.size()>1&&list[k].str()==lastLine)k=(k+1)%list.size();lastLine=list[k].str();try{return wide(lastLine);}catch(...){return L"陪著你。";}}
 void speak(const std::wstring&s){if(menuOpen||c.timer.pending)return;speech=s;bubbleEnd=GetTickCount64()+4000;placeBubble();}
 void interact(Action a,const std::string&state,int ms=2400){if(menuOpen)return;lastInput=GetTickCount64();if(c.timer.pending||dragging)return;action=a;actionEnd=lastInput+(c.pet==2?custom.duration(a,unsigned(ms)):unsigned(ms));speak(say(state));scheduleFrame();}
 void reminder(){if(dragging||menuOpen){reminderQueued=true;return;}reminderQueued=false;action=Action::Remind;actionEnd=GetTickCount64()+1800;speech=wide(c.timer.title)+L"\n"+wide(c.timer.text)+L"\n（點一下確認；右鍵可延後）";bubbleEnd=0;placeBubble();tray();scheduleFrame();}
 void acknowledge(){c.timer.acknowledge();speech.clear();action=Action::Happy;actionEnd=GetTickCount64()+1500;ShowWindow(bubble,SW_HIDE);tray();persist();}
 void defer(int min){c.timer.defer(now(),min);reminderQueued=false;bubbleEnd=0;actionEnd=0;speech.clear();ShowWindow(bubble,SW_HIDE);action=Action::Idle;tray();persist();}
 void scheduleFrame(){if(hidden()||menuOpen){KillTimer(window,2);return;}UINT ms=(dragging?16:spring.moving()?25:action==Action::Sleep?(c.pet==2?250:500):83);SetTimer(window,2,ms,nullptr);}
 void render(){if(hidden()||!window)return;auto tick=GetTickCount64();double t=tick/1000.;double dt=frameTime?std::min(.04,(tick-frameTime)/1000.):.016;frameTime=tick;if(!dragging)spring.step(dt);if(c.pet!=2)sprite.make(c.pet,c.coat,action,t);
  if(allocated!=canvas){if(memory){SelectObject(memory,oldBitmap);DeleteObject(bitmap);DeleteDC(memory);}BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=canvas;bi.bmiHeader.biHeight=-canvas;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;memory=CreateCompatibleDC(nullptr);bitmap=CreateDIBSection(memory,&bi,DIB_RGB_COLORS,reinterpret_cast<void**>(&bits),nullptr,0);if(!bitmap||!memory)throw std::runtime_error("Cannot create drawing surface");oldBitmap=static_cast<HBITMAP>(SelectObject(memory,bitmap));allocated=canvas;}
  std::fill(bits,bits+canvas*canvas,0u);double sx=spring.x,sy=spring.y;if(action==Action::Stretch){sx*=1.16;sy*=.85;}if(action==Action::Belly){sx*=1.08;sy*=.94;}if(action==Action::Sleep)sy*=.93;
  const ImageClip* image=nullptr;size_t imageFrame=0;bool fallback=false;
  if(c.pet==2&&custom.ready()){
   image=&custom.clip(action,fallback);imageFrame=custom.frame(action,tick);
   if(fallback||image->frames.size()==1){
    sy*=1.+std::sin(t*(action==Action::Sleep?1.6:2.5))*.014;
    if(action==Action::Happy||action==Action::Shy)sx*=1.+std::sin(t*5)*.035;
    if(action==Action::Angry)sx*=1.+std::sin(t*18)*.025;
    if(action==Action::Blink)sy*=.88;
   }
  }
  double aspect=image?double(image->crop.w)/image->crop.h:1.;
  int w=std::max(1,int(petSize*sx*std::min(1.,aspect))),h=std::max(1,int(petSize*sy/std::max(1.,aspect))),ox=(canvas-w)/2,oy=(canvas-h)/2;int jump=0;if(action==Action::Remind&&tick<actionEnd)jump=int(std::abs(std::sin((actionEnd-tick)/1800.*3.14159265*3))*petSize*.16);oy-=jump;
  for(int y=0;y<h;++y){int yy=oy+y;if(yy<0||yy>=canvas)continue;for(int x=0;x<w;++x){int xx=ox+x;if(xx<0||xx>=canvas)continue;bits[yy*canvas+xx]=image?image->sample(imageFrame,image->crop.x+(x+.5)*image->crop.w/w-.5,image->crop.y+(y+.5)*image->crop.h/h-.5):sprite.pixels[std::min(47,y*48/h)*48+std::min(47,x*48/w)];}}
  if(image){
   // Shared transparent effects; the imported drawing itself is never pixelized.
   Sprite effect;if(action==Action::Pet||action==Action::Happy||action==Action::Shy){effect.heart(4,5);effect.heart(37,4);}
   if(action==Action::Sleep){effect.box(37,3,6,1,0xffead9bd);effect.box(41,4,1,1,0xffead9bd);effect.box(39,5,2,1,0xffead9bd);effect.box(37,6,1,1,0xffead9bd);effect.box(37,7,6,1,0xffead9bd);}
   if(action==Action::Angry){effect.box(38,5,2,3,0xffe68892);effect.box(41,8,3,2,0xffe68892);effect.box(38,10,2,3,0xffe68892);effect.box(34,8,3,2,0xffe68892);}
   int unit=std::max(1,petSize/48),left=(canvas-48*unit)/2,top=(canvas-48*unit)/2;
   for(int y=0;y<48;++y)for(int x=0;x<48;++x)if(auto color=effect.pixels[y*48+x])for(int j=0;j<unit;++j)for(int i=0;i<unit;++i){int xx=left+x*unit+i,yy=top+y*unit+j;if(xx>=0&&xx<canvas&&yy>=0&&yy<canvas)bits[yy*canvas+xx]=color;}
  }
  POINT dest{c.x,c.y},src{0,0};SIZE sz{canvas,canvas};BLENDFUNCTION blend{AC_SRC_OVER,0,BYTE(c.opacity*255/100),AC_SRC_ALPHA};UpdateLayeredWindow(window,nullptr,&dest,&sz,memory,&src,0,&blend,ULW_ALPHA);
 }
 void placeBubble(){
  if(!bubble)return;
  if(hidden()||menuOpen||speech.empty()||dragging){ShowWindow(bubble,SW_HIDE);return;}
  MONITORINFO mi{sizeof(mi)};
  GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&mi);
  const int workWidth=std::max(1,int(mi.rcWork.right-mi.rcWork.left));
  const int workHeight=std::max(1,int(mi.rcWork.bottom-mi.rcWork.top));
  const int d=int(GetDpiForWindow(window));
  const int padding=std::min({MulDiv(12,d,96),(workWidth-1)/2,(workHeight-1)/2});
  const int maxWidth=std::min(workWidth,MulDiv(480,d,96));
  HDC dc=GetDC(bubble);
  HFONT font=CreateFontW(-MulDiv(14,d,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Microsoft JhengHei UI");
  auto old=SelectObject(dc,font);
  RECT natural{};
  DrawTextW(dc,speech.c_str(),-1,&natural,DT_CALCRECT|DT_NOPREFIX);
  int width=std::clamp(int(natural.right-natural.left)+2*padding,std::min(maxWidth,MulDiv(48,d,96)),maxWidth);
  RECT text{0,0,std::max(1,width-2*padding),0};
  DrawTextW(dc,speech.c_str(),-1,&text,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
  // For unusually tall text, use more of the work area before capping height.
  if(text.bottom+2*padding>workHeight&&width<workWidth){
   width=workWidth;text={0,0,std::max(1,width-2*padding),0};
   DrawTextW(dc,speech.c_str(),-1,&text,DT_CALCRECT|DT_WORDBREAK|DT_NOPREFIX);
  }
  SelectObject(dc,old);DeleteObject(font);ReleaseDC(bubble,dc);
  const int height=std::clamp(int(text.bottom)+2*padding,1,workHeight);
  int x=c.x+(canvas-width)/2,y=c.y-height+petSize/8;
  if(y<mi.rcWork.top)y=c.y+canvas-petSize/8;
  y=std::clamp(y,int(mi.rcWork.top),int(mi.rcWork.bottom)-height);
  x=std::clamp(x,int(mi.rcWork.left),int(mi.rcWork.right)-width);
  SetWindowPos(bubble,c.top?HWND_TOPMOST:HWND_NOTOPMOST,x,y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);
  SetWindowRgn(bubble,CreateRoundRectRgn(0,0,width+1,height+1,MulDiv(14,d,96),MulDiv(14,d,96)),TRUE);
  InvalidateRect(bubble,nullptr,TRUE);
 }

 bool isFullscreen(){auto w=GetForegroundWindow();if(!w||w==window||w==bubble||w==GetShellWindow()||!IsWindowVisible(w)||IsIconic(w))return false;wchar_t cls[128]{};GetClassNameW(w,cls,128);if(wcscmp(cls,L"Progman")==0||wcscmp(cls,L"WorkerW")==0||wcsstr(cls,L"TrayWnd"))return false;RECT r{};if(!GetWindowRect(w,&r))return false;MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST),&mi);return r.left<=mi.rcMonitor.left+2&&r.top<=mi.rcMonitor.top+2&&r.right>=mi.rcMonitor.right-2&&r.bottom>=mi.rcMonitor.bottom-2;}
 void visibility(){bool was=fullscreenHidden;fullscreenHidden=c.autoHide&&isFullscreen();if(hidden()){ShowWindow(window,SW_HIDE);ShowWindow(bubble,SW_HIDE);KillTimer(window,2);}else{ShowWindow(window,SW_SHOWNOACTIVATE);if(was&&c.timer.pending)reminder();render();placeBubble();scheduleFrame();}}
 void tick(){auto t=GetTickCount64();if(c.timer.tick(now())){lastInput=t;reminder();persist();}visibility();if(updates.busy())tray();if(!menuOpen&&!c.timer.pending&&bubbleEnd&&t>=bubbleEnd){speech.clear();bubbleEnd=0;ShowWindow(bubble,SW_HIDE);}if(c.autoUpdate&&updates.enabled()&&!updates.busy()&&(now()-c.lastCheck>=86400||now()<c.lastCheck)){c.lastCheck=now();updates.check();persist();}if(!menuOpen)updateTick();}
 void animate(){if(menuOpen)return;auto t=GetTickCount64();if(!dragging&&!c.timer.pending){if(t-lastInput>=300000){if(action!=Action::Sleep){action=Action::Sleep;actionEnd=0;speak(say("sleep"));}}else if(actionEnd&&t>=actionEnd){action=Action::Idle;actionEnd=0;}if(!actionEnd&&action!=Action::Sleep&&t>=nextIdle){static Action idle[]={Action::Blink,Action::Yawn,Action::Stretch,Action::Belly,Action::Happy};action=idle[rng()%5];actionEnd=t+(c.pet==2?custom.duration(action,action==Action::Blink?250u:2300u):(action==Action::Blink?250u:2300u));nextIdle=t+15000+rng()%30000;}}render();scheduleFrame();}
 void applyPass(){auto ex=GetWindowLongPtrW(window,GWL_EXSTYLE);if(c.pass)ex|=WS_EX_TRANSPARENT;else ex&=~LONG_PTR(WS_EX_TRANSPARENT);SetWindowLongPtrW(window,GWL_EXSTYLE,ex);SetWindowLongPtrW(bubble,GWL_EXSTYLE,(GetWindowLongPtrW(bubble,GWL_EXSTYLE)&~LONG_PTR(WS_EX_TRANSPARENT))|(c.pass?WS_EX_TRANSPARENT:0));}
 void showForm(Form&f){if(formOpen)return;formOpen=true;f.show(window,instance);formOpen=false;lastInput=GetTickCount64();}
 void timerForm(Mode mode){Form f;f.title=mode==Mode::Countdown?L"倒數計時":mode==Mode::Interval?L"循環提醒":L"鬧鐘設定";int n=std::clamp(c.timer.interval,1,Timer::maxInterval(mode));
  if(mode==Mode::Alarm){f.fields={{L"時（0～23）",std::to_wstring(c.timer.alarmHour)},{L"分（0～59）",std::to_wstring(c.timer.alarmMinute)},{L"重複",std::to_wstring(c.timer.repeat),std::vector<std::wstring>{L"單次",L"每日",L"指定星期"}},{L"星期（例：日一三五）",L"日一二三四五六"}};std::wstring days;for(int i=0;i<7;++i)if(c.timer.weekdays&(1<<i))days+=L"日一二三四五六"[i];f.fields[3].value=days;}
  else f.fields={{mode==Mode::Interval?L"時（0～24）":L"時（0～168）",std::to_wstring(n/3600)},{L"分（0～59）",std::to_wstring(n/60%60)},{L"秒（0～59）",std::to_wstring(n%60)}};
  f.fields.emplace_back(L"提醒標題",wide(c.timer.title));f.fields.emplace_back(L"提醒文字",wide(c.timer.text));f.fields.back().multiline=true;
  f.apply=[&,mode](auto&v){Timer temp=c.timer;temp.mode=mode;size_t k;if(mode==Mode::Alarm){temp.alarmHour=number(v[0].value,0,23);temp.alarmMinute=number(v[1].value,0,59);temp.repeat=number(v[2].value,0,2);temp.weekdays=0;for(auto ch:v[3].value){auto p=std::wstring(L"日一二三四五六").find(ch);if(p!=std::wstring::npos)temp.weekdays|=1<<p;}if(!temp.weekdays&&temp.repeat==2)throw std::runtime_error("請至少選一天星期");if(!temp.weekdays)temp.weekdays=127;k=4;}else{temp.interval=number(v[0].value,0,mode==Mode::Interval?24:168)*3600+number(v[1].value,0,59)*60+number(v[2].value,0,59);if(!temp.validInterval())throw std::runtime_error(mode==Mode::Interval?"循環間隔需為 1 秒至 24 小時；24 小時時分、秒必須為 0":"時間需為 1 秒至 7 天");k=3;}temp.title=utf8(v[k].value);temp.text=utf8(v[k+1].value);temp.start(now());c.timer=temp;speech.clear();ShowWindow(bubble,SW_HIDE);tray();persist();return true;};showForm(f);
 }
 void oneNumber(const std::wstring&title,int value,int lo,int hi,const std::function<void(int)>&save){Form f;f.title=title;f.fields.emplace_back(L"數值（"+std::to_wstring(lo)+L"～"+std::to_wstring(hi)+L"）",std::to_wstring(value));f.apply=[&](auto&v){save(number(v[0].value,lo,hi));persist();return true;};showForm(f);}
 void preferences(){Form f;f.title=L"偏好設定";f.fields={{L"全螢幕自動隱藏",c.autoHide?L"1":L"0",true},{L"開機自動啟動",c.startup?L"1":L"0",true},{L"自動檢查更新",c.autoUpdate?L"1":L"0",true},{L"預設延後（分鐘）",std::to_wstring(c.timer.snoozeMinutes)},{L"略過版本",wide(c.skip)}};f.apply=[&](auto&v){int snooze=number(v[3].value,1,1440);bool startup=v[1].value==L"1";setStartup(startup);c.startup=startup;c.autoHide=v[0].value==L"1";c.autoUpdate=v[2].value==L"1";c.timer.snoozeMinutes=snooze;c.skip=utf8(v[4].value);visibility();persist();return true;};showForm(f);}
 void selectPet(int pet){if(!c.selectBuiltin(pet,c.coat))return;custom=CustomPet{};render();tray();persist();}
 void selectProfile(int slot){
  CustomPet next;std::string failure;
  if(slot<0||slot>=3||!next.load(c.profiles[size_t(slot)].files,&failure)){error(failure.empty()?L"請先匯入這組寵物的待機素材。":wide(failure));return;}
  if(!c.selectCustom(slot))return;custom=std::move(next);render();tray();persist();
 }
 void renameProfile(int slot){
  Form f;f.title=L"自訂寵物 "+std::to_wstring(slot+1)+L" 名稱";f.fields.emplace_back(L"名稱（1～30 字）",wide(c.profiles[size_t(slot)].name));
  f.apply=[&](auto&fields){auto value=fields[0].value;if(value.empty()||value.size()>30)throw std::runtime_error("名字需為 1～30 字");c.profiles[size_t(slot)].name=utf8(value);if(c.pet==2&&c.activeCustom==slot)c.name=utf8(value);tray();persist();return true;};showForm(f);
 }
 void importCustom(int state,int slot=-1){
  if(slot<0)slot=c.activeCustom;
  if(state>0&&!c.profiles[size_t(slot)].ready()){error(L"請先匯入自訂寵物。");return;}
  wchar_t file[32768]{};OPENFILENAMEW picker{};picker.lStructSize=sizeof(picker);picker.hwndOwner=window;
  picker.lpstrFilter=L"透明 PNG／GIF\0*.png;*.gif\0";picker.lpstrFile=file;picker.nMaxFile=32768;
  picker.lpstrTitle=L"選擇離線寵物素材";picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_DONTADDTORECENT;
  if(!GetOpenFileNameW(&picker)){if(CommDlgExtendedError())error(L"無法開啟素材選擇視窗。");return;}
  auto bytes=readBytes(file,8*1024*1024);auto header=inspectImage(bytes);
  auto name=sha256(bytes)+(header.gif?".gif":".png");auto stored=customDirectory()/wide(name);rejectReparse(stored);
  atomicWrite(stored,bytes.data(),bytes.size());auto decoded=decodeCustom(stored);
  if(state==0){
   Form f;f.title=L"自訂寵物名稱";auto proposed=fs::path(file).stem().wstring();if(proposed.empty()||proposed.size()>30)proposed=L"自訂寵物";
   f.fields.emplace_back(L"名稱（1～30 字）",proposed);
   f.apply=[&](auto&fields){auto text=fields[0].value;if(text.empty()||text.size()>30)throw std::runtime_error("名字需為 1～30 字");
    c.profiles[size_t(slot)].files=Json{};c.profiles[size_t(slot)].files[customStates[0]]=name;c.profiles[size_t(slot)].name=utf8(text);c.selectCustom(slot);custom=CustomPet{};custom.set(0,std::move(decoded));render();tray();persist();return true;};showForm(f);
  }else{c.profiles[size_t(slot)].files[customStates[state]]=name;if(c.pet==2&&c.activeCustom==slot)custom.set(state,std::move(decoded));render();persist();}
 }
 void command(UINT id){if(id>=Profile0&&id<Profile0+48){int slot=int(id-Profile0)/16,action=int(id-Profile0)%16;if(action==0)selectProfile(slot);else if(action==1)renameProfile(slot);else if(action==2){if(MessageBoxW(window,L"清除此組設定？其他組與共用素材檔會保留。",L"清除自訂寵物",MB_YESNO|MB_ICONQUESTION)==IDYES){bool current=c.pet==2&&c.activeCustom==slot;c.clearProfile(slot);if(current)custom=CustomPet{};render();tray();persist();}}else if(action>=3&&action<=9)importCustom(action-3,slot);return;}if(id>=CustomAction0&&id<CustomAction0+7){importCustom(int(id-CustomAction0));return;}lastInput=GetTickCount64();if(id>=Coat0&&id<Coat0+5){if(c.pet==2||c.pet==4)return;c.coat=int(id-Coat0);render();persist();return;}if(id>=Quick0&&id<Quick0+5){int mins[]={5,15,25,30,60};c.timer.mode=Mode::Countdown;c.timer.interval=mins[id-Quick0]*60;c.timer.start(now());speech.clear();ShowWindow(bubble,SW_HIDE);tray();persist();return;}
  switch(id){case Countdown:timerForm(Mode::Countdown);break;case Interval:timerForm(Mode::Interval);break;case Alarm:timerForm(Mode::Alarm);break;case Pause:if(c.timer.paused)c.timer.resume(now());else c.timer.pause(now());persist();break;case Stop:c.timer.stop();speech.clear();ShowWindow(bubble,SW_HIDE);tray();persist();break;case Reset:c.timer.start(now());speech.clear();ShowWindow(bubble,SW_HIDE);tray();persist();break;case Acknowledge:if(MessageBoxW(window,(wide(c.timer.title)+L"\n\n"+wide(c.timer.text)+L"\n\n確認此提醒？").c_str(),L"待處理提醒",MB_YESNO|MB_ICONINFORMATION)==IDYES)acknowledge();break;case Snooze5:defer(5);break;case Snooze10:defer(10);break;case Snooze15:defer(15);break;case SnoozeCustom:oneNumber(L"延後提醒（分鐘）",c.timer.snoozeMinutes,1,1440,[&](int n){defer(n);});break;case Cat:case Dog:selectPet(id==Dog?1:0);break;case ImportCustom:importCustom(0);break;case SelectCustom:selectProfile(c.activeCustom);break;case Parrot:selectPet(3);break;case Panda:selectPet(4);break;case Rename:{Form f;f.title=L"修改名稱";f.fields.emplace_back(L"寵物名稱",wide(c.name));f.apply=[&](auto&v){if(v[0].value.empty()||v[0].value.size()>30)throw std::runtime_error("名字需為 1～30 字");c.name=utf8(v[0].value);if(c.pet==2)c.profiles[size_t(c.activeCustom)].name=c.name;else c.builtinName=c.name;tray();persist();return true;};showForm(f);break;}case Size:oneNumber(L"大小（%）",c.scale,50,300,[&](int n){c.scale=n;sizeChanged();});break;case Opacity:oneNumber(L"透明度（%）",c.opacity,20,100,[&](int n){c.opacity=n;render();});break;case Top:c.top=!c.top;clampPosition();placeBubble();persist();break;case Pass:c.pass=!c.pass;applyPass();persist();break;case Visibility:manualHidden=!manualHidden;visibility();if(!manualHidden&&c.timer.pending)reminder();break;case Preferences:preferences();break;case CheckUpdate:if(updates.busy()||handoff.pending()){auto info=updates.latest();if(!info.version.empty())offerUpdate(info);}else{manualCheck=true;updates.check();if(!updates.enabled()){error(wide(updates.message()));manualCheck=false;}}break;case About:MessageBoxW(window,(L"DesktopPet "+wide(PET_VERSION)+L"\n原創像素桌面寵物\n完全無聲 · 離線運作（更新除外）\n授權條款尚未指定。").c_str(),L"關於 DesktopPet",MB_OK);break;case Exit:if(handoff.pending())handoff.cancel();DestroyWindow(window);break;}
 }
 void menu(POINT p){
  if(menuOpen)return;
  menuOpen=true;ShowWindow(bubble,SW_HIDE);KillTimer(window,2);
  const ULONGLONG opened=GetTickCount64(),previousEnd=bubbleEnd;
  strokes=clicks=lastDirection=0;
  if(pressed){pressed=dragging=false;ReleaseCapture();spring.vy=2;persist();}
  auto finish=[&]{
   menuOpen=false;
   if(!IsWindow(window)||!IsWindow(bubble))return;
   // A normal bubble keeps its remaining visible time across a cancelled menu.
   if(!c.timer.pending&&bubbleEnd&&bubbleEnd==previousEnd)bubbleEnd+=GetTickCount64()-opened;
   if(c.timer.pending&&reminderQueued)reminder();
   else{reminderQueued=false;placeBubble();}
   scheduleFrame();
  };
  try{
  HMENU m=CreatePopupMenu();auto add=[&](UINT id,const wchar_t*s,UINT flags=0){AppendMenuW(m,MF_STRING|flags,id,s);};
  if(c.timer.running){auto label=c.timer.paused?L"計時：已暫停":L"計時：進行中";std::wstring status=label;if(!c.timer.paused&&c.timer.due){auto sec=std::max<Seconds>(0,c.timer.due-now());status+=L"（剩餘 "+std::to_wstring(sec/3600)+L":"+(sec/60%60<10?L"0":L"")+std::to_wstring(sec/60%60)+L":"+(sec%60<10?L"0":L"")+std::to_wstring(sec%60)+L"）";}add(0,status.c_str(),MF_GRAYED);add(Pause,c.timer.paused?L"繼續計時":L"暫停計時");add(Stop,L"停止計時");}add(Reset,L"重設並開始計時");
  if(c.timer.pending){std::wstring label=L"確認提醒："+wide(c.timer.title);add(Acknowledge,label.c_str());add(Snooze5,L"延後 5 分鐘");add(Snooze10,L"延後 10 分鐘");add(Snooze15,L"延後 15 分鐘");add(SnoozeCustom,L"自訂延後…");}
  AppendMenuW(m,MF_SEPARATOR,0,nullptr);add(Countdown,L"倒數計時…");add(Interval,L"循環提醒…");add(Alarm,L"鬧鐘設定…");HMENU quick=CreatePopupMenu();const wchar_t*q[]={L"5 分鐘",L"15 分鐘",L"25 分鐘",L"30 分鐘",L"60 分鐘"};for(int i=0;i<5;++i)AppendMenuW(quick,MF_STRING,Quick0+i,q[i]);AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(quick),L"快捷倒數");
  AppendMenuW(m,MF_SEPARATOR,0,nullptr);HMENU pets=CreatePopupMenu();AppendMenuW(pets,MF_STRING|(c.pet==0?MF_CHECKED:0),Cat,L"貓咪");AppendMenuW(pets,MF_STRING|(c.pet==1?MF_CHECKED:0),Dog,L"狗狗");AppendMenuW(pets,MF_STRING|(c.pet==3?MF_CHECKED:0),Parrot,L"鸚鵡");AppendMenuW(pets,MF_STRING|(c.pet==4?MF_CHECKED:0),Panda,L"黑白熊貓");AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(pets),L"更換寵物");
  for(int slot=0;slot<3;++slot){auto&profile=c.profiles[size_t(slot)];HMENU group=CreatePopupMenu();UINT base=Profile0+slot*16;AppendMenuW(group,MF_STRING|(profile.ready()?0:MF_GRAYED)|(c.pet==2&&c.activeCustom==slot?MF_CHECKED:0),base,L"使用這組寵物");AppendMenuW(group,MF_STRING,base+1,L"編輯名稱…");AppendMenuW(group,MF_STRING,base+2,L"清除此組設定…");const wchar_t* labels[]={L"匯入待機／更換角色…",L"匯入眨眼…",L"匯入睡覺…",L"匯入撒嬌…",L"匯入生氣…",L"匯入愛心…",L"匯入提醒…"};for(int i=0;i<7;++i)AppendMenuW(group,MF_STRING|((i&&!profile.ready())?MF_GRAYED:0),base+3+i,labels[i]);auto label=L"自訂寵物 "+std::to_wstring(slot+1)+L"："+wide(profile.name)+(c.pet==2&&c.activeCustom==slot?L"（使用中）":profile.ready()?L"（已設定）":L"（未設定）");AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(group),label.c_str());}
  HMENU coats=CreatePopupMenu();for(int i=0;i<5;++i)AppendMenuW(coats,MF_STRING|(i==c.coat?MF_CHECKED:0),Coat0+i,c.pet==3?parrots[i]:c.pet?dogs[i]:cats[i]);AppendMenuW(m,MF_POPUP|((c.pet==2||c.pet==4)?MF_GRAYED:0),reinterpret_cast<UINT_PTR>(coats),L"更換毛色");add(Rename,L"修改名稱…");add(Size,L"調整大小…");add(Opacity,L"調整透明度…");add(Top,L"永遠置頂",c.top?MF_CHECKED:0);add(Pass,L"滑鼠穿透（可由系統匣解除）",c.pass?MF_CHECKED:0);add(Visibility,manualHidden?L"顯示寵物":L"隱藏寵物");add(Preferences,L"偏好設定…");std::wstring updateLabel=updates.busy()?L"更新作業 "+std::to_wstring(updates.progress())+L"%（點選取消）":L"檢查更新…";add(CheckUpdate,updateLabel.c_str());add(About,L"關於…");add(Exit,L"結束程式");
  SetForegroundWindow(window);UINT id=TrackPopupMenu(m,TPM_RETURNCMD|TPM_RIGHTBUTTON,p.x,p.y,0,window,nullptr);DestroyMenu(m);PostMessageW(window,WM_NULL,0,0);if(id)command(id);
  }catch(...){finish();throw;}
  finish();
 }
 void bindUpdateWindow(){
  updateWindow.bind([this]{if(handoff.pending())return;handoffError.clear();if(updates.phase()==UpdatePhase::Ready&&!updates.stage().empty())installUpdate(updates.stage());else{downloadRequested=true;updates.download();updateWindow.state(0,L"正在下載更新…",true);}},[this]{updateNotified=false;nextUpdateNotice=GetTickCount64()+3600000;},[this]{c.skip=updates.latest().version;persist();},[this]{downloadRequested=false;if(handoff.pending()){handoff.reset();handoffError=L"已取消更新，主程式繼續運作。";updateWindow.state(0,handoffError,false);}else{updates.cancel();updateWindow.state(updates.progress(),L"正在取消下載…",true);}});
 }
 void updateTick(){
  if(handoff.pending()){
   int result=handoff.poll();updateWindow.state(100,L"已驗證更新，等待安全交接與正常關閉…",true);
   if(result==1&&!formOpen&&!menuOpen){try{saveConfig(config,c);updates.cancel();updating=true;handoff.commit();DestroyWindow(window);}catch(const std::exception&e){handoff.reset();updating=false;handoffError=L"無法儲存設定，更新已取消："+wide(e.what());updateWindow.state(0,handoffError,false);}}
   else if(result<0){handoff.reset();std::string detail=result==-2?"更新器準備逾時，未關閉主程式或替換檔案。":"更新器未完成身分驗證或已結束，未替換檔案。";try{detail=readText(observedStage/L"handoff-error.txt",8192);}catch(...){}handoffError=wide(detail);updateWindow.state(0,handoffError,false);}
   return;
  }
  updateWindow.state(updates.progress(),handoffError.empty()?wide(updates.message()):handoffError,updates.busy());if(updates.busy())return;
  auto status=updates.message();auto info=updates.latest();if(status!=observedStatus||manualCheck){observedStatus=status;if(manualCheck&&!status.empty()){manualCheck=false;if(!info.version.empty())offerUpdate(info);else error(wide(status));}}
  if(!info.version.empty()&&c.skip!=info.version&&!updateNotified&&GetTickCount64()>=nextUpdateNotice){updateNotified=true;speak(L"有新版本 "+wide(info.version)+L"！右鍵選單可檢查更新。");}
  if(downloadRequested&&!formOpen&&updates.phase()==UpdatePhase::Ready&&!updates.stage().empty()){downloadRequested=false;installUpdate(updates.stage());}
  else if(downloadRequested&&(updates.phase()==UpdatePhase::Failed||updates.phase()==UpdatePhase::Cancelled)){downloadRequested=false;updateWindow.state(updates.progress(),wide(status),false);}
 }
 void offerUpdate(const UpdateInfo&info){updateWindow.show(window,instance,info);updateWindow.state(updates.progress(),wide(updates.message()),updates.busy()||handoff.pending());}
 void installUpdate(const fs::path&stage){
  if(handoff.pending())return;
  try{handoffError.clear();saveConfig(config,c);auto temp=appData()/L"updaters"/wide(randomId());fs::create_directories(temp);auto bytes=readBytes(executable().parent_path()/L"Updater.exe");atomicWrite(temp/L"Updater.exe",bytes.data(),bytes.size());observedStage=stage;std::error_code staleError;fs::remove(stage/L"handoff-error.txt",staleError);handoff.start(temp/L"Updater.exe",stage,executable().parent_path());updateWindow.state(100,L"正在啟動更新器並驗證安全交接…",true);}catch(const std::exception&e){handoff.reset();handoffError=L"無法準備更新："+wide(e.what());updateWindow.state(0,handoffError,false);}
 }
 void mouseDown(){if(menuOpen)return;if(c.timer.pending){acknowledge();return;}pressed=true;dragging=false;GetCursorPos(&down);lastMouse=down;startPos={c.x,c.y};lastMove=GetTickCount64();SetCapture(window);}
 void mouseMove(LPARAM lp){if(menuOpen)return;auto t=GetTickCount64();POINT p;GetCursorPos(&p);if(pressed){if(!dragging&&(std::abs(p.x-down.x)>MulDiv(4,dpi,96)||std::abs(p.y-down.y)>MulDiv(4,dpi,96))){dragging=true;action=Action::Drag;speak(say("drag"));ShowWindow(bubble,SW_HIDE);}if(dragging){double dt=std::max<ULONGLONG>(1,t-lastMove);spring.drag((p.x-lastMouse.x)*16/dt,(p.y-lastMouse.y)*16/dt);c.x=startPos.x+p.x-down.x;c.y=startPos.y+p.y-down.y;clampPosition();render();scheduleFrame();lastMouse=p;lastMove=t;}lastInput=t;return;}
  if(!tracking){TRACKMOUSEEVENT e{sizeof(e),TME_LEAVE,window,0};TrackMouseEvent(&e);tracking=true;}int x=GET_X_LPARAM(lp);int direction=x>lastMouse.x?1:x<lastMouse.x?-1:0;if(t-lastStroke>1800)strokes=0;if(direction&&direction!=lastDirection&&t-lastStroke>100){lastDirection=direction;lastStroke=t;if(++strokes>=5){strokes=0;interact(Action::Pet,"pet");}}lastMouse.x=x;
 }
 void mouseUp(){if(menuOpen||!pressed)return;pressed=false;ReleaseCapture();lastInput=GetTickCount64();if(dragging){dragging=false;spring.vx=-2;spring.vy=3;action=Action::Happy;actionEnd=lastInput+1500;persist();if(c.timer.pending)reminder();else{speech=say("happy");bubbleEnd=lastInput+4000;placeBubble();}scheduleFrame();return;}
  if(lastInput-lastClick<650)++clicks;else clicks=1;lastClick=lastInput;if(clicks>=4){interact(Action::Angry,"angry");clicks=0;return;}if(action==Action::Sleep){interact(Action::Wake,"happy");return;}static Action a[]={Action::Happy,Action::Shy,Action::Surprise,Action::Yawn,Action::Stretch,Action::Belly};auto k=rng()%6;interact(a[k],k==0?"happy":"general");
 }
 static LRESULT CALLBACK bubbleProc(HWND w,UINT m,WPARAM wp,LPARAM lp){auto self=reinterpret_cast<App*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(w,m,wp,lp);if(m==WM_MOUSEACTIVATE)return MA_NOACTIVATE;if(m==WM_LBUTTONUP){if(!self->menuOpen&&self->c.timer.pending)self->acknowledge();return 0;}if(m==WM_RBUTTONUP){POINT p;GetCursorPos(&p);self->menu(p);return 0;}if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);auto brush=CreateSolidBrush(RGB(255,248,235));FillRect(dc,&r,brush);DeleteObject(brush);int d=int(GetDpiForWindow(w));auto f=CreateFontW(-MulDiv(14,d,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,L"Microsoft JhengHei UI");auto old=SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(77,65,76));InflateRect(&r,-MulDiv(12,d,96),-MulDiv(12,d,96));DrawTextW(dc,self->speech.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);DeleteObject(f);EndPaint(w,&ps);return 0;}return DefWindowProcW(w,m,wp,lp);}
 static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){auto self=reinterpret_cast<App*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=w;SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(w,m,wp,lp);
  try{switch(m){case WM_MOUSEACTIVATE:return MA_NOACTIVATE;case WM_TIMER:if(wp==1)self->tick();else if(wp==2)self->animate();return 0;case WM_LBUTTONDOWN:self->mouseDown();return 0;case WM_MOUSEMOVE:self->mouseMove(lp);return 0;case WM_LBUTTONUP:self->mouseUp();return 0;case WM_CAPTURECHANGED:if(self->pressed){self->pressed=false;self->dragging=false;self->spring.vy=2;self->persist();if(self->c.timer.pending)self->reminder();}return 0;case WM_MOUSELEAVE:self->tracking=false;return 0;case WM_RBUTTONUP:{POINT p;GetCursorPos(&p);self->menu(p);return 0;}case WM_DPICHANGED:{auto r=reinterpret_cast<RECT*>(lp);self->c.x=r->left;self->c.y=r->top;self->sizeChanged();return 0;}case WM_DISPLAYCHANGE:self->sizeChanged();self->persist();return 0;case WM_SETTINGCHANGE:if(wp==SPI_SETWORKAREA){self->clampPosition();self->placeBubble();}return 0;case WM_POWERBROADCAST:if(wp==PBT_APMRESUMEAUTOMATIC||wp==PBT_APMRESUMESUSPEND)self->tick();return TRUE;case TRAY:if(LOWORD(lp)==WM_CONTEXTMENU||LOWORD(lp)==WM_RBUTTONUP||LOWORD(lp)==NIN_SELECT||LOWORD(lp)==NIN_KEYSELECT){POINT p;GetCursorPos(&p);self->menu(p);}return 0;case WM_CLOSE:self->manualHidden=true;self->visibility();return 0;case WM_QUERYENDSESSION:return TRUE;case WM_ENDSESSION:if(wp){self->c.timer.stop();self->persist();}return 0;case WM_DESTROY:{KillTimer(w,1);KillTimer(w,2);if(!self->updating)self->c.timer.stop();self->persist();NOTIFYICONDATAW n{};n.cbSize=sizeof(n);n.hWnd=w;n.uID=1;Shell_NotifyIconW(NIM_DELETE,&n);if(self->bubble)DestroyWindow(self->bubble);PostQuitMessage(0);return 0;}}
   if(m==self->taskbar){self->tray(true);return 0;}
  }catch(const std::exception&e){self->error(L"操作未完成："+wide(e.what()));}return DefWindowProcW(w,m,wp,lp);
 }
 bool init(bool restore){singleton=CreateMutexW(nullptr,FALSE,L"Local\\DesktopPet.SingleInstance");if(!singleton||GetLastError()==ERROR_ALREADY_EXISTS)return false;config=settingsPath();try{c.load(Json::parse(readText(config)),restore);}catch(...){/* Missing/corrupt settings use documented defaults. */}
  std::string assetFailure;if(c.pet==2&&!custom.load(c.profiles[size_t(c.activeCustom)].files,&assetFailure))c.selectBuiltin(0);
  try{dialogue=Json::parse(readText(executable().parent_path()/L"data"/L"dialogue.json"));}catch(...){dialogue=Json{};}
  WNDCLASSW wc{};wc.hInstance=instance;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.lpfnWndProc=proc;wc.lpszClassName=L"DesktopPetWindow";wc.hIcon=LoadIconW(instance,MAKEINTRESOURCEW(1));RegisterClassW(&wc);wc.lpfnWndProc=bubbleProc;wc.lpszClassName=L"DesktopPetBubble";RegisterClassW(&wc);
  window=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"DesktopPetWindow",L"DesktopPet",WS_POPUP,c.x,c.y,144,144,nullptr,nullptr,instance,this);if(!window)throw std::runtime_error("Cannot create pet window");
  bubble=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"DesktopPetBubble",L"",WS_POPUP,0,0,10,10,window,nullptr,instance,this);if(!bubble)throw std::runtime_error("Cannot create bubble");
  if(!c.positioned){MONITORINFO mi{sizeof(mi)};POINT zero{0,0};GetMonitorInfoW(MonitorFromPoint(zero,MONITOR_DEFAULTTOPRIMARY),&mi);c.x=mi.rcWork.right-170;c.y=mi.rcWork.bottom-170;}sizeChanged();applyPass();taskbar=RegisterWindowMessageW(L"TaskbarCreated");bindUpdateWindow();tray(true);lastInput=GetTickCount64();nextIdle=lastInput+20000;frameTime=lastInput;loading=false;SetTimer(window,1,1000,nullptr);visibility();if(!assetFailure.empty())speak(L"部分自訂素材無法載入，已使用預設動作或橘貓。");if(c.timer.pending)reminder();if(c.autoUpdate&&updates.enabled()){c.lastCheck=now();updates.check();}persist();return true;
 }
};
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES|ICC_PROGRESS_CLASS};InitCommonControlsEx(&controls);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(argc==5&&wcscmp(argv[1],L"--verify-update")==0){int result=0;try{auto info=verifyManifest(readBytes(argv[2],65536),readBytes(argv[3],64));auto bytes=readBytes(argv[4],15*1024*1024);auto metadata=Json::parse(std::string(info.manifest.begin(),info.manifest.end()));if(bytes.size()!=info.size||sha256(bytes)!=metadata["sha256"].str())throw std::runtime_error("Package hash");inspectZip(bytes);}catch(...){result=1;}LocalFree(argv);CoUninitialize();return result;}if(argc==4&&wcscmp(argv[1],L"--adopt-update-trust")==0){int result=0;try{commitInstalledTrust(readBytes(argv[2],65536),readBytes(argv[3],64),PET_VERSION);}catch(...){result=1;}LocalFree(argv);CoUninitialize();return result;}bool restore=argc>1&&wcscmp(argv[1],L"--updated")==0;std::wstring eventName=restore&&argc>2?argv[2]:L"";LocalFree(argv);
 try{
  auto dir=executable().parent_path();if(!restore&&fs::exists(dir/L".desktoppet-transaction.json")){auto temp=appData()/L"updaters"/wide(randomId());fs::create_directories(temp);auto b=readBytes(dir/L"Updater.exe");atomicWrite(temp/L"Updater.exe",b.data(),b.size());auto pi=launch(temp/L"Updater.exe",{L"--recover",dir.wstring()});CloseHandle(pi.hProcess);CloseHandle(pi.hThread);return 0;}
  // Remove only stale, non-reparse update working directories; preserve active/recoverable transactions.
  if(!fs::exists(dir/L".desktoppet-transaction.json"))for(auto root:{appData()/L"updates",appData()/L"updaters"}){std::error_code ec;if(fs::exists(root,ec))for(auto&i:fs::directory_iterator(root,ec)){if(i.is_symlink(ec))continue;if(fs::file_time_type::clock::now()-i.last_write_time(ec)>std::chrono::hours(24*7)){try{rejectReparse(i.path());fs::remove_all(i.path(),ec);}catch(...){}}}}
  App app(instance);if(!app.init(restore))return 0;if(!eventName.empty()){HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,eventName.c_str());if(event){SetEvent(event);CloseHandle(event);}}
  MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){if(!app.updateWindow.dialog(msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}CoUninitialize();return 0;
 }catch(const std::exception&e){MessageBoxW(nullptr,wide(e.what()).c_str(),L"DesktopPet 啟動失敗",MB_OK|MB_ICONERROR);CoUninitialize();return 1;}
}
