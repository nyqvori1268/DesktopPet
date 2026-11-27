#pragma once
#include "core.h"
namespace pet {
// Operate on a caller-owned draft; the stored profile changes only after OK.
inline void setCustomAction(CustomProfile&draft,int state,const std::string&file,const std::string&label={}){
 if(!draft.ready())throw std::runtime_error(tr(TextId::IdleRequired));
 if(state<1||state>6||(!file.empty()&&!customFileName(file)))throw std::runtime_error(tr(TextId::S030));
 draft.files[customStates[state]]=file;draft.fileLabels[customStates[state]]=file.empty()?std::string{}:clipUtf8(label,240);
}
using DialogueBuffers=std::array<std::array<std::string,8>,3>;
inline CustomProfile editedDialogues(const CustomProfile&original,const DialogueBuffers&buffers,bool enabled){auto draft=original;draft.dialogueEnabled=enabled;for(int language=0;language<3;++language)for(size_t state=0;state<8;++state){Json::Array lines;for(auto&line:dialogueLines(buffers[size_t(language)][state],true))lines.emplace_back(line);draft.setDialogue(Language(language),dialogueStates[state],lines);}return draft;}
}
