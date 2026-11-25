#pragma once
#include "platform.h"
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>
namespace pet {
struct UpdateInfo{std::string version,notes,base;size_t size=0;std::vector<unsigned char> manifest,signature;};
UpdateInfo verifyManifest(const std::vector<unsigned char>&m,const std::vector<unsigned char>&s);
std::vector<unsigned char> verifyPackage(const fs::path&stage,const UpdateInfo&info);
enum class UpdatePhase{Idle,Checking,Available,Downloading,Verifying,Ready,Failed,Cancelled};
class UpdateClient{
 std::thread worker;std::mutex mutex;UpdateInfo info;std::string status;fs::path staged;std::atomic<bool> cancelFlag{false},busyFlag{false};std::atomic<int> progressFlag{0};std::atomic<UpdatePhase> phaseFlag{UpdatePhase::Idle};
 void phase(UpdatePhase value,const std::string&text){std::lock_guard<std::mutex>lock(mutex);status=text;phaseFlag=value;}
 void work(std::function<void()> f);
public:
 ~UpdateClient(){cancelFlag=true;if(worker.joinable())worker.join();}
 UpdatePhase phase()const{return phaseFlag;}
 bool enabled()const;
 bool busy()const{return busyFlag;}
 int progress()const{return progressFlag;}
 void cancel(){cancelFlag=true;}
 void check();void download();
 UpdateInfo latest(){std::lock_guard<std::mutex> lock(mutex);return info;}
 std::string message(){std::lock_guard<std::mutex> lock(mutex);return status;}
 fs::path stage(){std::lock_guard<std::mutex> lock(mutex);return staged;}
};
}
