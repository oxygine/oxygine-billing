//
//  BillingIOS.m
//  Match3Quest
//
//  Created by Denis on 15/01/15.
//  Copyright (c) 2015 Mac. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>
#include "IOSBilling.h"


#include "billing.h"

using namespace oxygine;

@interface Billing:NSObject<SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
    //NSArray *_products;
    NSMutableDictionary *_products;
    NSMutableDictionary *_transactions;
}


@end

@implementation Billing


-(id)init {
    self = [super init];
    
    
    _products = [NSMutableDictionary dictionary];
    _transactions = [NSMutableDictionary dictionary];
    
    return self;
}

-(void)free {
    [[SKPaymentQueue defaultQueue] removeTransactionObserver:self];
}


-(void)updateProducts:(NSArray*)items
{
    SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
                                          initWithProductIdentifiers: [NSSet setWithArray:items]];
    productsRequest.delegate = self;
    [productsRequest start];
}



- (void)paymentQueue:(SKPaymentQueue *)queue
 updatedTransactions:(NSArray *)transactions
{
    for (SKPaymentTransaction *transaction in transactions) {
        
        const char *trID = transaction.transactionIdentifier ? transaction.transactionIdentifier.UTF8String : "";
        const char *prodID = transaction.payment.productIdentifier.UTF8String;
        log::messageln("billing::transation %d '%s' '%s'", transaction.transactionState, trID, prodID);
        
        switch (transaction.transactionState)
        {
                // Call the appropriate custom method for the transaction state.
            case SKPaymentTransactionStatePurchasing:
                break;
                
            case SKPaymentTransactionStateDeferred:
                break;
                
            case SKPaymentTransactionStateFailed:
            {
                [queue finishTransaction:transaction];
                
                Json::Value data(Json::objectValue);
                
                data["transactionIdentifier"] = trID;
                data["productIdentifier"] = prodID;
                data["errorCode"] = (int)transaction.error.code;
                
                Json::FastWriter writer;
                
                if (transaction.error.code == SKErrorPaymentCancelled)
                    billing::internal::purchased(billing::internal::ActivityOK, billing::internal::RC_Canceled, writer.write(data), "", "");
                else
                    billing::internal::purchased(billing::internal::ActivityOK + 1, 0, writer.write(data), "", "");
            }
                break;
                
            case SKPaymentTransactionStatePurchased:
            {
                [_transactions setObject:transaction forKey:transaction.transactionIdentifier];
                
                Json::Value data(Json::objectValue);
                
                data["transactionIdentifier"] = trID;
                data["productIdentifier"] = prodID;
                
                NSString* str = [[NSString alloc] initWithData:transaction.transactionReceipt encoding:NSUTF8StringEncoding];
                string s = [str UTF8String];
                data["transactionReceipt"] = s;
                
                Json::FastWriter writer;
                
                NSString *userData = transaction.payment.applicationUsername;
                
                
                billing::internal::purchased(billing::internal::ActivityOK, billing::internal::RC_OK,
                                             writer.write(data), "", userData ? [userData UTF8String] : "");
            }
                break;
                
            case SKPaymentTransactionStateRestored:
                break;
                
            default:
                break;
        }
    }
}



- (void)restore {
    
    NSArray *all = [_transactions allValues];
    NSArray *purchased = [all filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(id object, NSDictionary *bindings) {
        return ((SKPaymentTransaction*)object).transactionState == SKPaymentTransactionStatePurchased;
    }]];
    
    [self paymentQueue:[SKPaymentQueue defaultQueue]  updatedTransactions: purchased];
    
    [[SKPaymentQueue defaultQueue] removeTransactionObserver:self];
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    
}

- (void)productsRequest:(SKProductsRequest *)request
     didReceiveResponse:(SKProductsResponse *)response
{
    NSArray *products = [response products];
    
    NSArray *inv = response.invalidProductIdentifiers;
    int q=0;
    
    
    for (NSString *invalidIdentifier in response.invalidProductIdentifiers) {
        // Handle any invalid product identifiers.
    }
    
    
    Json::Value data(Json::arrayValue);
    
    
    for (SKProduct *product in response.products) {
        [_products setObject:product forKey:product.productIdentifier];
        
        //Json::Value item(Json::ArrayIndex);
        //data.append(item);
    }
    
    
    
    Json::FastWriter writer;
    
    billing::internal::detailed(writer.write(data));
}

- (SKProduct*) getProduct:(const char *)name
{
    NSString *str = [NSString stringWithUTF8String:name];
    
    return _products[str];
}

- (void)purchase:(const char *)prod
                :(const char*)payload
{
    SKProduct *product = [self getProduct:prod];
    if (!product)
        return;
    
    SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product];
    payment.quantity = 1;
    payment.applicationUsername = [NSString stringWithUTF8String:payload];
    
    [[SKPaymentQueue defaultQueue] addPayment:payment];
}

- (void)consume:(const char *)token
{
    NSString *str = [NSString stringWithUTF8String:token];
    
    SKPaymentTransaction *trans = _transactions[str];
    if (!trans)
        return;
    
    [[SKPaymentQueue defaultQueue] finishTransaction:trans];
    [_transactions removeObjectForKey:trans.transactionIdentifier];
}

@end

Billing *_billing = 0;


void iosBillingInit()
{
    _billing = [[Billing alloc] init];
}

void iosBillingFree()
{
    [_billing free];
    _billing = 0;
}

void iosBillingUpdate(const vector<string> &items)
{
    NSArray *array = [[NSArray alloc] init];
    
    for (size_t i = 0; i < items.size(); ++i)
    {
        const string &item = items[i];
        
        NSString *str = [NSString stringWithUTF8String:item.c_str()];
        array = [array arrayByAddingObject:str];
    }
    
    [_billing updateProducts:array];
}

void iosBillingPurchase(const string &product, const string& payload)
{
    [_billing purchase: product.c_str() : payload.c_str()];
}

void iosBillingConsume(const string &token)
{
    [_billing consume:token.c_str()];
}

void iosBillingGetPurchases()
{
    [_billing restore];
}

bool iosBillingHasProduct(const string &product)
{
    return [_billing getProduct:product.c_str()] != 0;
}
