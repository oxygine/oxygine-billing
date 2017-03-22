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
        spEventDispatcher _dispatcher;
        spEventDispatcher dispatcher()
        {
            return _dispatcher;
        }

        void init()
        {
            log::messageln("billing::init");
            OX_ASSERT(!_dispatcher);

            _dispatcher = new EventDispatcher;

#ifdef __ANDROID__
            jniBillingInit();
#elif IOS_STORE
            iosBillingInit();
#else
            billingSimulatorInit();
#endif
        }

        void free()
        {
            log::messageln("billing::free");

#ifdef __ANDROID__
            jniBillingFree();
#elif IOS_STORE
            iosBillingFree();
#else
            billingSimulatorFree();
#endif
            _dispatcher->removeAllEventListeners();
            _dispatcher = 0;
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

#ifdef __ANDROID__
            jniBillingPurchase(id, payload);
#elif IOS_STORE
            iosBillingPurchase(id);
#else
            billingSimulatorPurchase(id, payload);
#endif
        }

        void consume(const std::string& token)
        {
            log::messageln("billing::consume");

#ifdef __ANDROID__
            jniBillingConsume(token);
#elif IOS_STORE
            iosBillingConsume(token);
#else
            billingSimulatorConsume(token);
#endif
        }

        void requestPurchases()
        {
            log::messageln("billing::requestPurchases");

#ifdef __ANDROID__
            jniBillingGetPurchases();
#elif IOS_STORE
            iosBillingGetPurchases();
#else
            billingSimulatorGetPurchases();
#endif
        }

        void requestDetails(const std::vector<std::string>& items)
        {
            log::messageln("billing::requestDetails");

#ifdef __ANDROID__
            jniBillingUpdate(items);
#elif IOS_STORE
            iosBillingUpdate(items);
#else
            billingSimulatorRequestDetails(items);
#endif
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
            void purchased(int requestCode, int resultCode, const std::string& data_, const std::string& sign_)
            {
                log::messageln("billing::internal::purchased %d %d <%s> <%s>", requestCode, resultCode, data_.c_str(), sign_.c_str());

                int event = PurchasedEvent::EVENT;
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
                                event = PurchasedEvent::EVENT;
                                break;
                        }
                    }
                }
                
                PurchasedEvent ev(data_, sign_, event);
                _dispatcher->dispatchEvent(&ev);
            }

            void detailed(const std::string& str)
            {
                log::messageln("billing::internal::detailed %s", str.c_str());
                DetailsEvent ev(str);
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
            reader.parse(event->data, data, false);

            MarketType mt = getMarketType();

            if (mt == google || mt == simulator)
            {
                productID = data["productId"].asString();
                purchaseToken = data["purchaseToken"].asString();
                purchaseState = data["purchaseState"].asInt();
            }

            if (mt == amazon)
            {
                purchaseToken = data["receiptId"].asString();
                productID = data["sku"].asString();
            }

            if (mt == ios)
            {
                productID = data["productIdentifier"].asString();
                purchaseToken = data["transactionIdentifier"].asString();
            }
        }
    }
}
