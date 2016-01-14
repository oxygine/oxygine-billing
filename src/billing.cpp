#include "billing.h"

#ifdef __ANDROID__
#include "android/AndroidBilling.h"
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
            _dispatcher = new EventDispatcher;

#ifdef __ANDROID__
            jniBillingInit();
#else
            billingSimulatorInit();
#endif
        }

        void free()
        {
            log::messageln("billing::free");

#ifdef __ANDROID__
            jniBillingFree();
#endif
            _dispatcher = 0;
        }

        void purchase(const std::string& id, const std::string& payload)
        {
            log::messageln("billing::purchase %s", id.c_str());

#ifdef __ANDROID__
            jniBillingPurchase(id, payload);
#else
            billingSimulatorPurchase(id, payload);
#endif
        }

        void consume(const std::string& token)
        {
            log::messageln("billing::consume");

#ifdef __ANDROID__
            jniBillingConsume(token);
#else
            billingSimulatorConsume(token);
#endif
        }

        void requestPurchases()
        {
            log::messageln("billing::requestPurchases");

#ifdef __ANDROID__
            jniBillingGetPurchases();
#else
            billingSimulatorGetPurchases();
#endif
        }

        void requestDetails(const std::vector<std::string>& items)
        {
            log::messageln("billing::requestDetails");

#ifdef __ANDROID__
            jniBillingUpdate(items);
#else
            billingSimulatorRequestDetails(items);
#endif
        }

        void simulatorSetDetails(const Json::Value& details)
        {
#ifdef __ANDROID__

#else
            billingSimulatorSetDetails(details);
#endif
        }

        ParsedDetailsData::ParsedDetailsData(const DetailsEvent* event)
        {
            for (Json::ArrayIndex i = 0; i < event->data.size(); ++i)
            {
                const Json::Value& item = event->data[i];

                Item it;

#if 1//def __ANDROID__
                it.productId            = item["productId"].asCString();
                it.description          = item["description"].asCString();
                it.price                = item["price"].asCString();
                it.price_amount_micros  = item["price_amount_micros"].asInt64();
                it.price_currency_code  = item["price_currency_code"].asCString();
                it.title                = item["title"].asCString();
                it.type                 = item["type"].asCString();
#else

                it.productId = item["productId"].asCString();

#endif
                items.push_back(it);
            }
        }

        namespace internal
        {
            void purchased(const std::string& data_, const std::string& sign_)
            {
                Json::Value data;
                Json::Value sign;

                Json::Reader reader;
                reader.parse(data_, data, false);
                reader.parse(sign_, sign, false);

                PurchasedEvent ev(data, sign);
                _dispatcher->dispatchEvent(&ev);
            }

            void detailed(const Json::Value& data)
            {
                DetailsEvent ev(data);
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
    }
}