#include "platform.h"
#include "update.h"
#include "zip.h"
#include "custom_pet.h"
#include "ocean.h"
#include "renewed_pet.h"
#include "handoff.h"
#include "update_logic.h"
#include "trust_store.h"
#include "transaction.h"
#include <bcrypt.h>
#include <iostream>
using namespace pet;
static void require(bool value,const char*text){if(!value)throw std::runtime_error(text);}
static std::string hex(const std::vector<unsigned char>&b){std::string s;const char*h="0123456789abcdef";for(auto c:b){s+=h[c>>4];s+=h[c&15];}return s;}
static std::vector<unsigned char> unhex(const std::string&s){std::vector<unsigned char>b;for(size_t i=0;i<s.size();i+=2)b.push_back(static_cast<unsigned char>(std::stoul(s.substr(i,2),nullptr,16)));return b;}
static void trustCryptoTests(){
 struct Keys{BCRYPT_ALG_HANDLE alg=nullptr;std::array<BCRYPT_KEY_HANDLE,3> handles{};~Keys(){for(auto key:handles)if(key)BCryptDestroyKey(key);if(alg)BCryptCloseAlgorithmProvider(alg,0);}}keys;
 require(BCryptOpenAlgorithmProvider(&keys.alg,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0)>=0,"test policy ECDSA provider");std::array<std::string,3> points;
 for(size_t i=0;i<3;++i){require(BCryptGenerateKeyPair(keys.alg,&keys.handles[i],256,0)>=0&&BCryptFinalizeKeyPair(keys.handles[i],0)>=0,"ephemeral test policy key");ULONG length=0;BCryptExportKey(keys.handles[i],nullptr,BCRYPT_ECCPUBLIC_BLOB,nullptr,0,&length,0);std::vector<unsigned char>blob(length);require(BCryptExportKey(keys.handles[i],nullptr,BCRYPT_ECCPUBLIC_BLOB,blob.data(),length,&length,0)>=0,"export ephemeral public key");points[i]=hex(std::vector<unsigned char>(blob.begin()+8,blob.end()));require(publicKeyValid(points[i]),"native P256 point validation");}
 require(!publicKeyValid(std::string(128,'0')),"invalid curve point rejected");
 Json policy;policy["schema"]=1;policy["epoch"]=1;policy["primary"]=points[2];policy["backup"]=points[1];policy["minimumVersion"]="1.1.1";std::string payload=policy.dump();
 auto sign=[&](int index){auto digest=unhex(sha256(std::vector<unsigned char>(payload.begin(),payload.end())));std::vector<unsigned char>sig(64);ULONG n=0;require(BCryptSignHash(keys.handles[size_t(index)],nullptr,digest.data(),ULONG(digest.size()),sig.data(),64,&n,0)>=0&&n==64,"sign test trust policy");return hex(sig);};
 auto verify=[](const std::string&text,const std::vector<unsigned char>&sig,const std::string&point){return verifySignature(std::vector<unsigned char>(text.begin(),text.end()),sig,point);};
 TrustState roots;roots.primary=points[0];roots.backup=points[1];Json envelope;envelope["payload"]=payload;envelope["signature"]=sign(0);bool rejected=false;try{applyPolicy(roots,envelope,verify);}catch(...){rejected=true;}require(rejected,"daily primary cannot replace backup-protected trust");
 envelope["signature"]=sign(1);auto rotated=applyPolicy(roots,envelope,verify);require(rotated.accepts(points[2])&&!rotated.accepts(points[0]),"offline backup authorized rotation and revoked primary");envelope["signature"]=sign(2);rejected=false;try{applyPolicy(roots,envelope,verify);}catch(...){rejected=true;}require(rejected,"new untrusted key cannot authorize itself");
 std::cout<<"PASS native ECDSA trust-policy authorization, backup rotation and revocation\n";
}
static Json testTransaction(const fs::path&dir){Json j;j["schema"]=2;j["phase"]="backing";j["target"]=updateTarget(dir);j["oldVersion"]="1.3.0";j["version"]="1.4.0";j["manifest"]="test-only manifest";j["receipt"]=sha256(std::vector<unsigned char>{'t','e','s','t','-','o','n','l','y',' ','m','a','n','i','f','e','s','t'});j["signature"]=std::string(128,'0');for(auto name:transactionFiles){auto path=dir/wide(name);std::vector<unsigned char>data{'o','l','d'};writeUpdateFile(path,data);j["files"][name]["existed"]=true;j["files"][name]["before"]=sha256(data);j["files"][name]["after"]=std::string(64,'b');}return j;}
static void transactionFilesystemTests(){
 auto root=appData()/L"test"/wide(randomId());fs::create_directories(root/L"data");
 try{
  auto pi=launch(executable(),{L"--journal-abort",root.wstring()});require(WaitForSingleObject(pi.hProcess,10000)==WAIT_OBJECT_0,"interrupted journal helper exits");DWORD code=0;GetExitCodeProcess(pi.hProcess,&code);CloseHandle(pi.hProcess);CloseHandle(pi.hThread);require(code==23,"real ExitProcess interrupt bypasses destructors");auto j=loadTransaction(root);require(j["phase"].str()=="prepared","authenticated journal survives abrupt process termination");
  for(auto name:transactionFiles)require(sha256(readUpdateFile(root/transactionBackup/wide(name)))==j["files"][name]["before"].str(),"all durable backup hashes match after process loss");
  bool rejected=false;auto copied=root/L"other";fs::create_directory(copied);auto cipher=readUpdateFile(root/transactionJournal);atomicWrite(copied/transactionJournal,cipher.data(),cipher.size());try{loadTransaction(copied);}catch(...){rejected=true;}require(rejected,"journal bound to original target directory");
  {UpdatePathLock guard(root/L"data");require(!MoveFileW((root/L"data").c_str(),(root/L"moved").c_str()),"locked parent cannot be swapped or renamed");}
  require(CreateHardLinkW((root/L"link").c_str(),(root/L"README.md").c_str(),nullptr)!=FALSE,"NTFS hardlink fixture");rejected=false;try{readUpdateFile(root/L"README.md");}catch(...){rejected=true;}require(rejected,"hardlinked update files rejected");fs::remove(root/L"link");
  auto expectTamperRejected=[&](const std::vector<unsigned char>&bad,const char*message){
   writeUpdateFile(root/transactionJournal,bad);bool refused=false;try{recoverUpdate(root);}catch(...){refused=true;}require(refused,message);
   require(readUpdateFile(root/transactionJournal)==bad,"rejected journal preserved unchanged");
   for(auto name:transactionFiles){require(sha256(readUpdateFile(root/transactionBackup/wide(name)))==j["files"][name]["before"].str(),"all backups preserved after journal rejection");require(sha256(readUpdateFile(root/wide(name)))==j["files"][name]["before"].str(),"installed files unchanged after journal rejection");}
  };
  require(cipher.size()>4+64+10,"protected journal envelope size");
  auto bad=cipher;bad[10]^=1;expectTamperRejected(bad,"tampered DPAPI journal rejected with backups preserved");
  // Alter the protected blob's authentication data and recompute the public
  // checksum. DPAPI/inner integrity validation must still reject corruption.
  bad=cipher;bad[bad.size()-65]^=1;
  auto checksum=sha256(std::vector<unsigned char>(bad.begin()+4,bad.end()-64));
  std::copy(checksum.begin(),checksum.end(),bad.end()-64);
  expectTamperRejected(bad,"modified protected blob rejected even with recomputed checksum");
  bad=cipher;bad[3]='2';expectTamperRejected(bad,"legacy journal preserved for manual recovery");
  writeUpdateFile(root/transactionJournal,cipher);require(loadTransaction(root).dump()==j.dump(),"original journal remains readable after tamper tests");fs::remove_all(root);
 }catch(...){std::error_code ec;fs::remove_all(root,ec);throw;}
 std::cout<<"PASS real process-abort journal durability, DPAPI tamper, target binding, parent locks and hardlink refusal\n";
}
int wmain(int argc,wchar_t**argv){try{
 require(secureDllLoading(),"System32 DLL policy");
 if(argc==3&&std::wstring(argv[1])==L"--renewed-resources"){
  HMODULE module=LoadLibraryExW(argv[2],nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);require(module!=nullptr,"built DesktopPet loads as data only");
  try{for(int pet:{0,1,3,4})for(int coat=0;coat<builtinCoats(pet);++coat){int index=renewedAppearance(pet,coat);auto resource=FindResourceW(module,MAKEINTRESOURCEW(301+index),RT_RCDATA);require(resource!=nullptr,"all sixteen original coat IDs have renewed resources");auto loaded=LoadResource(module,resource);auto bytes=static_cast<const unsigned char*>(LockResource(loaded));auto size=SizeofResource(module,resource);require(bytes&&size,"renewed resource bytes available");std::vector<unsigned char>data(bytes,bytes+size);RenewedPet art;art.select(pet,coat,data);require(art.clip().frames.size()==6,"all six native poses available");auto original=readBytes(fs::path(PET_TEST_FIXTURES).parent_path().parent_path()/L"assets"/L"renewed"/(L"pet-"+std::to_wstring(pet)+L"-"+std::to_wstring(coat)+L".ocp"));require(sha256(data)==sha256(original),"embedded art matches the selected coat source exactly");}}
  catch(...){FreeLibrary(module);throw;}FreeLibrary(module);std::cout<<"PASS sixteen embedded renewed appearances, original coat IDs and native poses\n";return 0;
 }

 if(argc==3&&std::wstring(argv[1])==L"--dll-policy"){
  auto dir=appData()/L"test"/wide(randomId());fs::create_directories(dir);auto name=L"DesktopPetProbe-"+wide(randomId())+L".dll";auto file=dir/name;fs::copy_file(argv[2],file);auto previous=fs::current_path();
  try{HMODULE module=LoadLibraryExW(file.c_str(),nullptr,LOAD_LIBRARY_SEARCH_SYSTEM32);require(module!=nullptr,"absolute benign fixture DLL is valid");auto probe=reinterpret_cast<int(*)()>(GetProcAddress(module,"DesktopPetProbe"));require(probe&&probe()==130,"benign fixture export callable");FreeLibrary(module);fs::current_path(dir);module=LoadLibraryW(name.c_str());if(module)FreeLibrary(module);require(!module,"current-directory DLL planting is blocked by actual loader policy");fs::current_path(previous);fs::remove_all(dir);}catch(...){fs::current_path(previous);std::error_code ec;fs::remove_all(dir,ec);throw;}std::cout<<"PASS valid absolute test DLL loads but current-directory planting is refused\n";return 0;
 }

 if(argc==3&&std::wstring(argv[1])==L"--journal-abort"){auto dir=fs::path(argv[2]);auto j=testTransaction(dir);prepareUpdate(dir,j);ExitProcess(23);}

 if(argc==3&&std::wstring(argv[1])==L"--ocean-resources"){
  HMODULE module=LoadLibraryExW(argv[2],nullptr,LOAD_LIBRARY_AS_DATAFILE|LOAD_LIBRARY_AS_IMAGE_RESOURCE);require(module!=nullptr,"built DesktopPet loads as data only");
  try{for(int i=0;i<6;++i){auto resource=FindResourceW(module,MAKEINTRESOURCEW(201+i),RT_RCDATA);require(resource!=nullptr,"all six marine resources linked into executable");auto loaded=LoadResource(module,resource);auto bytes=static_cast<const unsigned char*>(LockResource(loaded));auto size=SizeofResource(module,resource);require(bytes&&size,"resource bytes available");std::vector<unsigned char>data(bytes,bytes+size);auto clip=decodeOcean(data);require(clip.frames.size()==6,"embedded native ocean decoding");auto original=readBytes(fs::path(PET_TEST_FIXTURES).parent_path().parent_path()/L"assets"/L"ocean"/(L"ocean-"+std::to_wstring(i)+L".ocp"));require(sha256(data)==sha256(original),"resource exactly matches verified source asset");}}
  catch(...){FreeLibrary(module);throw;}FreeLibrary(module);std::cout<<"PASS six embedded ocean resources, exact payloads and native decoding\n";return 0;
 }

 if((argc==9||argc==11)&&std::wstring(argv[1])==L"--handoff"){
  if(argc==11)require(std::wstring(argv[9])==L"--language"&&std::wstring(argv[10])==wide(languageCode(uiLanguage())),"optional locale preserves legacy handoff arguments");
  if(fs::path(argv[2]).filename()==L"exit-before-ready")return 4;
  HANDLE parent=reinterpret_cast<HANDLE>(uintptr_t(std::stoull(argv[6]))),accepted=reinterpret_cast<HANDLE>(uintptr_t(std::stoull(argv[7]))),abort=reinterpret_cast<HANDLE>(uintptr_t(std::stoull(argv[8])));
  if(!validateUpdateParent(parent,DWORD(std::stoul(argv[4])),std::stoull(argv[5]),fs::path(argv[3])/L"windows_tests.exe"))return 5;
  if(!SetEvent(accepted))return 6;HANDLE wait[]={abort,parent};return WaitForMultipleObjects(2,wait,FALSE,3000)==WAIT_OBJECT_0?0:7;
 }

 if((argc==4||argc==5)&&std::wstring(argv[1])==L"--trust-chain"){auto chain=Json::parse(readText(argv[2],65536));require(authorizedSigningKey(chain,utf8(argv[3]),argc==5&&std::wstring(argv[4])==L"--emergency"),"signing key must be authorized by existing roots and policy chain");return 0;}
 if(argc==3&&std::wstring(argv[1])==L"--package"){
  auto original=readBytes(argv[2]);require(inspectZip(original).size()==5,"Release package format");auto dir=appData()/L"test"/wide(randomId());fs::create_directories(dir);
  try{atomicWrite(dir/L"package.zip",original.data(),original.size());UpdateInfo info;info.size=original.size();Json manifest;manifest["sha256"]=sha256(original);auto text=manifest.dump();info.manifest.assign(text.begin(),text.end());auto verified=verifyPackage(dir,info);original[0]^=1;atomicWrite(dir/L"package.zip",original.data(),original.size());bool rejected=false;try{verifyPackage(dir,info);}catch(...){rejected=true;}require(rejected&&inspectZip(verified).size()==5,"verified buffer retained and later file swap rejected");fs::remove_all(dir);}catch(...){std::error_code ec;fs::remove_all(dir,ec);throw;}
  std::cout<<"PASS package structure, CRC, allowlist, verified buffer and mutation rejection\n";return 0;
 }
 if(argc==5&&std::wstring(argv[1])==L"--signature"){require(verifySignature(readBytes(argv[3]),readBytes(argv[4]),utf8(argv[2])),"PowerShell/native ECDSA interoperability");std::cout<<"PASS PowerShell signature\n";return 0;}
 for(int lang=0;lang<3;++lang){setUILanguage(Language(lang));require(utf8(tw(TextId::Close))==tr(TextId::Close,Language(lang)),"native cached translations switch immediately");require(std::wstring(uiFontName()).size()>0,"locale font selected");}setUILanguage(Language::ZhTW);
 transactionFilesystemTests();
 require(validateUpdateParent(GetCurrentProcess(),GetCurrentProcessId(),processCreation(GetCurrentProcess()),executable()),"live self identity proof");
 require(!validateUpdateParent(GetCurrentProcess(),GetCurrentProcessId(),processCreation(GetCurrentProcess())+1,executable()),"reused PID/creation mismatch rejected");
 require(!validateUpdateParent(GetCurrentProcess(),GetCurrentProcessId()+1,processCreation(GetCurrentProcess()),executable()),"unrelated PID rejected");
 require(sameFileIdentity(executable(),executable().parent_path()/L"."/executable().filename()),"file identity handles path alias");
 {UpdateHandoff probe;probe.start(executable(),L"probe",executable().parent_path());ULONGLONG until=GetTickCount64()+3000;int state=0;while((state=probe.poll())==0&&GetTickCount64()<until)Sleep(10);require(state==1,"inherited restricted handles and validated ready handshake");probe.reset();}
 {UpdateHandoff probe;probe.start(executable(),L"exit-before-ready",executable().parent_path());ULONGLONG until=GetTickCount64()+3000;int state=0;while((state=probe.poll())==0&&GetTickCount64()<until)Sleep(10);require(state==-1,"updater early exit does not authorize shutdown");probe.reset();}
 std::cout<<"PASS parent identity, handle inheritance, ready/cancel and early-exit handoff\n";
 trustCryptoTests();
 auto fixtures=fs::u8path(PET_TEST_FIXTURES);auto alpha=decodeCustom(fixtures/L"alpha.png");
 require(alpha.width==7&&alpha.height==9&&alpha.frames.size()==1,"native PNG alpha crop");require(alpha.frames[0].pixels[0]==0x80800000,"native PNG premultiplied alpha");
 auto animation=decodeCustom(fixtures/L"disposal.gif");auto expected=Json::parse(readText(fixtures/L"disposal.json"));
 require(animation.width==int(expected["width"].num())&&animation.height==int(expected["height"].num()),"GIF stable crop");
 require(animation.frames.size()==expected["frames"].items().size(),"native GIF frame count");
 for(size_t i=0;i<animation.frames.size();++i){auto&frame=animation.frames[i];auto&pixels=expected["frames"].items()[i].items();require(frame.delay==unsigned(expected["delays"].items()[i].num()),"native GIF timing");require(frame.pixels.size()==pixels.size(),"GIF composite size");for(size_t p=0;p<pixels.size();++p)require(frame.pixels[p]==uint32_t(pixels[p].num()),"GIF partial-frame transparency/disposal composition");}
 std::cout<<"PASS native PNG/GIF alpha, crop, timing, disposal fixtures\n";
 std::vector<unsigned char> abc={'a','b','c'};require(sha256(abc)=="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad","SHA256 reference");
 BCRYPT_ALG_HANDLE algorithm=nullptr;BCRYPT_KEY_HANDLE key=nullptr;require(BCryptOpenAlgorithmProvider(&algorithm,BCRYPT_ECDSA_P256_ALGORITHM,nullptr,0)>=0,"ECDSA provider");require(BCryptGenerateKeyPair(algorithm,&key,256,0)>=0&&BCryptFinalizeKeyPair(key,0)>=0,"Generate ephemeral test key");ULONG n=0;require(BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,nullptr,0,&n,0)>=0,"Export size");std::vector<unsigned char>blob(n);require(BCryptExportKey(key,nullptr,BCRYPT_ECCPUBLIC_BLOB,blob.data(),n,&n,0)>=0,"Export key");require(blob.size()==72,"P256 blob");std::vector<unsigned char>point(blob.begin()+8,blob.end());auto digest=unhex(sha256(abc));std::vector<unsigned char>sig(64);require(BCryptSignHash(key,nullptr,digest.data(),ULONG(digest.size()),sig.data(),64,&n,0)>=0&&n==64,"ECDSA sign");require(verifySignature(abc,sig,hex(point)),"valid signature");abc[0]='x';require(!verifySignature(abc,sig,hex(point)),"tampered message rejected");abc[0]='a';sig[0]^=1;require(!verifySignature(abc,sig,hex(point)),"tampered signature rejected");sig.resize(63);require(!verifySignature(abc,sig,hex(point)),"short signature rejected");require(!verifySignature(abc,sig,""),"missing key rejected");BCryptDestroyKey(key);BCryptCloseAlgorithmProvider(algorithm,0);
 auto dir=appData()/L"test"/wide(randomId());try{fs::create_directories(dir);Config c;writeText(dir/L"config.json",c.json().dump());c.name="原子寫入測試";saveConfig(dir/L"config.json",c);Config loaded;loaded.load(Json::parse(readText(dir/L"config.json")),false);require(loaded.name==c.name,"UTF8 atomic configuration");require(quote(L"a b\\")==L"\"a b\\\\\"","Windows command quote");require(wide(utf8(L"小橘🐱"))==L"小橘🐱","UTF8 conversion");fs::remove_all(dir);}catch(...){std::error_code ec;fs::remove_all(dir,ec);throw;}
 bool rejected=false;try{verifyManifest({'{','}'},std::vector<unsigned char>(64));}catch(...){rejected=true;}require(rejected,"unsigned update rejected");std::cout<<"PASS Windows SHA256 / ECDSA tampering / atomic config / UTF8 / process quoting\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
