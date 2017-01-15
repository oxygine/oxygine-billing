#include "oxygine-framework.h"
#include <functional>
#include "test.h"
#include "billing.h"


using namespace oxygine;

string lastPurchasedItemToken;

vector<string> items = {"org.oxygine.test"};

class TestActor : public Test
{
public:
    
	TestActor()
    {
        instance = this;

        addButton("purchase", "Purchase Item");
        addButton("get_purchases", "Request Purchases");
        addButton("get_details", "Request Details");
        addButton("consume", "Consume");
	}


	void clicked(string id)
	{
        if (id == "purchase")
        {
            billing::purchase(items.front(), "123");
        }
        
        if (id == "get_purchases")
        {
            billing::requestPurchases();
        }

        if (id == "get_details")
        {
            billing::requestDetails(items);
        }

        if (id == "consume")
        {
            if (!lastPurchasedItemToken.empty())
                billing::consume(lastPurchasedItemToken);
        }
	}
};


void example_preinit() {}


//called from entry_point.cpp
void example_init()
{
	billing::init();

#if 1
    Json::Value itemsObject(Json::arrayValue);
    
    for (const auto &id:items)
    {
        Json::Value item(Json::objectValue);
        item["productId"] = id;
        item["price"] = "1 USD";

        itemsObject.append(item);
    }


    billing::simulatorSetDetails(itemsObject);
#endif

	billing::dispatcher()->addEventListener(billing::PurchasedEvent::EVENT, [](Event* e){
		
        //Test::instance->notify("purchased");

        billing::PurchasedEvent *ev = safeCast<billing::PurchasedEvent*>(e);

        billing::ParsePurchasedData parced(ev);
        lastPurchasedItemToken = parced.purchaseToken;
        Test::instance->updateText("consume", "Consume item: " + parced.productID + ":" + lastPurchasedItemToken);

        Test::instance->notify(ev->data, 10000, true);
	});

	billing::dispatcher()->addEventListener(billing::DetailsEvent::EVENT, [](Event* e){
		//Test::instance->notify("details");

        billing::DetailsEvent *ev = safeCast<billing::DetailsEvent*>(e);
        billing::ParsedDetailsData parced(ev);

        Test::instance->notify(ev->data, 10000, true);
	});

	Test::init();
	getStage()->addChild(new TestActor);
}


//called each frame from entry_point.cpp
void example_update()
{
}

//called each frame from entry_point.cpp
void example_destroy()
{
	Test::free();
}
