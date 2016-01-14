#pragma once
#include <string>
#include <vector>
#include "json/json.h"
using namespace std;

void billingSimulatorInit();
void billingSimulatorPurchase(const string& id, const string& payload);
void billingSimulatorConsume(const string& token);
void billingSimulatorGetPurchases();
void billingSimulatorRequestDetails(const vector<string>& items);
void billingSimulatorSetDetails(const Json::Value& details);
