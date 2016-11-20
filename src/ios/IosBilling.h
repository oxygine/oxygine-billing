#pragma once
#include <vector>
#include <string>
#include <map>
using namespace std;

void        iosBillingInit();
void        iosBillingFree();
void        iosBillingPurchase( const string & prodID, const string & payLoad );
void        iosRestore();

void        iosBillingGetPurchases();
void        iosBillingUpdate(const std::vector<std::string>& items);

void        iosBillingConsume(const string &token);

