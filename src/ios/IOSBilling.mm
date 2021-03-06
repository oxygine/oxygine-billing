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
#include "ox/oxygine.hpp"
#include "ox/ThreadDispatcher.hpp"

#include "billing.h"

using namespace oxygine;

@interface Billing:NSObject<SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
    //NSArray *_products;
    NSMutableDictionary *_products;
    NSMutableDictionary *_transactionsDict;
    NSMutableArray* _transactionsQueue;
    BOOL _ready;
}


@end

@implementation Billing


-(id)init {
    self = [super init];
    
    
    _products = [NSMutableDictionary dictionary];
    _transactionsDict = [NSMutableDictionary dictionary];
    _transactionsQueue = [NSMutableArray array];
    _ready = false;
    
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    
    return self;
}

-(void)free {
    _ready = false;
}


-(void)updateProducts:(NSArray*)items
{
    SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
                                          initWithProductIdentifiers: [NSSet setWithArray:items]];
    productsRequest.delegate = self;
    [productsRequest start];
}


- (void)handleQueue:(SKPaymentQueue *)queue
 updatedTransactions:(NSArray *)transactions
{
    for (SKPaymentTransaction *transaction in transactions) {
        
        const char *trID = transaction.transactionIdentifier ? transaction.transactionIdentifier.UTF8String : "";
        const char *prodID = transaction.payment.productIdentifier.UTF8String;
        logs::messageln("billing::transaction %d '%s' '%s'", transaction.transactionState, trID, prodID);
        
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
                [_transactionsDict setObject:transaction forKey:transaction.transactionIdentifier];
                
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


- (void)paymentQueue:(SKPaymentQueue *)queue
 updatedTransactions:(NSArray *)transactions
{
    if (_ready)
    {
        [self handleQueue:queue updatedTransactions:transactions];
    }
    else
    {
        [_transactionsQueue addObjectsFromArray:transactions];
    }
}

-(void)ready {
    _ready = true;
}

- (void)restore {
    
    if (!_ready)
        return;
    
    NSArray *all = [_transactionsDict allValues];
    NSArray *purchased = [all filteredArrayUsingPredicate:[NSPredicate predicateWithBlock:^BOOL(id object, NSDictionary *bindings) {
        return ((SKPaymentTransaction*)object).transactionState == SKPaymentTransactionStatePurchased;
    }]];
    
    [self handleQueue:[SKPaymentQueue defaultQueue] updatedTransactions: purchased];
    [self handleQueue:[SKPaymentQueue defaultQueue] updatedTransactions:_transactionsQueue];
    [_transactionsQueue removeAllObjects];
}

- (void)productsRequest:(SKProductsRequest *)request
     didReceiveResponse:(SKProductsResponse *)response
{
    NSArray *products = [response products];
    
    NSArray *inv = response.invalidProductIdentifiers;
    
    for (NSString *invalidIdentifier in response.invalidProductIdentifiers) {
        // Handle any invalid product identifiers.
    }
    
    
    Json::Value data(Json::arrayValue);
    
    
    for (SKProduct *product in response.products) {
        [_products setObject:product forKey:product.productIdentifier];
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
    
    SKPaymentTransaction *trans = _transactionsDict[str];
    if (!trans)
        return;
    
    [[SKPaymentQueue defaultQueue] finishTransaction:trans];
    [_transactionsDict removeObjectForKey:trans.transactionIdentifier];
}

@end

Billing *_billing = 0;


void iosBillingInit()
{
    if (_billing)
        return;
    _billing = [[Billing alloc] init];
}

void iosBillingFree()
{
    [_billing free];
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
    [_billing ready];
    core::getMainThreadDispatcher().postCallback([](){
        [_billing restore];
    });
}

bool iosBillingHasProduct(const string &product)
{
    return [_billing getProduct:product.c_str()] != 0;
}
