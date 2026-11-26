#pragma once
#include "platform.h"
#include "trust_logic.h"
#include "transaction_logic.h"
namespace pet {
bool verifyTrustedManifest(const std::vector<unsigned char>&message,const std::vector<unsigned char>&signature);
void checkTrustVersion(const Json&manifest);
void commitInstalledTrust(const std::vector<unsigned char>&message,const std::vector<unsigned char>&signature,const std::string&installedVersion);
TrustCommit installedTrustCommit(const std::string&oldVersion,const std::string&version,const std::string&receipt);
bool authorizedSigningKey(const Json&chain,const std::string&point,bool emergency);
}
