

#ifndef Oxygine_BillingIOS_h
#define Oxygine_BillingIOS_h

#include <vector>
#include <string>

#ifdef __OBJC__

#endif


using namespace std;

void iosBillingInit();
void iosBillingFree();
void iosBillingUpdate(const vector<string>& items);
void iosBillingPurchase(const string& product, const string& payload);
void iosBillingConsume(const string& token);
void iosBillingGetPurchases();

bool iosBillingHasProduct(const string &);
#endif
