#pragma once
#include "oxygine/EventDispatcher.h"
#include "oxygine/Event.h"
#include "oxygine/json/json.h"
#include <string>

namespace oxygine
{
    namespace billing
    {
        class PurchasedEvent : public Event
        {
        public:
            enum { EVENT_SUCCESS = sysEventID('b', 'p', 'r') };
            enum { EVENT_CANCELED = sysEventID('b', 'c', 'n') };
            enum { EVENT_ERROR = sysEventID('b', 'e', 'r') };

            PurchasedEvent(const std::string& Data1, const std::string& Data2, const std::string& Data3, eventType event) : Event(event), data1(Data1), data2(Data2), data3(Data3) {}

            std::string data1;

            //google "signature"
            //amazon "userId"
            std::string data2;

            //ios username
            std::string data3;
        };

        class ParsePurchasedData
        {
        public:
            ParsePurchasedData(const PurchasedEvent* event);


            Json::Value data;
            Json::Value signature;

            //google "productID"
            //amazon "sku"
            std::string productID;

            //google "purchaseToken"
            //amazon "receiptId"
            //ios    "transactionIdentifier"
            std::string purchaseToken;

            std::string iosTransactionReceipt;

            int         purchaseState;

            std::string payload;
        };

        class DetailsEvent : public Event
        {
        public:
            enum { EVENT = sysEventID('b', 'd', 't') };
            DetailsEvent(const std::string& data_) : Event(EVENT), data(data_) {}

            std::string data;


            /*
            //android
            [
                {
                    "description" : "A little pack of coins.",
                        "price" : "30,00 ?",
                        "price_amount_micros" : 30000000,
                        "price_currency_code" : "RUB",
                        "productId" : "com.package.game.pack1",
                        "title" : "Coins pack 1",
                        "type" : "inapp"
                },
                {
                    "description" : "A middle pack of coins.",
                    "price" : "60,00 ?",
                    "price_amount_micros" : 60000000,
                    "price_currency_code" : "RUB",
                    "productId" : "com.package.game.pack2",
                    "title" : "Coins pack 2",
                    "type" : "inapp"
                },
            ]
            */
        };

        class ParsedDetailsData
        {
        public:
            ParsedDetailsData(const DetailsEvent* event);

            class Item
            {
            public:
                Item() : price_amount_micros(0) {}

                std::string     productId;
                std::string     description;
                std::string     price;
                int64           price_amount_micros;
                std::string     price_currency_code;
                std::string     title;
                std::string     type;
            };

            std::vector<Item> items;
        };

        spEventDispatcher dispatcher();

        /**initializes oxygine-billing module*/
        void init();

        /**free oxygine-billing module*/
        void free();

        bool isInitialized();


        void purchase(const std::string& id, const std::string& payload);
        void consume(const std::string& token);

        /**requestPurchases should be called right after billing::init() or when you are ready to receive purchased
        ios purchases wont work without this;
        */
        void requestPurchases();

        void requestDetails(const std::vector<std::string>& items);

        void simulatorSetDetails(const Json::Value& details);

        enum MarketType
        {
            unknown,
            simulator,
            ios,
            google,
            amazon
        };

        MarketType getMarketType();


        namespace internal
        {
            typedef void(*cbInit)();
            typedef void(*cbFree)();
            typedef void(*cbPurchase)(const std::string& id, const std::string& payload);
            typedef void(*cbConsume)(const std::string&);
            typedef void(*cbRequestPurchases)();
            typedef void(*cbRequestDetails)(const std::vector<std::string>& items);

            extern cbInit                       fInit;
            extern cbFree                       fFree;
            extern cbPurchase                   fPurchase;
            extern cbConsume                    fConsume;
            extern cbRequestPurchases           fRequestPurchases;
            extern cbRequestDetails             fRequestDetails;

            const int ActivityOK = -1;

            const int RC_OK = 0;
            const int RC_Canceled = 1;

            void purchased(int requestCode, int resultCode, const std::string& data1, const std::string& data2, const std::string& data3);
            //void detailed(const std::string&);
            void detailed(const std::string&);
        }
    }
}
