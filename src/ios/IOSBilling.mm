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

#import <CommonCrypto/CommonCrypto.h>

// Custom method to calculate the SHA-256 hash using Common Crypto
- (NSString *)hashedValueForAccountName:(NSString*)userAccountName
{
    const int HASH_SIZE = 32;
    unsigned char hashedChars[HASH_SIZE];
    const char *accountName = [userAccountName UTF8String];
    size_t accountNameLen = strlen(accountName);
    
    // Confirm that the length of the user name is small enough
    // to be recast when calling the hash function.
    if (accountNameLen > UINT32_MAX) {
        NSLog(@"Account name too long to hash: %@", userAccountName);
        return nil;
    }
    CC_SHA256(accountName, (CC_LONG)accountNameLen, hashedChars);
    
    // Convert the array of bytes into a string showing its hex representation.
    NSMutableString *userAccountHash = [[NSMutableString alloc] init];
    for (int i = 0; i < HASH_SIZE; i++) {
        // Add a dash every four bytes, for readability.
        if (i != 0 && i%4 == 0) {
            [userAccountHash appendString:@"-"];
        }
        [userAccountHash appendFormat:@"%02x", hashedChars[i]];
    }
    
    return userAccountHash;
}

-(id)init {
    self = [super init];
    
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    
    _products = [NSMutableDictionary dictionary];
    _transactions = [NSMutableDictionary dictionary];
    
    return self;
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
        switch (transaction.transactionState) {
                // Call the appropriate custom method for the transaction state.
            case SKPaymentTransactionStatePurchasing:
                //[self showTransactionAsInProgress:transaction deferred:NO];
                log::messageln("billing::transactions SKPaymentTransactionStatePurchasing %s", transaction.transactionIdentifier.UTF8String);
                break;
                
            case SKPaymentTransactionStateDeferred:
                log::messageln("billing::transactions SKPaymentTransactionStateDeferred %s", transaction.transactionIdentifier.UTF8String);
                //[self showTransactionAsInProgress:transaction deferred:YES];
                break;
                
            case SKPaymentTransactionStateFailed:
                log::messageln("billing::transactions SKPaymentTransactionStateFailed %s", transaction.transactionIdentifier.UTF8String);
                break;
                
            case SKPaymentTransactionStatePurchased: {
            
                
                log::messageln("billing::transactions SKPaymentTransactionStatePurchased %s", transaction.transactionIdentifier.UTF8String);
                [_transactions setObject:transaction forKey:transaction.transactionIdentifier];
                //[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                
                /*
                const data::goods *g = [self getGoods:transaction.payment.productIdentifier];
                SKProduct *product = (__bridge SKProduct*)g->userData;
                
                billing::details det;
                det.orderID = transaction.transactionIdentifier.UTF8String;
                
                NSString *currencyCode = [product.priceLocale objectForKey:NSLocaleCurrencyCode];
                det.currency = currencyCode.UTF8String;
                det.price = product.price.doubleValue;
                det.sandbox = true;
                billing::addCurrency(g, false, &det);
                 */
                //[self completeTransaction:transaction];
                //string data;
                string sign;
                
                Json::Value data(Json::objectValue);
                
                data["transactionIdentifier"] = transaction.transactionIdentifier;
                data["productIdentifier"] = transaction.payment.productIdentifier;
                
                
                Json::FastWriter writer;
                
                billing::internal::purchased(billing::internal::ActivityOK, billing::internal::RC_OK, writer.write(data), sign);
            }
                break;
            case SKPaymentTransactionStateRestored:
                //[self restoreTransaction:transaction];
                break;
            default:
                // For debugging
                NSLog(@"Unexpected transaction state %@", @(transaction.transactionState));
                break;
        }
    }
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
        
        Json::Value item(Json::ArrayIndex);
        data.append(item);
    }
    
    
    
    Json::FastWriter writer;
    
    billing::internal::detailed(writer.write(data));
}

- (void)purchase:(const char *)prod
{
    
    NSString *str = [NSString stringWithUTF8String:prod];
    
    SKProduct *product = _products[str];
    if (!product)
        return;
    
    SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product];
    payment.quantity = 1;
    //payment.applicationUsername =
    
    [[SKPaymentQueue defaultQueue] addPayment:payment];
}

- (void)consume:(const char *)token
{
    
    NSString *str = [NSString stringWithUTF8String:token];
    
    SKPaymentTransaction *trans = _transactions[str];
    [[SKPaymentQueue defaultQueue] finishTransaction:trans];
}

@end

Billing *_billing = 0;


void iosBillingInit()
{
    _billing = [[Billing alloc] init];
}

void iosBillingFree()
{
    _billing = nil;
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

void iosBillingPurchase(const string &product)
{
    [_billing purchase: product.c_str()];
}

void iosBillingConsume(const string &token)
{
    [_billing consume:token.c_str()];
}

void iosBillingGetPurchases()
{
}
