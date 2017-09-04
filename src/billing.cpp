#include "billing.h"


#if TARGET_OS_IPHONE
#define IOS_STORE 1
#endif

#ifdef __ANDROID__
#include "android/AndroidBilling.h"
#elif IOS_STORE
#include "ios/IOSBilling.h"
#else
#include "sim/BillingSimulator.h"
#endif



namespace oxygine
{
    namespace billing
    {

        namespace internal
        {
            cbInit                       fInit = []() {};
            cbFree                       fFree = []() {};
            cbPurchase                   fPurchase = [](const string&, const string&) {};
            cbConsume                    fConsume = [](const string&) {};
            cbRequestPurchases           fRequestPurchases = []() {};
            cbRequestDetails             fRequestDetails = [](const std::vector<std::string>& items) {};
        }

        using namespace internal;

        spEventDispatcher _dispatcher;
        spEventDispatcher dispatcher()
        {
            return _dispatcher;
        }

        void init()
        {

#ifdef __ANDROID__
            fInit = jniBillingInit;
            fFree = jniBillingFree;
            fPurchase = jniBillingPurchase;
            fConsume = jniBillingConsume;
            fRequestPurchases = jniBillingGetPurchases;
            fRequestDetails = jniBillingUpdate;
#elif IOS_STORE
            fInit = iosBillingInit;
            fFree = iosBillingFree;
            fPurchase = iosBillingPurchase;
            fConsume = iosBillingConsume;
            fRequestPurchases = iosBillingGetPurchases;
            fRequestDetails = iosBillingUpdate;
#else
            fInit = billingSimulatorInit;
            fFree = billingSimulatorFree;
            fPurchase = billingSimulatorPurchase;
            fConsume = billingSimulatorConsume;
            fRequestPurchases = billingSimulatorGetPurchases;
            fRequestDetails = billingSimulatorRequestDetails;
#endif


            log::messageln("billing::init");
            OX_ASSERT(_dispatcher == 0);
            _dispatcher = new EventDispatcher;

            fInit();

            log::messageln("billing::init done");
        }

        void free()
        {
            log::messageln("billing::free");

            if (!_dispatcher)
                return;

            fFree();

            _dispatcher->removeAllEventListeners();
            _dispatcher = 0;

            log::messageln("billing::free done");
        }

        bool isInitialized()
        {
            return _dispatcher != 0;
        }

        MarketType getMarketType()
        {
#ifdef IOS_STORE
            return ios;
#elif __ANDROID__
            string tp = jniBillingGetType();
            if (tp == "google")
                return google;
            if (tp == "amazon")
                return amazon;
            return unknown;
#else

#endif
            return simulator;
        }

        void purchase(const std::string& id, const std::string& payload)
        {
            log::messageln("billing::purchase '%s', payload '%s'", id.c_str(), payload.c_str());

            fPurchase(id, payload);
        }

        void consume(const std::string& token)
        {
            log::messageln("billing::consume");
            fConsume(token);
            log::messageln("billing::consume done");
        }

        void requestPurchases()
        {
            log::messageln("billing::requestPurchases");
            fRequestPurchases();
            log::messageln("billing::requestPurchases done");
        }

        void requestDetails(const std::vector<std::string>& items)
        {
            log::messageln("billing::requestDetails");
            fRequestDetails(items);
            log::messageln("billing::requestDetails done");
        }

        void simulatorSetDetails(const Json::Value& details)
        {
#ifdef __ANDROID__
#elif  IOS_STORE
#else
            billingSimulatorSetDetails(details);
#endif
        }

        std::string getString(const Json::Value& obj, const string& key, const string& def = "")
        {
            if (obj[key].isNull())
                return def;
            return obj[key].asString();
        }

        Json::Int64 getInt64(const Json::Value& obj, const string& key, const Json::Int64 def = 0)
        {
            if (obj[key].isNull())
                return def;
            return obj[key].asInt64();
        }

        ParsedDetailsData::ParsedDetailsData(const DetailsEvent* event)
        {
            Json::Value data;
            Json::Reader reader;
            reader.parse(event->data, data, false);

            MarketType mt = getMarketType();

            for (Json::ArrayIndex i = 0; i < data.size(); ++i)
            {
                const Json::Value& item = data[i];

                Item it;

                if (mt == google || mt == simulator || mt == amazon)
                {
                    it.productId = getString(item, "productId");
                    it.description = getString(item, "description");
                    it.price = getString(item, "price");
                    it.price_amount_micros = getInt64(item, "price_amount_micros");
                    it.price_currency_code = getString(item, "price_currency_code");
                    it.title = getString(item, "title");
                    it.type = getString(item, "type");
                }

                items.push_back(it);
            }
        }

        namespace internal
        {
            void purchased(int requestCode, int resultCode, const std::string& data1, const std::string& data2, const std::string& data3)
            {
                log::messageln("billing::internal::purchased %d %d <%s> <%s> <%s>", requestCode, resultCode, data1.c_str(), data2.c_str(), data3.c_str());

                int event = PurchasedEvent::EVENT_SUCCESS;
                MarketType mt = getMarketType();
                if (mt == google || mt == simulator || mt == amazon || mt == ios)
                {
                    event = PurchasedEvent::EVENT_ERROR;
                    if (requestCode == ActivityOK)
                    {
                        switch (resultCode)
                        {
                            case RC_Canceled:
                                event = PurchasedEvent::EVENT_CANCELED;
                                break;
                            case RC_OK:
                                event = PurchasedEvent::EVENT_SUCCESS;
                                break;
                        }
                    }
                }

                PurchasedEvent ev(data1, data2, data3, event);
                if (_dispatcher)
                    _dispatcher->dispatchEvent(&ev);
            }

            void detailed(const std::string& str)
            {
                log::messageln("billing::internal::detailed %s", str.c_str());
                DetailsEvent ev(str);
                if (_dispatcher)
                    _dispatcher->dispatchEvent(&ev);
            }

            /*

            void detailed(const std::string& data_)
            {
                Json::Value data;

                Json::Reader reader;
                reader.parse(data_, data, false);

                DetailsEvent ev(data);
                _dispatcher->dispatchEvent(&ev);
            }
            */
        }

        ParsePurchasedData::ParsePurchasedData(const PurchasedEvent* event)
        {
            Json::Reader reader;
            reader.parse(event->data1, data, false);

            MarketType mt = getMarketType();

            if (mt == google || mt == simulator)
            {
                productID = data["productId"].asString();
                purchaseToken = data["purchaseToken"].asString();
                purchaseState = data["purchaseState"].asInt();
                payload = data["developerPayload"].asString();
            }

            if (mt == amazon)
            {
                purchaseToken = data["receiptId"].asString();
                productID = data["sku"].asString();
                payload = event->data3;
            }

            if (mt == ios)
            {
                productID = data["productIdentifier"].asString();
                iosTransactionReceipt = data["transactionReceipt"].asString();

                purchaseToken = data["transactionIdentifier"].asString();
                payload = event->data3;
            }
        }
    }
}
