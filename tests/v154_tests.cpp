#include "nameplate_layout.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
using namespace pet;
#ifndef PET_TEST_ROOT
#define PET_TEST_ROOT "."
#endif
static int checks=0;
static void check(bool ok,const char*message){++checks;if(!ok)throw std::runtime_error(message);}
static std::string read(const char*name){std::ifstream file(std::filesystem::path(PET_TEST_ROOT)/name);if(!file)throw std::runtime_error("missing source");std::ostringstream out;out<<file.rdbuf();return out.str();}
int main(){try{
 check(nameplateMetrics(100,96).font==14,"baseline changes from 12 to 14 physical pixels");check(nameplateMetrics(50,96).font==10,"small pets retain ten logical pixels for readability");
 for(int dpi:{96,120,144,168,192,240,288,384}){int previous=0;for(int scale=50;scale<=300;++scale){auto m=nameplateMetrics(scale,dpi);int expected=int(std::lround(std::max(10.,14.*scale/100)*dpi/96));check(m.font==expected&&m.font>=previous,"font uses one final rounding and grows monotonically with pet scale");previous=m.font;check(m.padX>=uiPixels(6,dpi)&&m.padY>=uiPixels(3,dpi)&&m.diameter>=uiPixels(8,dpi),"padding and rounded background retain readable minimums");
   for(double width:{1.,14.2,84.5,168.25})for(double height:{10.,17.1,24.75}){auto size=nameplateSize(width,height,m);check(size.w>=std::ceil(width)+2*m.padX&&size.h>=std::ceil(height)+2*m.padY,"complete measured text and padding fit without truncation");check((size.w-width)/2>=m.padX&&(size.h-height)/2>=m.padY,"horizontal and vertical centering retain padding");}
   auto reduced=nameplateMetrics(scale,dpi,std::max(1,m.font/2));check(reduced.padX<=m.padX&&reduced.padY<=m.padY,"workspace fitting also adjusts padding instead of wasting text width");check(nameplateReservedHeight(scale,dpi)>=nameplateSize(m.font,m.font,m).h,"bottom-screen reserve covers the nominal label height");
  }}
 for(auto name:{"中文日文混合","あいうえおか","WWWWWWWWWWWW","Blue Dolphin","名🐾名🐾名🐾"}){check(petNameMetrics(name).valid,"all longest multilingual names retain the existing twelve-unit rule");}
 check(!petNameMetrics("中文日文超過六字").valid,"nameplate optimization never weakens name limits");
 UiRect work{-1920,-1080,1920,1040},body{-1300,-700,160,96};auto m=nameplateMetrics(100,144);auto size=nameplateSize(150.5,24.2,m);auto plate=accessoryPlacement(body,work,size.w,size.h,uiPixels(5,144));check(plate&&plate->y==body.y+body.h+uiPixels(5,144)&&!overlaps(body,*plate),"larger label stays centered below stable visible bounds, including negative monitors");
 auto source=read("src/nameplate.h"),theme=read("src/ui_theme.h"),cmake=read("CMakeLists.txt");check(source.find("theme.sync(window,false)")!=std::string::npos&&theme.find("if(nativeFrame){")!=std::string::npos,"layered label no longer opts into generic DWM window decoration");for(auto token:{"DWMNCRP_DISABLED","DWMWA_NCRENDERING_POLICY","DWMWA_TRANSITIONS_FORCEDISABLED","CS_DROPSHADOW","WM_DWMCOMPOSITIONCHANGED"})check(source.find(token)!=std::string::npos,"shadow policy is scoped to the nameplate and reapplied on DWM/theme changes");check(source.find("wc.style=0")!=std::string::npos&&source.find("StringTrimmingNone")!=std::string::npos,"class shadow is disabled and text never uses ellipsis");check(source.find("return;theme.sync(window,false)")!=std::string::npos,"unchanged text/DPI/scale/theme/workspace returns before font creation and measurement");check(source.find("SetTimer")==std::string::npos&&source.find("SystemParametersInfoW")==std::string::npos,"no polling, forced z-order loop or global visual-setting mutation added");
 const char*prior[]={"v152_brand_sources","core","v150_dialogue_timer","v151_settings_layout","v152_modes_editors","v153_menu_state","v153_name_persistence","v150_animations","renewed_art","security_recovery","v150_native_controls","v151_native_ui","v152_native_ui","v153_native_menu","dll_search_policy","windows_native","ocean_embedded","renewed_embedded"};for(auto name:prior)check(cmake.find(std::string("add_test(NAME ")+name+" ")!=std::string::npos,"all eighteen previous CI tests remain registered");
 std::cout<<"PASS: "<<checks<<" v1.5.4 checks; font/DPI/padding/measurement, stable placement, scoped DWM policy and cache/CI preservation\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<" after "<<checks<<" checks\n";return 1;}}
