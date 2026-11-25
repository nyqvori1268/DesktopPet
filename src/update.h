#pragma once
#include "platform.h"
#include <thread>
#include <functional>
#include <mutex>
#include <atomic>
namespace pet {
struct UpdateInfo{std::string version,notes,base;size_t size=0;std::vector<unsigned char> manifest,signature;};
UpdateInfo verifyManifest(const std::vector<unsigned char>&m,const std::vector<unsigned char>&s);
void verifyPackage(const fs::path&stage,const UpdateInfo&info);
class UpdateClient{
 std::thread worker;std::mutex mutex;UpdateInfo info;std::string status;fs::path staged;std::atomic<bool> cancelFlag{false},busyFlag{false};std::atomic<int> progressFlag{0};
 void work(std::function<void()> f);
public:
 ~UpdateClient(){cancelFlag=true;if(worker.joinable())worker.join();}
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
