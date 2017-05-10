#pragma once
#include <string>
#include <vector>
#include "oxygine/json/json.h"
using namespace std;

void billingSimulatorInit();
void billingSimulatorFree();
void billingSimulatorPurchase(const std::string& id, const std::string& payload);
void billingSimulatorConsume(const std::string& token);
void billingSimulatorGetPurchases();
void billingSimulatorRequestDetails(const vector<std::string>& items);
void billingSimulatorSetDetails(const Json::Value& details);
