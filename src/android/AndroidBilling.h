#pragma once
#include <vector>
#include <string>
using namespace std;

void jniBillingInit();
void jniBillingFree();
void jniBillingUpdate(const vector<string>& ids);
void jniBillingPurchase(const string& sku, const string& payload);
void jniBillingGetPurchases();
void jniBillingConsume(const string& token);
string jniBillingGetCurrency();