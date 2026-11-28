#pragma once
namespace pet {
enum class SettingsFile{None,Portable,Local};
struct SettingsChoice{SettingsFile read,alternate,write;};
// Read existing data before choosing where to write. A permission or version-folder
// change must not turn a valid user profile into a first-run default profile.
inline SettingsChoice chooseSettings(bool portableExists,bool localExists,bool portableWritable,bool localNewer=false){
 auto first=portableExists?SettingsFile::Portable:localExists?SettingsFile::Local:SettingsFile::None;
 if(localExists&&(!portableExists||localNewer))first=SettingsFile::Local;
 auto other=first==SettingsFile::Portable&&localExists?SettingsFile::Local:first==SettingsFile::Local&&portableExists?SettingsFile::Portable:SettingsFile::None;
 return {first,other,portableWritable?SettingsFile::Portable:SettingsFile::Local};
}
}
