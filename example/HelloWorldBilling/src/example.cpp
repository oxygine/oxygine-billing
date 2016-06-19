#include "oxygine-framework.h"
#include <functional>
#include "test.h"
#include "billing.h"


using namespace oxygine;


class TestActor : public Test
{
public:

	TestActor()
	{
		addButton("purchase", "Purchase");
	
	}


	void clicked(string id)
	{
		billing::purchase("test", "123");
	}
};


void example_preinit() {}


//called from entry_point.cpp
void example_init()
{
	billing::init();
	billing::dispatcher()->addEventListener(billing::PurchasedEvent::EVENT, [](Event*){
		Test::instance->notify("purchased");
	});

	billing::dispatcher()->addEventListener(billing::DetailsEvent::EVENT, [](Event*){
		Test::instance->notify("details");
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
