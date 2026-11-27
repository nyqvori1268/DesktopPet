#include "sprite.h"
#include "core.h"
#include <filesystem>
#include <fstream>
#include <vector>
#include <iostream>
namespace fs=std::filesystem;
using Bytes=std::vector<unsigned char>;
static void le(Bytes&b,unsigned v,int n){for(int i=0;i<n;++i)b.push_back(static_cast<unsigned char>(v>>(8*i)));}
static void be(Bytes&b,unsigned v){for(int i=3;i>=0;--i)b.push_back(static_cast<unsigned char>(v>>(8*i)));}
static void save(fs::path p,const Bytes&b){fs::create_directories(p.parent_path());std::ofstream f(p,std::ios::binary);f.write(reinterpret_cast<const char*>(b.data()),std::streamsize(b.size()));if(!f)throw std::runtime_error("asset write failed");}
static void chunk(Bytes&b,const char*tag,const Bytes&data){be(b,unsigned(data.size()));auto start=b.size();b.insert(b.end(),tag,tag+4);b.insert(b.end(),data.begin(),data.end());be(b,pet::crc32(b.data()+start,b.size()-start));}
static void png(fs::path p,const std::vector<uint32_t>&pixels,int w,int h){Bytes b={137,80,78,71,13,10,26,10},ihdr;be(ihdr,w);be(ihdr,h);ihdr.insert(ihdr.end(),{8,6,0,0,0});chunk(b,"IHDR",ihdr);Bytes raw;for(int y=0;y<h;++y){raw.push_back(0);for(int x=0;x<w;++x){auto c=pixels[y*w+x];raw.push_back((c>>16)&255);raw.push_back((c>>8)&255);raw.push_back(c&255);raw.push_back((c>>24)&255);}}Bytes z={0x78,0x01};for(size_t p0=0;p0<raw.size();){unsigned n=unsigned(std::min<size_t>(65535,raw.size()-p0));z.push_back(p0+n==raw.size()?1:0);le(z,n,2);le(z,(~n)&65535,2);z.insert(z.end(),raw.begin()+p0,raw.begin()+p0+n);p0+=n;}unsigned a=1,c=0;for(auto v:raw){a=(a+v)%65521;c=(c+a)%65521;}be(z,(c<<16)|a);chunk(b,"IDAT",z);chunk(b,"IEND",{});save(p,b);}
int main(int argc,char**argv){try{fs::path root=argc>1?argv[1]:".";pet::Sprite sprite;
 for(int species=0;species<2;++species){int w=240,h=14*48;std::vector<uint32_t> atlas(w*h);for(int coat=0;coat<5;++coat)for(int a=0;a<14;++a){sprite.make(species,coat,pet::Action(a),1.3);for(int y=0;y<48;++y)for(int x=0;x<48;++x)atlas[(a*48+y)*w+coat*48+x]=sprite.pixels[y*48+x];}png(root/"assets"/(species?"dogs/dogs.png":"cats/cats.png"),atlas,w,h);}
 // Brand ICO is supplied artwork: do not overwrite it when exporting pet sprites.
 std::cout<<"Original assets exported\n";
 }catch(const std::exception&e){std::cerr<<e.what();return 1;}}
