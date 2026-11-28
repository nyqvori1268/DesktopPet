#include "platform.h"
#include "sprite.h"
#include "update.h"
#include "form.h"
#include "nameplate.h"
#include "menu_layer.h"
#include "scale_window.h"
#include "timer_window.h"
#include "about_window.h"
#include "build_config.h"
#include "custom_pet.h"
#include "action_editor.h"
#include "ocean.h"
#include "renewed_pet.h"
#include "handoff.h"
#include "update_window.h"
#include "trust_store.h"
#include "transaction.h"
#include "zip.h"
#include <commdlg.h>
#include <windowsx.h>
#include <commctrl.h>
#include <random>
#include <memory>
using namespace pet;
namespace {
constexpr UINT TRAY=WM_APP+1;
enum Command{Countdown=100,Interval,Alarm,Pause,Stop,Reset,Acknowledge,Snooze5,Snooze10,Snooze15,SnoozeCustom,Cat,Dog,Rename,Size,Opacity,Top,Pass,Visibility,Preferences,CheckUpdate,About,Exit,ImportCustom,SelectCustom,Parrot,Panda,Coat0=200,Quick0=300,CustomAction0=400,Profile0=500,Language0=600,DialogueLanguage0=610,ToggleDialogue=620,Dolphin=630,WhaleShark=631,ToggleNameplate=640,ResetName=641,Mode0=700};
const TextId cats[]={TextId::S037,TextId::S038,TextId::S039,TextId::S040,TextId::S041};
const TextId dogs[]={TextId::S042,TextId::S043,TextId::S044,TextId::S045,TextId::S046};
const TextId parrots[]={TextId::S047,TextId::S048,TextId::S049,TextId::S050,TextId::S051};
const TextId dolphins[]={TextId::DolphinBlue,TextId::DolphinPink,TextId::DolphinGray};
const TextId whaleSharks[]={TextId::WhaleSharkDeep,TextId::WhaleSharkLight,TextId::WhaleSharkGray};
struct App{
 MenuLayer menuLayer;Nameplate nameplate;int previewOriginalOpacity=0;HWINEVENTHOOK foregroundHook=nullptr;HWND previousForeground=nullptr;HMONITOR previousPetMonitor=nullptr;bool previousTop=true;UiRect nameAnchor{24,24,96,96};PixelRect stableVisible;std::tuple<const ImageClip*,int,int,int,Action,size_t,size_t> anchorKey{};bool anchorCached=false;int previewOriginalScale=0;POINT previewOriginalPosition{};
 HINSTANCE instance;HWND window=nullptr,bubble=nullptr;HANDLE singleton=nullptr;Config c;fs::path config,configRecovery;Json dialogue;UpdateClient updates;UpdateHandoff handoff;PetUpdateWindow updateWindow;AboutWindow aboutWindow;std::mt19937 rng{std::random_device{}()};std::string lastLine;Sprite sprite;CustomPet custom;OceanPet ocean;RenewedPet renewed;Spring spring;Action action=Action::Idle;
 bool speechIsDialogue=false;bool manualHidden=false,fullscreenHidden=false,dragging=false,pressed=false,tracking=false,updating=false,loading=true,manualCheck=false,updateNotified=false,downloadRequested=false,formOpen=false,menuOpen=false,reminderQueued=false;
 POINT down{},lastMouse{},startPos{};int dpi=96,canvas=144,petSize=96,clicks=0,strokes=0,lastDirection=0;ULONGLONG nextMarineSpecial=0,nextUpdateNotice=0,lastInput=0,actionEnd=0,bubbleEnd=0,lastClick=0,lastStroke=0,nextIdle=0,frameTime=0,lastMove=0;std::wstring speech,handoffError;std::string observedStatus;fs::path observedStage;UINT taskbar=0;HDC memory=nullptr;HBITMAP bitmap=nullptr,oldBitmap=nullptr;uint32_t*bits=nullptr;int allocated=0;
 explicit App(HINSTANCE i):instance(i){}
 ~App(){if(foregroundHook)UnhookWinEvent(foregroundHook);if(memory){SelectObject(memory,oldBitmap);DeleteObject(bitmap);DeleteDC(memory);}if(singleton)CloseHandle(singleton);}
 bool hasReminder()const{return c.timers.pending();}
 TextId modeLabel(Mode mode)const{return mode==Mode::Alarm?TextId::S063:mode==Mode::Interval?TextId::S062:TextId::S061;}
 Timer&reminderTask(){return c.timers.at(c.timers.front()<0?c.timers.selected:Mode(c.timers.front()));}
 void refreshReminders(){c.timers.reconcile();if(hasReminder())reminder();else{reminderQueued=false;speech.clear();bubbleEnd=0;ShowWindow(bubble,SW_HIDE);action=Action::Idle;actionEnd=0;}tray();persist();}
 bool hidden()const{return manualHidden||fullscreenHidden;}
 void error(const std::wstring&s){auto text=utf8(s);localizedMessage(window,instance,TextId::S016,[text]{return wide(currentSystemText(text));},false);}
 void persist(){if(loading)return;try{auto saved=c;if(previewOriginalOpacity)saved.opacity=previewOriginalOpacity;if(previewOriginalScale){saved.scale=previewOriginalScale;saved.x=previewOriginalPosition.x;saved.y=previewOriginalPosition.y;}saveConfig(config,saved,configRecovery);}catch(...){error(tw(TextId::S052));}}
 void rememberMonitor(){MONITORINFOEXW mi{};mi.cbSize=sizeof(mi);GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),reinterpret_cast<MONITORINFO*>(&mi));c.monitor=utf8(mi.szDevice);}
 void clampPosition(){RECT r{c.x,c.y,c.x+canvas,c.y+canvas};MONITORINFO mi{sizeof(mi)};GetMonitorInfoW(MonitorFromRect(&r,MONITOR_DEFAULTTONEAREST),&mi);c.x=std::clamp(c.x,int(mi.rcWork.left),std::max(int(mi.rcWork.left),int(mi.rcWork.right)-canvas));c.y=std::clamp(c.y,int(mi.rcWork.top),std::max(int(mi.rcWork.top),int(mi.rcWork.bottom)-canvas-(c.showNameplate?Nameplate::logicalHeight(c.scale,dpi)+MulDiv(2,dpi,96):0)));c.positioned=true;SetWindowPos(window,c.top?HWND_TOPMOST:HWND_NOTOPMOST,c.x,c.y,canvas,canvas,SWP_NOACTIVATE|(menuLayer.active()?SWP_NOZORDER:0));rememberMonitor();}
 void sizeChanged(){dpi=int(GetDpiForWindow(window));petSize=MulDiv(96*c.scale/100,dpi,96);canvas=petSize*3/2;clampPosition();render();placeBubble();}
 void tray(bool add=false){NOTIFYICONDATAW n{};n.cbSize=sizeof(n);n.hWnd=window;n.uID=1;n.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP;n.uCallbackMessage=TRAY;n.hIcon=brandIcon(GetSystemMetricsForDpi(SM_CXSMICON,dpi));auto tip=wide(c.name)+L" · DesktopPet";if(hasReminder())tip+=tw(TextId::S053);else if(updates.busy())tip+=tw(TextId::S054)+std::to_wstring(updates.progress())+L"%";wcsncpy_s(n.szTip,tip.c_str(),_TRUNCATE);Shell_NotifyIconW(add?NIM_ADD:NIM_MODIFY,&n);if(add){n.uVersion=NOTIFYICON_VERSION_4;Shell_NotifyIconW(NIM_SETVERSION,&n);}}
 std::wstring say(const std::string&state){return wide(chooseDialogue(dialogueCandidates(c,dialogue,state),lastLine,rng()));}
 std::string dialogueState()const{return action==Action::Wake?"wake":action==Action::Sleep?"sleep":action==Action::Angry?"angry":action==Action::Pet?"pet":action==Action::Drag?"drag":action==Action::Happy?"happy":"general";}
 void speakDialogue(const std::string&state){if(menuOpen||hasReminder())return;speak(say(state));speechIsDialogue=true;}
 void refreshLanguage(){
  handoffError=wide(currentSystemText(utf8(handoffError)));
  if(hasReminder())speech=std::wstring(tw(modeLabel(reminderTask().mode)))+L" · "+wide(reminderTask().title)+L"\n"+(reminderTask().text.empty()?say("remind"):wide(reminderTask().text))+L"\n"+tw(TextId::ReminderConfirmHint);
  else if(!speech.empty()){if(speechIsDialogue&&(c.pet==2||c.dialogueLanguage=="follow-ui")){lastLine.clear();speech=say(dialogueState());}else if(!speechIsDialogue)speech=wide(currentSystemText(utf8(speech)));}
  tray();updateNameplate();placeBubble();
 }
 void changeLanguage(Language language){auto previous=uiLanguage();c.setLanguage(language);setUILanguage(c.language);broadcastLanguageChange(previous);persist();}
 void changeDialogue(const std::string&language){c.dialogueLanguage=language;lastLine.clear();if(speechIsDialogue&&!speech.empty()&&!hasReminder()){speech=say(dialogueState());placeBubble();}persist();}
 void speak(const std::wstring&s){if(menuOpen||hasReminder())return;speechIsDialogue=false;speech=s;bubbleEnd=GetTickCount64()+4000;placeBubble();}
 void interact(Action a,const std::string&state,int ms=2400){if(menuOpen)return;lastInput=GetTickCount64();if(hasReminder()||dragging)return;action=a;actionEnd=lastInput+(c.pet==2?custom.duration(a,unsigned(ms)):oceanSpecies(c.pet)?ocean.duration(a,unsigned(ms)):unsigned(ms));speakDialogue(state);scheduleFrame();}
 void reminder(){if(dragging||menuOpen){reminderQueued=true;return;}reminderQueued=false;action=Action::Remind;actionEnd=GetTickCount64()+1800;speechIsDialogue=false;speech=std::wstring(tw(modeLabel(reminderTask().mode)))+L" · "+wide(reminderTask().title)+L"\n"+(reminderTask().text.empty()?say("remind"):wide(reminderTask().text))+L"\n"+tw(TextId::ReminderConfirmHint);bubbleEnd=0;placeBubble();tray();scheduleFrame();}
 void acknowledge(){c.timers.acknowledge();refreshReminders();if(!hasReminder()){action=Action::Happy;actionEnd=GetTickCount64()+1500;}scheduleFrame();}
 void defer(int minutes){c.timers.defer(now(),minutes);refreshReminders();scheduleFrame();}
 void scheduleFrame(){if(hidden()){KillTimer(window,2);return;}UINT ms=(dragging?16:spring.moving()?25:action==Action::Sleep?(c.pet==2?250:500):83);SetTimer(window,2,ms,nullptr);}
 void loadOcean(){
  int identity=oceanAppearance(c.pet,c.coat);if(identity<0){ocean.clear();return;}if(ocean.identity()==identity)return;
  auto resource=FindResourceW(instance,MAKEINTRESOURCEW(201+identity),RT_RCDATA);if(!resource)throw std::runtime_error(tr(TextId::OceanResourceError));
  DWORD size=SizeofResource(instance,resource);auto loaded=LoadResource(instance,resource);auto bytes=static_cast<const unsigned char*>(LockResource(loaded));if(!bytes||!size||size>4*1024*1024)throw std::runtime_error(tr(TextId::OceanResourceError));
  ocean.select(c.pet,c.coat,std::vector<unsigned char>(bytes,bytes+size));
 }
 void loadRenewed(){
  int identity=renewedAppearance(c.pet,c.coat);if(identity<0){renewed.clear();return;}if(renewed.identity()==identity)return;
  auto resource=FindResourceW(instance,MAKEINTRESOURCEW(301+identity),RT_RCDATA);if(!resource)throw std::runtime_error(tr(TextId::BuiltinResourceError));DWORD size=SizeofResource(instance,resource);auto loaded=LoadResource(instance,resource);auto bytes=static_cast<const unsigned char*>(LockResource(loaded));if(!bytes||!size||size>4*1024*1024)throw std::runtime_error(tr(TextId::BuiltinResourceError));renewed.select(c.pet,c.coat,std::vector<unsigned char>(bytes,bytes+size));
 }
 void render(){if(hidden()||!window)return;auto tick=GetTickCount64();double t=tick/1000.;double dt=frameTime?std::min(.04,(tick-frameTime)/1000.):.016;frameTime=tick;if(!dragging)spring.step(dt);loadOcean();loadRenewed();
  if(allocated!=canvas){if(memory){SelectObject(memory,oldBitmap);DeleteObject(bitmap);DeleteDC(memory);}BITMAPINFO bi{};bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=canvas;bi.bmiHeader.biHeight=-canvas;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;bi.bmiHeader.biCompression=BI_RGB;memory=CreateCompatibleDC(nullptr);bitmap=CreateDIBSection(memory,&bi,DIB_RGB_COLORS,reinterpret_cast<void**>(&bits),nullptr,0);if(!bitmap||!memory)throw std::runtime_error(tr(TextId::S060));oldBitmap=static_cast<HBITMAP>(SelectObject(memory,bitmap));allocated=canvas;}
  std::fill(bits,bits+canvas*canvas,0u);double sx=spring.x,sy=spring.y;if(c.pet==2&&action==Action::Stretch){sx*=1.16;sy*=.85;}if(c.pet==2&&action==Action::Belly){sx*=1.08;sy*=.94;}if(action==Action::Sleep&&!oceanSpecies(c.pet))sy*=.93;
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
  if(oceanSpecies(c.pet)){
   image=&ocean.clip();imageFrame=ocean.frame(action,tick);
   // Marine flippers, tail, sleep breathing and expressions are authored in each clip.
  }
  if(renewed.identity()>=0){image=&renewed.clip();imageFrame=renewed.frame(action,tick);sy*=1.+std::sin(t*(action==Action::Sleep?1.5:2.5))*.015;if(action==Action::Angry)sx*=1.+std::sin(t*18)*.018;}
  // Stable per-animation alpha union ignores transparent canvas and per-frame breathing/jumps.
  size_t first=0,count=image?image->frames.size():0;
  if(image&&oceanSpecies(c.pet)&&count==120){first=size_t(OceanPet::actionSlot(action))*12;count=12;if(action==Action::Wake){first=0;count=84;}}
  else if(image&&renewed.identity()>=0){
   if(image->frames.size()==30&&(action==Action::Yawn||action==Action::Belly)){first=action==Action::Yawn?6:18;count=12;}
   else if(action==Action::Sleep){first=2;count=1;}else if(action==Action::Blink||action==Action::Shy){first=1;count=1;}
   else if(action==Action::Belly){first=5;count=1;}else if(action==Action::Stretch){first=0;count=5;}
   else if(action==Action::Wake||action==Action::Yawn||((action==Action::Happy||action==Action::Pet)&&c.pet==3)){first=0;count=6;}
   else{first=0;count=2;}
  }
  auto key=std::make_tuple(image,c.pet,c.coat,c.activeCustom,action,first,count);
  if(!anchorCached||key!=anchorKey){stableVisible=image?image->visibleBounds(first,count):PixelRect{};anchorKey=key;anchorCached=true;}
  double aspect=image?double(image->crop.w)/image->crop.h:1.;
  double anchorScaleX=spring.x,anchorScaleY=spring.y;
  if(c.pet==2&&action==Action::Stretch){anchorScaleX*=1.16;anchorScaleY*=.85;}if(c.pet==2&&action==Action::Belly){anchorScaleX*=1.08;anchorScaleY*=.94;}
  if(action==Action::Sleep&&!oceanSpecies(c.pet))anchorScaleY*=.93;
  int anchorW=std::max(1,int(petSize*anchorScaleX*std::min(1.,aspect))),anchorH=std::max(1,int(petSize*anchorScaleY/std::max(1.,aspect)));
  nameAnchor=nameplateAnchor(stableVisible,image?image->crop:PixelRect{},canvas,anchorW,anchorH);
  int w=std::max(1,int(petSize*sx*std::min(1.,aspect))),h=std::max(1,int(petSize*sy/std::max(1.,aspect))),ox=(canvas-w)/2,oy=(canvas-h)/2;int jump=0;if(action==Action::Remind&&tick<actionEnd)jump=int(std::abs(std::sin((actionEnd-tick)/1800.*3.14159265*3))*petSize*.16);oy-=jump;
  int sway=renewed.identity()>=0&&action!=Action::Sleep?int(std::lround(std::sin(t*2.8)*2.)):0;
  if(oceanSpecies(c.pet)||renewed.identity()>=0)blitNativePet(*image,imageFrame,bits,canvas,ox,oy,w,h,sway);
  else for(int y=0;y<h;++y){int yy=oy+y;if(yy<0||yy>=canvas)continue;for(int x=0;x<w;++x){int xx=ox+x;if(xx<0||xx>=canvas)continue;bits[yy*canvas+xx]=image?image->sample(imageFrame,image->crop.x+(x+.5)*image->crop.w/w-.5,image->crop.y+(y+.5)*image->crop.h/h-.5):sprite.pixels[std::min(47,y*48/h)*48+std::min(47,x*48/w)];}}
  POINT dest{c.x,c.y},src{0,0};SIZE sz{canvas,canvas};BLENDFUNCTION blend{AC_SRC_OVER,0,BYTE(c.opacity*255/100),AC_SRC_ALPHA};UpdateLayeredWindow(window,nullptr,&dest,&sz,memory,&src,0,&blend,ULW_ALPHA);updateNameplate();
 }
 void updateNameplate(){std::optional<UiRect>avoid;if(bubble&&IsWindowVisible(bubble)&&!speech.empty()){RECT r{};GetWindowRect(bubble,&r);avoid=UiRect{int(r.left),int(r.top),int(r.right-r.left),int(r.bottom-r.top)};}nameplate.place(c,canvas,dpi,hidden()||menuLayer.active()||(menuOpen&&!formOpen),avoid,UiRect{c.x+nameAnchor.x,c.y+nameAnchor.y,nameAnchor.w,nameAnchor.h});}
 void placeBubble(){
  if(!bubble)return;
  if(hidden()||menuOpen||menuLayer.active()||speech.empty()||dragging){ShowWindow(bubble,SW_HIDE);return;}
  MONITORINFO mi{sizeof(mi)};
  GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&mi);
  const int workWidth=std::max(1,int(mi.rcWork.right-mi.rcWork.left));
  const int workHeight=std::max(1,int(mi.rcWork.bottom-mi.rcWork.top));
  const int d=int(GetDpiForWindow(window));
  const int padding=std::min({MulDiv(12,d,96),(workWidth-1)/2,(workHeight-1)/2});
  const int maxWidth=std::min(workWidth,MulDiv(480,d,96));
  HDC dc=GetDC(bubble);
  HFONT font=CreateFontW(-MulDiv(14,d,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());
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
  if(y<mi.rcWork.top){y=c.y+canvas+MulDiv(2,d,96);if(auto plate=nameplate.bounds())y=plate->y+plate->h+MulDiv(4,d,96);}
  y=std::clamp(y,int(mi.rcWork.top),int(mi.rcWork.bottom)-height);
  x=std::clamp(x,int(mi.rcWork.left),int(mi.rcWork.right)-width);
  if(auto plate=nameplate.bounds();plate&&overlaps(*plate,{x,y,width,height}))nameplate.hide();
  SetWindowPos(bubble,c.top?HWND_TOPMOST:HWND_NOTOPMOST,x,y,width,height,SWP_NOACTIVATE|SWP_SHOWWINDOW);
  SetWindowRgn(bubble,CreateRoundRectRgn(0,0,width+1,height+1,MulDiv(14,d,96),MulDiv(14,d,96)),TRUE);
  InvalidateRect(bubble,nullptr,TRUE);
 }

 static void CALLBACK foregroundChanged(HWINEVENTHOOK,DWORD,HWND window,LONG,LONG,DWORD,DWORD){if(!window)return;auto pet=FindWindowW(L"DesktopPetWindow",L"DesktopPet");DWORD pid=0;if(pet){GetWindowThreadProcessId(pet,&pid);if(pid==GetCurrentProcessId())PostMessageW(pet,WM_APP+30,0,0);}}
 bool isFullscreen(){auto w=GetForegroundWindow();if(!w||w==window||w==bubble||w==GetShellWindow()||!IsWindowVisible(w)||IsIconic(w))return false;wchar_t cls[128]{};GetClassNameW(w,cls,128);if(wcscmp(cls,L"Progman")==0||wcscmp(cls,L"WorkerW")==0||wcsstr(cls,L"TrayWnd"))return false;RECT r{};if(!GetWindowRect(w,&r))return false;auto foreign=MonitorFromWindow(w,MONITOR_DEFAULTTONEAREST),own=MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST);MONITORINFO a{sizeof(a)},b{sizeof(b)};if(!GetMonitorInfoW(foreign,&a)||!GetMonitorInfoW(own,&b))return false;auto rect=[](RECT v){return UiRect{int(v.left),int(v.top),int(v.right-v.left),int(v.bottom-v.top)};};auto style=GetWindowLongPtrW(w,GWL_STYLE);return fullscreenOnPetMonitor(rect(r),rect(a.rcMonitor),rect(b.rcMonitor),IsZoomed(w)!=FALSE,(style&(WS_CAPTION|WS_THICKFRAME))!=0,true);}
 void maintainZOrder(bool force=false){if(menuLayer.active())return;auto foreground=GetForegroundWindow();auto monitor=MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST);bool actualTop=(GetWindowLongPtrW(window,GWL_EXSTYLE)&WS_EX_TOPMOST)!=0;if(!force&&foreground==previousForeground&&monitor==previousPetMonitor&&previousTop==c.top&&actualTop==c.top)return;previousForeground=foreground;previousPetMonitor=monitor;previousTop=c.top;SetWindowPos(window,c.top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);nameplate.invalidate();updateNameplate();}
 void visibility(){if(menuLayer.active()){render();scheduleFrame();return;}maintainZOrder();bool was=fullscreenHidden;fullscreenHidden=c.autoHide&&isFullscreen();if(hidden()){ShowWindow(window,SW_HIDE);ShowWindow(bubble,SW_HIDE);nameplate.hide();KillTimer(window,2);}else{ShowWindow(window,SW_SHOWNOACTIVATE);if(was&&hasReminder())reminder();render();placeBubble();scheduleFrame();}}
 void tick(){auto t=GetTickCount64();int previous=c.timers.front();if(c.timers.tick(now())){lastInput=t;if(previous!=c.timers.front()||previous<0)reminder();tray();persist();}visibility();if(updates.busy())tray();if(!menuOpen&&!hasReminder()&&bubbleEnd&&t>=bubbleEnd){speech.clear();bubbleEnd=0;ShowWindow(bubble,SW_HIDE);}if(c.autoUpdate&&updates.enabled()&&!updates.busy()&&(now()-c.lastCheck>=86400||now()<c.lastCheck)){c.lastCheck=now();updates.check();persist();}if(!menuOpen)updateTick();}
 void animate(){if(menuOpen){if(!hasReminder()&&actionEnd&&GetTickCount64()>=actionEnd){action=Action::Idle;actionEnd=0;}render();scheduleFrame();return;}auto t=GetTickCount64();if(!dragging&&!hasReminder()){if(t-lastInput>=300000){if(action!=Action::Sleep){action=Action::Sleep;actionEnd=0;speakDialogue("sleep");}}else if(actionEnd&&t>=actionEnd){action=Action::Idle;actionEnd=0;}if(!actionEnd&&action!=Action::Sleep&&t>=nextIdle){static Action idle[]={Action::Blink,Action::Yawn,Action::Stretch,Action::Belly,Action::Happy};action=idle[rng()%5];if(oceanSpecies(c.pet)&&(action==Action::Yawn||action==Action::Stretch||action==Action::Belly)){if(t<nextMarineSpecial)action=Action::Blink;else nextMarineSpecial=t+90000+rng()%60000;}actionEnd=t+(c.pet==2?custom.duration(action,action==Action::Blink?250u:2300u):oceanSpecies(c.pet)?ocean.duration(action,action==Action::Blink?250u:2300u):(action==Action::Blink?250u:2300u));nextIdle=t+15000+rng()%30000;}}render();scheduleFrame();}
 void applyPass(){auto ex=GetWindowLongPtrW(window,GWL_EXSTYLE);if(c.pass)ex|=WS_EX_TRANSPARENT;else ex&=~LONG_PTR(WS_EX_TRANSPARENT);SetWindowLongPtrW(window,GWL_EXSTYLE,ex);SetWindowLongPtrW(bubble,GWL_EXSTYLE,(GetWindowLongPtrW(bubble,GWL_EXSTYLE)&~LONG_PTR(WS_EX_TRANSPARENT))|(c.pass?WS_EX_TRANSPARENT:0));}
 void showForm(Form&f){if(formOpen)return;formOpen=true;try{f.show(window,instance);}catch(...){formOpen=false;throw;}formOpen=false;lastInput=GetTickCount64();}
 void timerForm(Mode mode){if(formOpen)return;formOpen=true;c.timers.selected=mode;try{TimerWindow editor(c.timers.at(mode),mode,[&]{refreshReminders();});editor.show(window,instance);}catch(...){formOpen=false;throw;}formOpen=false;lastInput=GetTickCount64();}
 void opacityForm(){if(formOpen)return;formOpen=true;previewOriginalOpacity=c.opacity;bool accepted=false;try{ScaleWindow editor;accepted=editor.show(window,instance,c.opacity,[&](int value){c.opacity=value;render();},true);}catch(...){c.opacity=previewOriginalOpacity;previewOriginalOpacity=0;formOpen=false;if(IsWindow(window))render();throw;}if(!accepted)c.opacity=previewOriginalOpacity;previewOriginalOpacity=0;formOpen=false;if(IsWindow(window))render();persist();}
 void scaleForm(){if(formOpen)return;formOpen=true;previewOriginalScale=c.scale;previewOriginalPosition={c.x,c.y};bool accepted=false;try{ScaleWindow editor;accepted=editor.show(window,instance,c.scale,[&](int value){c.scale=value;sizeChanged();});}catch(...){c.scale=previewOriginalScale;c.x=previewOriginalPosition.x;c.y=previewOriginalPosition.y;previewOriginalScale=0;formOpen=false;if(IsWindow(window))sizeChanged();throw;}if(!accepted){c.scale=previewOriginalScale;c.x=previewOriginalPosition.x;c.y=previewOriginalPosition.y;}previewOriginalScale=0;formOpen=false;if(IsWindow(window))sizeChanged();persist();lastInput=GetTickCount64();}
 void oneNumber(TextId title,int value,int lo,int hi,const std::function<void(int)>&save){Form f;f.titleId=title;f.fields.emplace_back(TextId::ValueRange,std::to_wstring(value));f.fields[0].labelFactory=[lo,hi]{return wf(TextId::ValueRange,{std::to_wstring(lo),std::to_wstring(hi)});};f.apply=[&](auto&v){save(number(v[0].value,lo,hi));persist();return true;};showForm(f);}
 void preferences(){Form f;f.titleId=TextId::S081;f.fields={{TextId::S082,c.autoHide?L"1":L"0",true},{TextId::S083,c.startup?L"1":L"0",true},{TextId::S084,c.autoUpdate?L"1":L"0",true},{TextId::S085,std::to_wstring(c.timers.at(c.timers.selected).snoozeMinutes)}};f.fields[3].labelFactory=[&]{return std::wstring(tw(modeLabel(c.timers.selected)))+L" · "+tw(TextId::S085);};f.apply=[&](auto&v){int snooze=number(v[3].value,1,1440);bool startup=v[1].value==L"1";setStartup(startup);c.startup=startup;c.autoHide=v[0].value==L"1";c.autoUpdate=v[2].value==L"1";c.timers.at(c.timers.selected).snoozeMinutes=snooze;visibility();persist();return true;};showForm(f);}
 void selectPet(int pet){if(!c.selectBuiltin(pet,c.coat))return;ocean.clear();renewed.clear();custom=CustomPet{};lastLine.clear();if(!hasReminder()){speech.clear();ShowWindow(bubble,SW_HIDE);}render();tray();persist();}
 void selectProfile(int slot){
  CustomPet next;std::string failure;
  if(slot<0||slot>=3||!next.load(c.profiles[size_t(slot)].files,&failure)){error(failure.empty()?tw(TextId::S087):wide(failure));return;}
  if(!c.selectCustom(slot))return;ocean.clear();renewed.clear();custom=std::move(next);lastLine.clear();if(!hasReminder()){speech.clear();ShowWindow(bubble,SW_HIDE);}render();tray();persist();
 }
 void editDialogue(int slot){
  auto&profile=c.profiles[size_t(slot)];Form editor;editor.resizable=editor.multiEditor=true;editor.logicalWidth=c.dialogueEditorWidth;editor.logicalHeight=c.dialogueEditorHeight;editor.titleId=TextId::EditDialogue;editor.helpId=TextId::DialogueHelp;editor.tabIds={TextId::ZhTW,TextId::EnUS,TextId::JaJP};editor.selectedTab=int(c.language);
  std::array<std::array<std::wstring,8>,3>buffers;const TextId labels[]={TextId::DialogueGeneral,TextId::DialogueHappy,TextId::DialoguePet,TextId::DialogueAngry,TextId::DialogueDrag,TextId::DialogueSleep,TextId::DialogueWake,TextId::DialogueRemind};
  for(int language=0;language<3;++language)for(size_t state=0;state<8;++state){std::string lines;for(auto&line:validDialogueList(profile.dialogueFor(Language(language))[dialogueStates[state]])){if(!lines.empty())lines+="\r\n";lines+=line;}buffers[size_t(language)][state]=wide(lines);}
  for(size_t state=0;state<8;++state){editor.fields.emplace_back(labels[state],buffers[size_t(c.language)][state]);auto&field=editor.fields.back();field.multiline=true;field.textLimit=9000;field.multilineHeight=100;}
  editor.fields.emplace_back(TextId::DialogueEnabled,profile.dialogueEnabled?L"1":L"0",true);
  auto capture=[&](int language){for(size_t i=0;i<8;++i)buffers[size_t(language)][i]=controlText(editor.fields[i].input);};
  editor.onTab=[&](int previous,int next){capture(previous);for(size_t i=0;i<8;++i)SetWindowTextW(editor.fields[i].input,buffers[size_t(next)][i].c_str());};
  editor.preview=[&]{editor.completeIME();size_t state=size_t(std::clamp(editor.activeField,0,7));auto candidates=dialogueLines(utf8(controlText(editor.fields[state].input)),true);if(candidates.empty()){Config previewConfig=c;previewConfig.pet=2;previewConfig.activeCustom=slot;previewConfig.language=Language(editor.selectedTab);previewConfig.dialogueEnabled=true;previewConfig.profiles[size_t(slot)].dialogueEnabled=true;previewConfig.profiles[size_t(slot)].setDialogue(previewConfig.language,dialogueStates[state],Json::Array{});candidates=dialogueCandidates(previewConfig,dialogue,dialogueStates[state]);}auto preview=chooseDialogue(candidates,lastLine,rng());localizedMessage(editor.hwnd,instance,TextId::Preview,[preview]{return wide(preview);},false);};
  editor.apply=[&](auto&fields){capture(editor.selectedTab);DialogueBuffers serialized;for(int language=0;language<3;++language)for(size_t state=0;state<8;++state)serialized[size_t(language)][state]=utf8(buffers[size_t(language)][state]);auto candidate=editedDialogues(profile,serialized,fields[8].value==L"1");Config staged=c;staged.profiles[size_t(slot)]=candidate;if(staged.json().dump().size()>1024*1024-8192)throw std::runtime_error(tr(TextId::DialogueCapacity));profile=std::move(candidate);lastLine.clear();if(c.pet==2&&c.activeCustom==slot&&!hasReminder()){speech.clear();ShowWindow(bubble,SW_HIDE);}persist();return true;};showForm(editor);c.dialogueEditorWidth=editor.logicalWidth;c.dialogueEditorHeight=editor.logicalHeight;persist();
 }
 void manageActions(int slot){if(!c.profiles[size_t(slot)].ready()){error(tw(TextId::IdleRequired));return;}if(formOpen)return;formOpen=true;try{auto candidate=c.profiles[size_t(slot)];if(editActionAssets(window,instance,candidate)){Config staged=c;staged.profiles[size_t(slot)]=candidate;if(staged.json().dump().size()>1024*1024-8192)throw std::runtime_error(tr(TextId::DialogueCapacity));CustomPet next;std::string failure;if(c.pet==2&&c.activeCustom==slot&&!next.load(candidate.files,&failure))throw std::runtime_error(failure);c.profiles[size_t(slot)]=std::move(candidate);if(c.pet==2&&c.activeCustom==slot){custom=std::move(next);anchorCached=false;render();}persist();}}catch(...){formOpen=false;throw;}formOpen=false;}

 void renameProfile(int slot){
  Form f;f.titleFactory=[slot]{return wf(TextId::ProfileTitle,{std::to_wstring(slot+1)});};f.nameMode=true;f.fields.emplace_back(TextId::S089,wide(c.profiles[size_t(slot)].name));
  f.apply=[&](auto&fields){auto value=fields[0].value;validatePetName(utf8(value));c.profiles[size_t(slot)].name=utf8(value);c.profiles[size_t(slot)].nameDefault=false;if(c.pet==2&&c.activeCustom==slot)c.syncName();updateNameplate();tray();persist();return true;};showForm(f);
 }
 void importCustom(int state,int slot=-1){
  if(slot<0)slot=c.activeCustom;
  if(state>0&&!c.profiles[size_t(slot)].ready()){error(tw(TextId::S091));return;}
  wchar_t file[32768]{};OPENFILENAMEW picker{};picker.lStructSize=sizeof(picker);picker.hwndOwner=window;
  std::wstring filter=tw(TextId::S092);filter.push_back(0);filter+=L"*.png;*.gif";filter.push_back(0);filter.push_back(0);picker.lpstrFilter=filter.c_str();picker.lpstrFile=file;picker.nMaxFile=32768;
  picker.lpstrTitle=tw(TextId::S093);picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_DONTADDTORECENT;
  if(!GetOpenFileNameW(&picker)){if(CommDlgExtendedError())error(tw(TextId::S094));return;}
  auto bytes=readBytes(file,8*1024*1024);auto header=inspectImage(bytes);
  auto name=sha256(bytes)+(header.gif?".gif":".png");auto stored=customDirectory()/wide(name);rejectReparse(stored);
  atomicWrite(stored,bytes.data(),bytes.size());auto decoded=decodeCustom(stored);
  if(state==0){
   Form f;f.titleId=TextId::S095;auto proposed=fs::path(file).stem().wstring();if(!petNameMetrics(utf8(proposed)).valid)proposed=tw(TextId::S002);
   f.nameMode=true;f.fields.emplace_back(TextId::S089,proposed);
   f.apply=[&](auto&fields){auto text=fields[0].value;validatePetName(utf8(text));
    c.profiles[size_t(slot)].files[customStates[0]]=name;c.profiles[size_t(slot)].fileLabels[customStates[0]]=clipUtf8(utf8(fs::path(file).filename().wstring()),240);c.profiles[size_t(slot)].name=utf8(text);c.profiles[size_t(slot)].nameDefault=false;c.selectCustom(slot);ocean.clear();renewed.clear();custom=CustomPet{};custom.load(c.profiles[size_t(slot)].files);anchorCached=false;render();tray();persist();return true;};showForm(f);
  }else{c.profiles[size_t(slot)].files[customStates[state]]=name;anchorCached=false;if(c.pet==2&&c.activeCustom==slot)custom.set(state,std::move(decoded));render();persist();}
 }
 void command(UINT id){if(id>=Mode0&&id<Mode0+48){Mode mode=Mode((id-Mode0)/16);int operation=int(id-Mode0)%16;c.timers.selected=mode;auto&task=c.timers.at(mode);if(operation==0){timerForm(mode);return;}if(operation==1){if(task.paused)task.resume(now());else task.pause(now());}else if(operation==2)task.stop();else if(operation==3)task.start(now());refreshReminders();return;}if(id>=Language0&&id<Language0+3){changeLanguage(Language(id-Language0));return;}if(id>=DialogueLanguage0&&id<DialogueLanguage0+4){changeDialogue(id==DialogueLanguage0?"follow-ui":languageCode(Language(id-DialogueLanguage0-1)));return;}if(id>=Profile0&&id<Profile0+48){int slot=int(id-Profile0)/16,action=int(id-Profile0)%16;if(action==0)selectProfile(slot);else if(action==1)renameProfile(slot);else if(action==2){if(localizedMessage(window,instance,TextId::S097,[]{return std::wstring(tw(TextId::S096));},true)){bool current=c.pet==2&&c.activeCustom==slot;c.clearProfile(slot);if(current)custom=CustomPet{};lastLine.clear();if(!hasReminder()){speech.clear();ShowWindow(bubble,SW_HIDE);}render();tray();persist();}}else if(action==10)editDialogue(slot);else if(action==12)manageActions(slot);else if(action==11){c.profiles[size_t(slot)].name=std::string(tr(TextId::S004,Language::ZhTW))+std::to_string(slot+1);c.profiles[size_t(slot)].nameDefault=true;if(c.pet==2&&c.activeCustom==slot)c.syncName();tray();updateNameplate();persist();}else if(action>=3&&action<=9)importCustom(action-3,slot);return;}if(id>=CustomAction0&&id<CustomAction0+7){importCustom(int(id-CustomAction0));return;}lastInput=GetTickCount64();if(id>=Coat0&&id<Coat0+5){if(id>=Coat0+UINT(builtinCoats(c.pet)))return;c.selectBuiltin(c.pet,int(id-Coat0));ocean.clear();renewed.clear();lastLine.clear();if(!hasReminder()){speech.clear();ShowWindow(bubble,SW_HIDE);}render();persist();return;}if(id>=Quick0&&id<Quick0+5){int mins[]={5,15,25,30,60};auto&task=c.timers.at(Mode::Countdown);task.interval=mins[id-Quick0]*60;task.start(now());c.timers.selected=Mode::Countdown;refreshReminders();return;}
  switch(id){case ToggleNameplate:c.showNameplate=!c.showNameplate;clampPosition();updateNameplate();placeBubble();persist();break;case ResetName:c.resetName();tray();updateNameplate();persist();break;case ToggleDialogue:c.dialogueEnabled=!c.dialogueEnabled;if(!hasReminder()&&speechIsDialogue){speech.clear();ShowWindow(bubble,SW_HIDE);}persist();break;case Countdown:timerForm(Mode::Countdown);break;case Interval:timerForm(Mode::Interval);break;case Alarm:timerForm(Mode::Alarm);break;case Pause:if(c.timers.at(c.timers.selected).paused)c.timers.at(c.timers.selected).resume(now());else c.timers.at(c.timers.selected).pause(now());refreshReminders();break;case Stop:c.timers.at(c.timers.selected).stop();refreshReminders();break;case Reset:c.timers.at(c.timers.selected).start(now());refreshReminders();break;case Acknowledge:if(localizedMessage(window,instance,TextId::S099,[&]{return wide(reminderTask().title)+L"\n\n"+wide(reminderTask().text)+tw(TextId::S098);},true))acknowledge();break;case Snooze5:defer(5);break;case Snooze10:defer(10);break;case Snooze15:defer(15);break;case SnoozeCustom:oneNumber(TextId::S100,reminderTask().snoozeMinutes,1,1440,[&](int n){defer(n);});break;case Cat:case Dog:selectPet(id==Dog?1:0);break;case ImportCustom:importCustom(0);break;case SelectCustom:selectProfile(c.activeCustom);break;case Parrot:selectPet(3);break;case Panda:selectPet(4);break;case Dolphin:selectPet(5);break;case WhaleShark:selectPet(6);break;case Rename:{Form f;f.nameMode=true;f.titleId=TextId::S101;f.fields.emplace_back(TextId::S102,wide(c.name));f.apply=[&](auto&v){validatePetName(utf8(v[0].value));c.rename(utf8(v[0].value));tray();updateNameplate();persist();return true;};showForm(f);break;}case Size:scaleForm();break;case Opacity:opacityForm();break;case Top:c.top=!c.top;clampPosition();placeBubble();persist();break;case Pass:c.pass=!c.pass;applyPass();persist();break;case Visibility:manualHidden=!manualHidden;visibility();if(!manualHidden&&hasReminder())reminder();break;case Preferences:preferences();break;case CheckUpdate:if(updates.busy()||handoff.pending()){auto info=updates.latest();if(!info.version.empty())offerUpdate(info);}else{manualCheck=true;updates.check();if(!updates.enabled()){error(wide(updates.message()));manualCheck=false;}}break;case About:aboutWindow.show(window,instance);break;case Exit:if(handoff.pending())handoff.cancel();DestroyWindow(window);break;}
 }
 void menu(POINT p){
  if(menuOpen)return;
  menuOpen=true;ShowWindow(bubble,SW_HIDE);nameplate.hide();scheduleFrame();
  const ULONGLONG opened=GetTickCount64(),previousEnd=bubbleEnd;
  strokes=clicks=lastDirection=0;
  if(pressed){pressed=dragging=false;ReleaseCapture();spring.vy=2;persist();}
  auto finish=[&]{
   menuOpen=false;
   if(!IsWindow(window)||!IsWindow(bubble))return;
   // A normal bubble keeps its remaining visible time across a cancelled menu.
   if(!hasReminder()&&bubbleEnd&&bubbleEnd==previousEnd)bubbleEnd+=GetTickCount64()-opened;
   if(hasReminder()&&reminderQueued)reminder();
   else{reminderQueued=false;placeBubble();}
   updateNameplate();scheduleFrame();
  };
  try{
  HMENU m=CreatePopupMenu();auto add=[&](UINT id,const wchar_t*s,UINT flags=0){AppendMenuW(m,MF_STRING|flags,id,s);};
  const TextId modeLabels[]={TextId::S119,TextId::S120,TextId::S121};for(int i=0;i<3;++i){auto&task=c.timers.at(Mode(i));HMENU group=CreatePopupMenu();auto state=task.pending?TextId::TimerPending:task.paused?TextId::TimerPaused:task.running?TextId::TimerRunning:TextId::TimerStopped;auto label=std::wstring(tw(modeLabels[i]))+L" · "+tw(state);AppendMenuW(group,MF_STRING,Mode0+i*16,tw(modeLabels[i]));AppendMenuW(group,MF_STRING|(task.running?0:MF_GRAYED),Mode0+i*16+1,tw(task.paused?TextId::S110:TextId::S111));AppendMenuW(group,MF_STRING|((task.running||task.pending||task.paused)?0:MF_GRAYED),Mode0+i*16+2,tw(TextId::S112));AppendMenuW(group,MF_STRING,Mode0+i*16+3,tw(TextId::S113));AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(group),label.c_str());}
  if(hasReminder()){add(0,wf(TextId::PendingCount,{std::to_wstring(c.timers.queue.size())}).c_str(),MF_GRAYED);std::wstring label=std::wstring(tw(TextId::S114))+tw(modeLabel(reminderTask().mode))+L" · "+wide(reminderTask().title);add(Acknowledge,label.c_str());add(Snooze5,tw(TextId::S115));add(Snooze10,tw(TextId::S116));add(Snooze15,tw(TextId::S117));add(SnoozeCustom,tw(TextId::S118));}
  AppendMenuW(m,MF_SEPARATOR,0,nullptr);HMENU quick=CreatePopupMenu();const TextId q[]={TextId::S122,TextId::S123,TextId::S124,TextId::S125,TextId::S126};for(int i=0;i<5;++i)AppendMenuW(quick,MF_STRING,Quick0+i,tw(q[i]));AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(quick),tw(TextId::S127));
  AppendMenuW(m,MF_SEPARATOR,0,nullptr);HMENU pets=CreatePopupMenu();AppendMenuW(pets,MF_STRING|(c.pet==0?MF_CHECKED:0),Cat,tw(TextId::S128));AppendMenuW(pets,MF_STRING|(c.pet==1?MF_CHECKED:0),Dog,tw(TextId::S129));AppendMenuW(pets,MF_STRING|(c.pet==3?MF_CHECKED:0),Parrot,tw(TextId::S130));AppendMenuW(pets,MF_STRING|(c.pet==4?MF_CHECKED:0),Panda,tw(TextId::S131));AppendMenuW(pets,MF_STRING|(c.pet==5?MF_CHECKED:0),Dolphin,tw(TextId::Dolphin));AppendMenuW(pets,MF_STRING|(c.pet==6?MF_CHECKED:0),WhaleShark,tw(TextId::WhaleShark));AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(pets),tw(TextId::S132));
  for(int slot=0;slot<3;++slot){auto&profile=c.profiles[size_t(slot)];HMENU group=CreatePopupMenu();UINT base=Profile0+slot*16;AppendMenuW(group,MF_STRING|(profile.ready()?0:MF_GRAYED)|(c.pet==2&&c.activeCustom==slot?MF_CHECKED:0),base,tw(TextId::S133));AppendMenuW(group,MF_STRING,base+1,tw(TextId::S134));AppendMenuW(group,MF_STRING,base+2,tw(TextId::S135));AppendMenuW(group,MF_STRING,base+3,tw(TextId::S136));AppendMenuW(group,MF_STRING|(profile.ready()?0:MF_GRAYED),base+12,tw(TextId::EditActions));if(!profile.ready())AppendMenuW(group,MF_STRING|MF_GRAYED,0,tw(TextId::IdleRequired));AppendMenuW(group,MF_STRING,base+10,tw(TextId::EditDialogue));AppendMenuW(group,MF_STRING,base+11,tw(TextId::ResetName));auto label=wf(TextId::CustomPrefix,{std::to_wstring(slot+1),wide(c.profileName(slot))})+(c.pet==2&&c.activeCustom==slot?tw(TextId::S143):profile.ready()?tw(TextId::S144):tw(TextId::S145));AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(group),label.c_str());}
  HMENU coats=CreatePopupMenu();for(int i=0;i<builtinCoats(c.pet);++i)AppendMenuW(coats,MF_STRING|(i==c.coat?MF_CHECKED:0),Coat0+i,tw(c.pet==5?dolphins[i]:c.pet==6?whaleSharks[i]:c.pet==3?parrots[i]:c.pet?dogs[i]:cats[i]));AppendMenuW(m,MF_POPUP|((c.pet==2||c.pet==4)?MF_GRAYED:0),reinterpret_cast<UINT_PTR>(coats),tw(TextId::S146));add(Rename,tw(TextId::S147));add(ResetName,tw(TextId::ResetName));add(ToggleNameplate,tw(TextId::ShowNameplate),c.showNameplate?MF_CHECKED:0);add(Size,tw(TextId::S148));add(Opacity,tw(TextId::S149));add(Top,tw(TextId::S150),c.top?MF_CHECKED:0);add(Pass,tw(TextId::S151),c.pass?MF_CHECKED:0);add(Visibility,manualHidden?tw(TextId::S152):tw(TextId::S153));add(Preferences,tw(TextId::S154));add(ToggleDialogue,tw(TextId::DialogueEnabled),c.dialogueEnabled?MF_CHECKED:0);HMENU languages=CreatePopupMenu(),talk=CreatePopupMenu();const TextId languageNames[]={TextId::ZhTW,TextId::EnUS,TextId::JaJP};for(int i=0;i<3;++i)AppendMenuW(languages,MF_STRING|(int(c.language)==i?MF_CHECKED:0),Language0+i,tw(languageNames[i]));AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(languages),tw(TextId::Language));AppendMenuW(talk,MF_STRING|(c.dialogueLanguage=="follow-ui"?MF_CHECKED:0),DialogueLanguage0,tw(TextId::FollowUI));for(int i=0;i<3;++i)AppendMenuW(talk,MF_STRING|(c.dialogueLanguage==languageCode(Language(i))?MF_CHECKED:0),DialogueLanguage0+i+1,tw(languageNames[i]));AppendMenuW(m,MF_POPUP,reinterpret_cast<UINT_PTR>(talk),tw(TextId::DialogueLanguage));std::wstring updateLabel=updates.busy()?tw(TextId::S155)+std::to_wstring(updates.progress())+tw(TextId::S156):tw(TextId::S157);add(CheckUpdate,updateLabel.c_str());add(About,tw(TextId::S158));add(Exit,tw(TextId::S159));
  UINT id=trackPetPopup(menuLayer,window,c.top,m,p);DestroyMenu(m);if(id)command(id);
  }catch(...){finish();throw;}
  finish();
 }
 void bindUpdateWindow(){
  updateWindow.bind([this]{if(handoff.pending())return;handoffError.clear();if(updates.phase()==UpdatePhase::Ready&&!updates.stage().empty())installUpdate(updates.stage());else{downloadRequested=true;updates.download();updateWindow.state(0,tw(TextId::S160),true);}},[this]{updateNotified=false;nextUpdateNotice=GetTickCount64()+3600000;},[this]{downloadRequested=false;if(handoff.pending()){handoff.reset();handoffError=tw(TextId::S161);updateWindow.state(0,handoffError,false);}else{updates.cancel();updateWindow.state(updates.progress(),tw(TextId::S162),true);}});
 }
 void updateTick(){
  if(handoff.pending()){
   int result=handoff.poll();updateWindow.state(100,tw(TextId::S163),true);
   if(result==1&&!formOpen&&!menuOpen){try{saveConfig(config,c,configRecovery);updates.cancel();updating=true;handoff.commit();DestroyWindow(window);}catch(const std::exception&e){handoff.reset();updating=false;handoffError=tw(TextId::S164)+wide(e.what());updateWindow.state(0,handoffError,false);}}
   else if(result<0){handoff.reset();std::string detail=result==-2?tr(TextId::S165):tr(TextId::S166);try{detail=readText(observedStage/L"handoff-error.txt",8192);}catch(...){}handoffError=wide(detail);updateWindow.state(0,handoffError,false);}
   return;
  }
  updateWindow.state(updates.progress(),handoffError.empty()?wide(updates.message()):handoffError,updates.busy());if(updates.busy())return;
  auto status=updates.message();auto info=updates.latest();if(status!=observedStatus||manualCheck){observedStatus=status;if(manualCheck&&!status.empty()){manualCheck=false;if(!info.version.empty())offerUpdate(info);else error(wide(status));}}
  if(!info.version.empty()&&!updateNotified&&GetTickCount64()>=nextUpdateNotice){updateNotified=true;speak(tw(TextId::S167)+wide(info.version)+tw(TextId::S168));}
  if(downloadRequested&&!formOpen&&updates.phase()==UpdatePhase::Ready&&!updates.stage().empty()){downloadRequested=false;installUpdate(updates.stage());}
  else if(downloadRequested&&(updates.phase()==UpdatePhase::Failed||updates.phase()==UpdatePhase::Cancelled)){downloadRequested=false;updateWindow.state(updates.progress(),wide(status),false);}
 }
 void offerUpdate(const UpdateInfo&info){updateWindow.show(window,instance,info);updateWindow.state(updates.progress(),wide(updates.message()),updates.busy()||handoff.pending());}
 void installUpdate(const fs::path&stage){
  if(handoff.pending())return;
  try{handoffError.clear();saveConfig(config,c,configRecovery);auto temp=appData()/L"updaters"/wide(randomId());fs::create_directories(temp);auto bytes=readBytes(executable().parent_path()/L"Updater.exe");atomicWrite(temp/L"Updater.exe",bytes.data(),bytes.size());observedStage=stage;std::error_code staleError;fs::remove(stage/L"handoff-error.txt",staleError);handoff.start(temp/L"Updater.exe",stage,executable().parent_path());updateWindow.state(100,tw(TextId::S169),true);}catch(const std::exception&e){handoff.reset();handoffError=tw(TextId::S170)+wide(e.what());updateWindow.state(0,handoffError,false);}
 }
 void mouseDown(){if(menuOpen)return;if(hasReminder()){acknowledge();return;}pressed=true;dragging=false;GetCursorPos(&down);lastMouse=down;startPos={c.x,c.y};lastMove=GetTickCount64();SetCapture(window);}
 void mouseMove(LPARAM lp){if(menuOpen)return;auto t=GetTickCount64();POINT p;GetCursorPos(&p);if(pressed){if(!dragging&&(std::abs(p.x-down.x)>MulDiv(4,dpi,96)||std::abs(p.y-down.y)>MulDiv(4,dpi,96))){dragging=true;action=Action::Drag;speakDialogue("drag");ShowWindow(bubble,SW_HIDE);}if(dragging){double dt=std::max<ULONGLONG>(1,t-lastMove);spring.drag((p.x-lastMouse.x)*16/dt,(p.y-lastMouse.y)*16/dt);c.x=startPos.x+p.x-down.x;c.y=startPos.y+p.y-down.y;clampPosition();render();scheduleFrame();lastMouse=p;lastMove=t;}lastInput=t;return;}
  if(!tracking){TRACKMOUSEEVENT e{sizeof(e),TME_LEAVE,window,0};TrackMouseEvent(&e);tracking=true;}int x=GET_X_LPARAM(lp);int direction=x>lastMouse.x?1:x<lastMouse.x?-1:0;if(t-lastStroke>1800)strokes=0;if(direction&&direction!=lastDirection&&t-lastStroke>100){lastDirection=direction;lastStroke=t;if(++strokes>=5){strokes=0;interact(Action::Pet,"pet");}}lastMouse.x=x;
 }
 void mouseUp(){if(menuOpen||!pressed)return;pressed=false;ReleaseCapture();lastInput=GetTickCount64();if(dragging){dragging=false;spring.vx=-2;spring.vy=3;action=Action::Happy;actionEnd=lastInput+1500;persist();if(hasReminder())reminder();else{speech=say("happy");speechIsDialogue=true;bubbleEnd=lastInput+4000;placeBubble();}scheduleFrame();return;}
  if(lastInput-lastClick<650)++clicks;else clicks=1;lastClick=lastInput;if(clicks>=4){interact(Action::Angry,"angry");clicks=0;return;}if(action==Action::Sleep){interact(Action::Wake,"wake");return;}static Action a[]={Action::Happy,Action::Shy,Action::Surprise,Action::Yawn,Action::Stretch,Action::Belly};auto k=rng()%6;interact(a[k],k==0?"happy":"general");
 }
 static LRESULT CALLBACK bubbleProc(HWND w,UINT m,WPARAM wp,LPARAM lp){auto self=reinterpret_cast<App*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(w,m,wp,lp);if(m==WM_MOUSEACTIVATE)return MA_NOACTIVATE;if(m==WM_LBUTTONUP){if(!self->menuOpen&&self->hasReminder())self->acknowledge();return 0;}if(m==WM_RBUTTONUP){POINT p;GetCursorPos(&p);self->menu(p);return 0;}if(m==WM_PAINT){PAINTSTRUCT ps;HDC dc=BeginPaint(w,&ps);RECT r;GetClientRect(w,&r);auto brush=CreateSolidBrush(RGB(255,248,235));FillRect(dc,&r,brush);DeleteObject(brush);int d=int(GetDpiForWindow(w));auto f=CreateFontW(-MulDiv(14,d,96),0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,0,0,uiFontName());auto old=SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(77,65,76));InflateRect(&r,-MulDiv(12,d,96),-MulDiv(12,d,96));DrawTextW(dc,self->speech.c_str(),-1,&r,DT_WORDBREAK|DT_NOPREFIX);SelectObject(dc,old);DeleteObject(f);EndPaint(w,&ps);return 0;}return DefWindowProcW(w,m,wp,lp);}
 static LRESULT CALLBACK proc(HWND w,UINT m,WPARAM wp,LPARAM lp){auto self=reinterpret_cast<App*>(GetWindowLongPtrW(w,GWLP_USERDATA));if(m==WM_NCCREATE){self=static_cast<App*>(reinterpret_cast<CREATESTRUCTW*>(lp)->lpCreateParams);self->window=w;SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}if(!self)return DefWindowProcW(w,m,wp,lp);
  try{switch(m){case WM_ENTERMENULOOP:self->menuLayer.enter(w);return 0;case WM_EXITMENULOOP:self->menuLayer.exit(self->c.top);return 0;case WM_APP+30:self->maintainZOrder(true);self->visibility();return 0;case LanguageChanged:self->refreshLanguage();return 0;case WM_MOUSEACTIVATE:return MA_NOACTIVATE;case WM_TIMER:if(wp==1)self->tick();else if(wp==2)self->animate();return 0;case WM_LBUTTONDOWN:self->mouseDown();return 0;case WM_MOUSEMOVE:self->mouseMove(lp);return 0;case WM_LBUTTONUP:self->mouseUp();return 0;case WM_CAPTURECHANGED:if(self->pressed){self->pressed=false;self->dragging=false;self->spring.vy=2;self->persist();if(self->hasReminder())self->reminder();}return 0;case WM_MOUSELEAVE:self->tracking=false;return 0;case WM_RBUTTONUP:{POINT p;GetCursorPos(&p);self->menu(p);return 0;}case WM_DPICHANGED:{auto r=reinterpret_cast<RECT*>(lp);self->c.x=r->left;self->c.y=r->top;self->sizeChanged();return 0;}case WM_DISPLAYCHANGE:self->sizeChanged();self->persist();return 0;case WM_THEMECHANGED:self->nameplate.invalidate();self->updateNameplate();return 0;case WM_SETTINGCHANGE:if(wp==SPI_SETWORKAREA){self->clampPosition();self->placeBubble();}self->nameplate.invalidate();self->updateNameplate();return 0;case WM_POWERBROADCAST:if(wp==PBT_APMRESUMEAUTOMATIC||wp==PBT_APMRESUMESUSPEND)self->tick();return TRUE;case TRAY:if(LOWORD(lp)==WM_CONTEXTMENU||LOWORD(lp)==WM_RBUTTONUP||LOWORD(lp)==NIN_SELECT||LOWORD(lp)==NIN_KEYSELECT){POINT p;GetCursorPos(&p);self->menu(p);}return 0;case WM_CLOSE:self->manualHidden=true;self->visibility();return 0;case WM_QUERYENDSESSION:return TRUE;case WM_ENDSESSION:if(wp){self->c.timers.stopAll();self->persist();}return 0;case WM_DESTROY:{KillTimer(w,1);KillTimer(w,2);if(!self->updating)self->c.timers.stopAll();self->persist();NOTIFYICONDATAW n{};n.cbSize=sizeof(n);n.hWnd=w;n.uID=1;Shell_NotifyIconW(NIM_DELETE,&n);if(self->bubble)DestroyWindow(self->bubble);PostQuitMessage(0);return 0;}}
   if(m==self->taskbar){self->tray(true);return 0;}
  }catch(const std::exception&e){self->error(tw(TextId::S171)+wide(e.what()));}return DefWindowProcW(w,m,wp,lp);
 }
 bool init(bool restore){singleton=CreateMutexW(nullptr,FALSE,L"Local\\DesktopPet.SingleInstance");if(!singleton||GetLastError()==ERROR_ALREADY_EXISTS)return false;auto locations=configPaths();config=locations.write;configRecovery=locations.recovery;if(locations.read.empty()){c.language=windowsDisplayLanguage();c.timer.title=tr(TextId::S000,c.language);c.timer.text=tr(TextId::S001,c.language);c.syncName();}readConfig(locations,c,restore);c.enableTimerModes();c.syncName();setUILanguage(c.language);
  std::string assetFailure;if(c.pet==2&&!custom.load(c.profiles[size_t(c.activeCustom)].files,&assetFailure))c.selectBuiltin(0);
  try{dialogue=Json::parse(readText(executable().parent_path()/L"data"/L"dialogue.json"));}catch(...){dialogue=Json{};}
  WNDCLASSW wc{};wc.hInstance=instance;wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);wc.lpfnWndProc=proc;wc.lpszClassName=L"DesktopPetWindow";wc.hIcon=brandIcon(GetSystemMetricsForDpi(SM_CXICON,GetDpiForSystem()));RegisterClassW(&wc);wc.lpfnWndProc=bubbleProc;wc.lpszClassName=L"DesktopPetBubble";RegisterClassW(&wc);
  window=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"DesktopPetWindow",L"DesktopPet",WS_POPUP,c.x,c.y,144,144,nullptr,nullptr,instance,this);if(!window)throw std::runtime_error(tr(TextId::S172));
  bubble=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,L"DesktopPetBubble",L"",WS_POPUP,0,0,10,10,window,nullptr,instance,this);if(!bubble)throw std::runtime_error(tr(TextId::S173));
  brandWindow(window);nameplate.create(window,instance);foregroundHook=SetWinEventHook(EVENT_SYSTEM_FOREGROUND,EVENT_SYSTEM_FOREGROUND,nullptr,foregroundChanged,0,0,WINEVENT_OUTOFCONTEXT|WINEVENT_SKIPOWNPROCESS);
  if(!c.positioned){MONITORINFO mi{sizeof(mi)};POINT zero{0,0};GetMonitorInfoW(MonitorFromPoint(zero,MONITOR_DEFAULTTOPRIMARY),&mi);c.x=mi.rcWork.right-170;c.y=mi.rcWork.bottom-170;}sizeChanged();applyPass();taskbar=RegisterWindowMessageW(L"TaskbarCreated");bindUpdateWindow();aboutWindow.bind([this]{command(CheckUpdate);});tray(true);lastInput=GetTickCount64();nextIdle=lastInput+20000;frameTime=lastInput;loading=false;SetTimer(window,1,1000,nullptr);visibility();if(!assetFailure.empty())speak(tw(TextId::S174));if(hasReminder())reminder();if(c.autoUpdate&&updates.enabled()){c.lastCheck=now();updates.check();}persist();return true;
 }
};
}
int WINAPI wWinMain(HINSTANCE instance,HINSTANCE,PWSTR,int){
 if(!secureDllLoading())return 1;
 SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);INITCOMMONCONTROLSEX controls{sizeof(controls),ICC_STANDARD_CLASSES|ICC_PROGRESS_CLASS|ICC_UPDOWN_CLASS|ICC_TAB_CLASSES|ICC_BAR_CLASSES};InitCommonControlsEx(&controls);CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
 int argc=0;auto argv=CommandLineToArgvW(GetCommandLineW(),&argc);if(argc==5&&wcscmp(argv[1],L"--verify-update")==0){int result=0;try{auto info=verifyManifest(readBytes(argv[2],65536),readBytes(argv[3],64));auto bytes=readBytes(argv[4],15*1024*1024);auto metadata=Json::parse(std::string(info.manifest.begin(),info.manifest.end()));if(bytes.size()!=info.size||sha256(bytes)!=metadata["sha256"].str())throw std::runtime_error(tr(TextId::S175));inspectZip(bytes);}catch(...){result=1;}LocalFree(argv);CoUninitialize();return result;}if(argc==4&&wcscmp(argv[1],L"--adopt-update-trust")==0){int result=0;try{commitInstalledTrust(readBytes(argv[2],65536),readBytes(argv[3],64),PET_VERSION);}catch(...){result=1;}LocalFree(argv);CoUninitialize();return result;}bool restore=argc>1&&wcscmp(argv[1],L"--updated")==0;std::wstring eventName=restore&&argc>2?argv[2]:L"";LocalFree(argv);
 try{
  auto dir=executable().parent_path();if(!restore&&fs::exists(dir/L".desktoppet-transaction.json")){loadTransaction(dir);auto temp=appData()/L"updaters"/wide(randomId());fs::create_directories(temp);auto b=readUpdateFile(dir/L"Updater.exe");atomicWrite(temp/L"Updater.exe",b.data(),b.size());auto pi=launch(temp/L"Updater.exe",{L"--recover",dir.wstring()});CloseHandle(pi.hProcess);CloseHandle(pi.hThread);return 0;}
  if(!fs::exists(dir/transactionJournal))cleanupUpdateWork();
  App app(instance);if(!app.init(restore))return 0;if(!eventName.empty()){HANDLE event=OpenEventW(EVENT_MODIFY_STATE,FALSE,eventName.c_str());if(event){SetEvent(event);CloseHandle(event);}}
  MSG msg{};while(GetMessageW(&msg,nullptr,0,0)>0){if(!app.updateWindow.dialog(msg)&&!app.aboutWindow.dialog(msg)){TranslateMessage(&msg);DispatchMessageW(&msg);}}CoUninitialize();return 0;
 }catch(const std::exception&e){auto message=std::string(e.what());localizedMessage(nullptr,instance,TextId::S176,[message]{return wide(currentSystemText(message));},false);CoUninitialize();return 1;}
}
