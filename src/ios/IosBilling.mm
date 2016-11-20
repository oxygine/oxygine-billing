#include <assert.h>
#include "core/oxygine.h"
#include "core/Object.h"
#include "core/ThreadDispatcher.h"
#include "core/oxygine.h"
#include "IosBilling.h"
#include "billing.h"
#include "json/json.h"
#import <StoreKit/StoreKit.h>

#include <set>

using namespace oxygine;

static std::map< std::string, std::string >              gs_PayLoads;
static std::map< std::string, SKPaymentTransaction* >    gs_Transactions;
static std::map< std::string, SKProduct* >               gs_Products;
static std::set< std::string >                           gs_TriedBuys;


@interface INAPP_PURCHASE : NSObject  <SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
    SKProductsRequest *productsRequest;
    NSSet* itemsDetailed;
}
- (IBAction)restore;
- (IBAction)startPurchase:( NSString* )productID withPayLoad:(NSString*) payLoad;
- (void)requestProducts:( NSSet* )items;

@end

static INAPP_PURCHASE * gTransactor = NULL;

void iosBillingInit()
{
    gTransactor = [ [ INAPP_PURCHASE alloc ] init ];
}

void iosBillingFree()
{
    gTransactor = nil;
}



void iosBillingPurchase( const string & prodID, const string & payLoad )
{
    gs_PayLoads[ prodID ] = payLoad;
    NSString *prID =  [NSString stringWithFormat: @"%s", prodID.c_str()];
    NSString *pL =  [NSString stringWithFormat: @"%s", payLoad.c_str()];
    
    gs_TriedBuys.insert( prodID );
    [gTransactor startPurchase: prID withPayLoad:pL];
}

void iosRestore()
{
    [ gTransactor restore ];
}

void iosBillingConsume(const string &token)
{
    std::map< std::string, SKPaymentTransaction* >::iterator iter = gs_Transactions.find( token );
    if( iter != gs_Transactions.end() )
    {
        SKPaymentTransaction * tr = iter->second;
        [[SKPaymentQueue defaultQueue] finishTransaction:tr];
    }
}

void iosBillingGetPurchases()
{
    [ gTransactor restore ];
}

void iosBillingUpdate(const std::vector<std::string>& items)
{
    
    NSMutableSet *productIdentifiers = [[NSSet alloc]init];
    
    NSMutableSet * pitems = [[ NSMutableSet alloc ]init];
    for ( int i = 0; i < ( int )items.size(); i++ )
    {
        NSString * st = [ NSString stringWithFormat:@"%s", items[ i ].c_str() ];
        [ pitems addObject:st ];
    }
   // [ pitems addObject:nil ];
    
    
    [ gTransactor requestProducts:pitems ];
   
}


@implementation INAPP_PURCHASE //the name of your view controller (same as above)

-(void)requestProducts:( NSSet* )items
{
    if([SKPaymentQueue canMakePayments])
    {
        itemsDetailed = items;
        
        productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:items];
        productsRequest.delegate = self;
        [productsRequest start];
        
        
    }
}

- (IBAction)startPurchase:( NSString* )productID withPayLoad:(NSString*) payLoad
{
    NSLog(@"User requests to remove ads");
    
    if([SKPaymentQueue canMakePayments]){
        NSLog(@"User can make payments");
        
        
        NSString *message =  [NSString stringWithFormat: productID];
        NSSet *productIdentifiers = [NSSet setWithObject:message];
        productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:productIdentifiers];
        productsRequest.delegate = self;
        [productsRequest start];
        
        
    }
    else{
       
        NSLog(@"User cannot make payments due to parental controls");
        //this is called the user cannot make payments, most likely due to parental controls
    }
}

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response{
    SKProduct *validProduct = nil;
    
    //gs_TriedBuys here
    
    int count = [response.products count];
    if(count > 0){
        
        for (SKProduct *item in response.products)
        {
            std::string prID = item.productIdentifier.UTF8String;
            gs_Products[ prID ] = item;
            std::set< std::string >::iterator iter = gs_TriedBuys.find( prID );
            if( gs_TriedBuys.end() != iter )
            {
                gs_TriedBuys.erase(iter);
                [self purchase:item];
            }
        }
        if( itemsDetailed && count == [ itemsDetailed count ] )
        {
                //billing::internal::detailed(<#const Json::Value &#>) here;
            itemsDetailed = nil;
        }
        
    }
    else if(!validProduct){
       
        NSLog(@"No products available");
        //this is called if your product id is not valid, this shouldn't be called unless that happens.
    }
}

- (void)purchase:(SKProduct *)product{
    
    std::string prID = product.productIdentifier.UTF8String;
    std::map< std::string, std::string >::iterator iter = gs_PayLoads.find( prID );
    std::string payLoad = "";
    if( iter != gs_PayLoads.end() )
    {
        payLoad = iter->second;
        gs_PayLoads.erase(iter);
    }
    
    SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product];
    
    //Populate applicationUsername with your customer's username on your server
    payment.applicationUsername = [ NSString stringWithFormat:@"%s", payLoad.c_str() ];//@"userdata";//[self hashedValueForAccountName:@"userNameOnYourServer"];
    
    
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    [[SKPaymentQueue defaultQueue] addPayment:payment];
}

- (IBAction) restore{
    //this is called when the user restores purchases, you should hook this up to a button
    [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    [[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void) paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
    NSLog(@"received restored transactions: %i", queue.transactions.count);
    for(SKPaymentTransaction *transaction in queue.transactions){
        if(transaction.transactionState == SKPaymentTransactionStateRestored){
            
            NSString *productID = transaction.payment.productIdentifier;
            //called when the user successfully restores a purchase
            NSLog(@"Transaction state -> Restored");
            [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
            break;
        }
    }
    
}

-( void )request:( SKRequest* )request didFailWithError:(NSError *)error
{
    
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions{
    for(SKPaymentTransaction *transaction in transactions){
        switch(transaction.transactionState){
            case SKPaymentTransactionStatePurchasing: NSLog(@"Transaction state -> Purchasing");
                //called when the user is in the process of purchasing, do not add any of your own code here.
                break;
            case SKPaymentTransactionStatePurchased:
            {
                //[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                NSLog(@"Transaction state -> Purchased");
                
                SKMutablePayment * mp = ( SKMutablePayment * )transaction.payment;
                
                std::map< std::string, SKProduct* >::iterator iter = gs_Products.find( mp.productIdentifier.UTF8String );
                
                
                
                
                
                const char * trID = transaction.transactionIdentifier.UTF8String;
                gs_Transactions[ std::string( trID ) ] = transaction;
                
                if( iter != gs_Products.end() )
                {
                    std::string payLoad = mp.applicationUsername.UTF8String;
                    billing::internal::purchased("pr", "fake");
                }
                
                
            }
                break;
            case SKPaymentTransactionStateRestored:
                NSLog(@"Transaction state -> Restored");
                //add the same code as you did from SKPaymentTransactionStatePurchased here
                [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                break;
            case SKPaymentTransactionStateFailed:
                //called when the transaction does not finish
                if(transaction.error.code == SKErrorPaymentCancelled)
                {
                    NSLog(@"Transaction state -> Cancelled");
                    //the user cancelled the payment ;(
                }
                
                [[SKPaymentQueue defaultQueue] finishTransaction:transaction];
                break;
        }
    }
}

@end

