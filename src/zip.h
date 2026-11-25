#pragma once
#include "core.h"
#include <set>
namespace pet {
struct ZipFile{std::string name;size_t offset=0,size=0;};
// Intentionally accepts only our deterministic STORE ZIP, never invokes a shell/extractor.
inline std::vector<ZipFile> inspectZip(const std::vector<unsigned char>&b){
 auto fail=[](){throw std::runtime_error("Unsafe or unsupported ZIP");};
 auto u16=[&](size_t p)->uint16_t{if(p>b.size()||b.size()-p<2)fail();return uint16_t(b[p]|(b[p+1]<<8));};
 auto u32=[&](size_t p)->uint32_t{if(p>b.size()||b.size()-p<4)fail();return uint32_t(b[p])|(uint32_t(b[p+1])<<8)|(uint32_t(b[p+2])<<16)|(uint32_t(b[p+3])<<24);};
 if(b.size()<22||b.size()>15*1024*1024)fail();
 size_t e=b.size()-22;
 if(u32(e)!=0x06054b50||u16(e+4)||u16(e+6)||u16(e+20)||u16(e+8)!=5||u16(e+10)!=5)fail();
 size_t cd=u32(e+16),cdSize=u32(e+12);if(cd>e||cdSize!=e-cd)fail();
 std::vector<ZipFile> files;std::set<std::string> seen;size_t p=cd,localEnd=0;size_t total=0;
 for(int i=0;i<5;++i){if(u32(p)!=0x02014b50)fail();auto flags=u16(p+8);if(flags!=0&&flags!=0x800)fail();if(u16(p+10)!=0||u16(p+34)!=0)fail();size_t n=u16(p+28),extra=u16(p+30),comment=u16(p+32),sz=u32(p+24),off=u32(p+42);uint32_t crc=u32(p+16);
  if(sz!=u32(p+20)||p+46+n+extra+comment>e)fail();
  std::string name(reinterpret_cast<const char*>(b.data()+p+46),n);
  if(!packagePath(name)||!seen.insert(name).second||off!=localEnd)fail();
  // Reject unix symlinks and Windows reparse attributes.
  uint32_t attr=u32(p+38);if(((attr>>16)&0170000)==0120000||(attr&0x400))fail();
  if(u32(off)!=0x04034b50||u16(off+6)!=flags||u16(off+8)||u32(off+14)!=crc||u32(off+18)!=sz||u32(off+22)!=sz||u16(off+26)!=n)fail();
  size_t data=off+30+n+u16(off+28);if(data>cd||sz>cd-data||off+30+n>b.size())fail();
  if(std::string(reinterpret_cast<const char*>(b.data()+off+30),n)!=name)fail();
  if(crc32(b.data()+data,sz)!=crc)fail();
  total+=sz;if(total>15*1024*1024)fail();files.push_back({name,data,sz});localEnd=data+sz;p+=46+n+extra+comment;
 }
 if(p!=e||localEnd!=cd)fail();
 return files;
}
}
