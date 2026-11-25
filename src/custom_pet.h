#pragma once
#include "platform.h"
#include "image_core.h"
namespace pet {
// Local, bounded native decoding. No downloader, scripts or remote references.
ImageClip decodeCustom(const fs::path&file);
fs::path customDirectory();
class CustomPet {
 std::array<ImageClip,7> clips;
 int active=-1;uint64_t started=0;
public:
 bool load(const Config&config,std::string*failure=nullptr);
 void set(int state,ImageClip clip){clips.at(size_t(state))=std::move(clip);active=-1;}
 bool ready()const{return !clips[0].frames.empty();}
 unsigned duration(Action action,unsigned fallback)const{const auto&c=clips[size_t(customState(action))];if(c.frames.size()<2)return fallback;uint64_t total=0;for(auto&f:c.frames)total+=f.delay;return unsigned(std::min<uint64_t>(10000,std::max<uint64_t>(fallback,total)));}
 const ImageClip& clip(Action action,bool&fallback){int index=customState(action);fallback=clips[size_t(index)].frames.empty();return fallback?clips[0]:clips[size_t(index)];}
 size_t frame(Action action,uint64_t time){bool fallback;clip(action,fallback);int index=fallback?0:customState(action);if(index!=active){active=index;started=time;}return clips[size_t(index)].frameAt(time-started);}
};
}
