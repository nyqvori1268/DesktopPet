#include "platform.h"
#include "update.h"
#include "zip.h"
#include <bcrypt.h>
#include <iostream>
using namespace pet;
static void require(bool value,const char*text){if(!value)throw std::runtime_error(text);}
static std::string hex(const std::vector<unsigned char>&b){std::string s;const char*h="0123456789abcdef";for(auto c:b){s+=h[c>>4];s+=h[c&15];}return s;}
static std::vector<unsigned char> unhex(const std::string&s){std::vector<unsigned char>b;for(size_t i=0;i<s.size();i+=2)b.push_back(static_cast<unsigned char>(std::stoul(s.substr(i,2),nullptr,16)));return b;}
int wmain(int argc,wchar_t**argv){try{
 if(argc==3&&std::wstring(argv[1])==L"--package"){auto files=inspectZip(readBytes(argv[2]));require(files.size()==5,"Release package format");std::cout<<"PASS package structure, CRC, allowlist\n";return 0;}
 if(argc==5&&std::wstring(argv[1])==L"--signature"){require(verifySignature(readBytes(argv[3]),readBytes(argv[4]),utf8(argv[2])),"PowerShell/native ECDSA interoperability");std::cout<<"PASS PowerShell signature\n";return 0;}
 std::vector<unsigned char> abc={'a','b','c'};require(sha256(abc)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA256 reference");
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_KEY_HANDLE key=nullptr;require(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0)>=0,"ECDSA provider");require(BCryptGenerateKeyPair(algorithm,&key,256,0)>=0&&BCryptFinalizeKeyPair(key,0)>=0,"Generate ephemeral test key");ULONG n=0;require(BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,nullptr,0,&n,0)>=0,"Export size");std::vector<unsigned char>blob(n);require(BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,blob.data(),n,&n,0)>=0,"Export key");require(blob.size()==72,"P256 blob");std::vector<unsigned char>point(blob.begin()+8,blob.end());auto digest=unhex(sha256(abc));std::vector<unsigned char>sig(64);require(BCryptSignHash(key,nullptr,digest.data(),ULONG(digest.size()),sig.data(),64,&n,0)>=0&&n==64,"ECDSA sign");require(verifySignature(abc,sig,hex(point)),"valid signature");abc[0]='x';require(!verifySignature(abc,sig,hex(point)),"tampered message rejected");abc[0]='a';sig[0]^=1;require(!verifySignature(abc,sig,hex(point)),"tampered signature rejected");sig.resize(63);require(!verifySignature(abc,sig,hex(point)),"short signature rejected");require(!verifySignature(abc,sig,""),"missing key rejected");BCryptDestroyKey(key);BCryptCloseAlgorithmProvider(algorithm,0);
 auto dir=appData()/L"test"/wide(randomId());try{fs::create_directories(dir);Config c;writeText(dir/L"config.json",c.json().dump());c.name="原子寫入測試";saveConfig(dir/L"config.json",c);Config loaded;loaded.load(Json::parse(readText(dir/L"config.json")),false);require(loaded.name==c.name,"UTF8 atomic configuration");require(quote(L"a b\\")==L"\"a b\\\\\"","Windows command quote");require(wide(utf8(L"小橘🐱"))==L"小橘🐱","UTF8 conversion");fs::remove_all(dir);}catch(...){std::error_code ec;fs::remove_all(dir,ec);throw;}
 bool rejected=false;try{verifyManifest({'{','}'},std::vector<unsigned char>(64));}catch(...){rejected=true;}require(rejected,"unsigned update rejected");std::cout<<"PASS Windows SHA256 / ECDSA tampering / atomic config / UTF8 / process quoting\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
