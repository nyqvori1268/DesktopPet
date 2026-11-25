#pragma once
#include "platform.h"
#include "trust_logic.h"
namespace pet {
bool verifyTrustedManifest(const std::vector<unsigned char>&message,const std::vector<unsigned char>&signature);
void checkTrustVersion(const Json&manifest);
void commitInstalledTrust(const std::vector<unsigned char>&message,const std::vector<unsigned char>&signature,const std::string&installedVersion);
bool authorizedSigningKey(const Json&chain,const std::string&point,bool emergency);
}
