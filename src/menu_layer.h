#pragma once
namespace pet {
// Tracking spans all popup/submenu messages, including the final WM_EXITMENULOOP.
struct MenuLayerState{
 bool tracking=false;unsigned loops=0;
 bool active()const{return tracking||loops!=0;}
 bool begin(){if(tracking)return false;tracking=true;return true;}
 void enter(){++loops;}
 void exit(){if(loops)--loops;}
 void finish(){tracking=false;loops=0;}
};
}
#ifdef _WIN32
#include "platform.h"
namespace pet {
class MenuLayer{
 MenuLayerState state;HWND owner=nullptr;bool suspended=false;
 void suspend(HWND window){if(suspended)return;owner=window;suspended=true;if(IsWindow(owner)&&(GetWindowLongPtrW(owner,GWL_EXSTYLE)&WS_EX_TOPMOST))SetWindowPos(owner,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);}
 void restore(bool top){if(!suspended||active())return;suspended=false;if(IsWindow(owner)&&(top||(GetWindowLongPtrW(owner,GWL_EXSTYLE)&WS_EX_TOPMOST)))SetWindowPos(owner,top?HWND_TOPMOST:HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE);owner=nullptr;}
public:
 bool active()const{return state.active();}
 bool begin(HWND window){bool owns=state.begin();if(owns)suspend(window);return owns;}
 void enter(HWND window){state.enter();suspend(window);}
 void exit(bool top){state.exit();restore(top);}
 void finish(bool top){state.finish();restore(top);}
};
class PopupLayerScope{
 MenuLayer&layer;const bool&top;bool owns;
public:
 PopupLayerScope(MenuLayer&value,HWND owner,const bool&requested):layer(value),top(requested),owns(layer.begin(owner)){}
 ~PopupLayerScope(){if(owns)layer.finish(top);}
 PopupLayerScope(const PopupLayerScope&)=delete;PopupLayerScope&operator=(const PopupLayerScope&)=delete;
};
inline UINT trackPetPopup(MenuLayer&layer,HWND owner,const bool&top,HMENU menu,POINT point){
 HWND previous=GetForegroundWindow();UINT selected=0;
 {
  PopupLayerScope scope(layer,owner,top);
  // Required by Win32 for notification-area menus to dismiss on outside clicks.
  SetForegroundWindow(owner);
  selected=UINT(TrackPopupMenuEx(menu,TPM_RETURNCMD|TPM_RIGHTBUTTON,point.x,point.y,owner,nullptr));
 }
 PostMessageW(owner,WM_NULL,0,0);
 // Return focus only if the menu still owns it. Never override an outside click.
 if(previous&&previous!=owner&&IsWindow(previous)&&GetForegroundWindow()==owner)SetForegroundWindow(previous);
 return selected;
}
}
#endif
