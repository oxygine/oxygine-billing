#include "BillingSimulator.h"
#include "Actor.h"
#include "ColorRectSprite.h"
#include "TextField.h"
#include "Stage.h"
#include "Box9Sprite.h"
#include "DebugActor.h"
#include "res/Resources.h"
#include "utils/stringUtils.h"
#include "billing.h"
#include "core/file.h"
#include "core/oxygine.h"

using namespace oxygine;

DECLARE_SMART(Btn, spBtn);
class Btn : public Box9Sprite
{
public:
    Btn()
    {
        _txt = new TextField;
        _txt->setText("OK");
        _txt->setAlign(TextStyle::VALIGN_MIDDLE, TextStyle::HALIGN_MIDDLE);
        addChild(_txt);

        setColor(Color::Green);
        setResAnim(DebugActor::resSystem->getResAnim("btn"));

        addEventListener(TouchEvent::OVER, CLOSURE(this, &Btn::touch));
        addEventListener(TouchEvent::OUT, CLOSURE(this, &Btn::touch));
    }

    void setText(const string& txt)
    {
        _txt->setText(txt);
    }

    void touch(Event* ev)
    {
        if (ev->type == TouchEvent::OVER)
            setColor(Color::GreenYellow);
        if (ev->type == TouchEvent::OUT)
            setColor(Color::Green);
    }

    void sizeChanged(const Vector2& size)
    {
        _txt->setSize(size);
    }

    spTextField _txt;
};

DECLARE_SMART(BillingDialog, spBillingDialog);
class BillingDialog : public Actor
{
public:
    enum
    {
        EVENT_OK = 12323,
        EVENT_CANCEL
    };

    BillingDialog()
    {
        setPriority(9999);

        spActor blocker = new Actor;
        blocker->setPosition(-getStage()->getSize());
        blocker->setSize(getStage()->getSize() * 3);
        addChild(blocker);

        _bg = new Box9Sprite;
        addChild(_bg);

        _title = new TextField;
        _title->setAlign(TextStyle::VALIGN_MIDDLE, TextStyle::HALIGN_MIDDLE);
        _title->setMultiline(true);
        _title->setColor(Color::Black);
        addChild(_title);

        _btnOk = new Btn();
        _btnOk->setSize(70, 30);
        _btnOk->setText("Ok");
        addChild(_btnOk);

        _btnCancel = new Btn();
        _btnCancel->setSize(70, 30);
        _btnCancel->setText("Cancel");
        addChild(_btnCancel);
    }

    void setTitle(const string& txt)
    {
        _title->setText(txt);
    }

    void doRender(const RenderState& rs)
    {
        //Stage::render()
    }

    void sizeChanged(const Vector2& size)
    {
        _bg->setSize(size);

        Vector2 center = core::getDisplaySize().cast<Vector2>() / 2.0f;
        center = getStage()->parent2local(center);

        float sx = getStage()->getScaleX();
        setPosition(center - size / sx / 2);

        _btnOk->setPosition(size - _btnOk->getSize() - Vector2(10, 10));
        _btnCancel->setPosition(10, getHeight() - 10 - _btnCancel->getHeight());
        _title->setWidth(getWidth());
        _title->setHeight(_btnCancel->getY());
    }

    spBox9Sprite        _bg;
    spTextField         _title;
    spBtn               _btnOk;
    spBtn               _btnCancel;
};


Json::Value _purchases(Json::arrayValue);
Json::Value _details(Json::arrayValue);

void billingSimulatorInit()
{
    DebugActor::initialize();

    file::buffer bf;
    file::read(".billing", bf, ep_ignore_error);

    if (!bf.empty())
    {
        Json::Reader reader;
        reader.parse((char*)&bf.front(), (char*)&bf.front() + bf.size(), _purchases, false);
    }
}

void save()
{
    Json::FastWriter writer;
    string s = writer.write(_purchases);
    file::write(".billing", s.c_str(), s.size());
}

void billingSimulatorPurchase(const string& id, const string& payload)
{
    spBillingDialog d = new BillingDialog;
    d->setScale(1.0f / getStage()->getScaleX());
    d->setSize(200, 130);
    getStage()->addChild(d);

    bool alreadyPurchased = false;
    for (Json::ArrayIndex i = 0; i < _purchases.size(); ++i)
    {
        const Json::Value& item = _purchases[i];
        if (item["data"]["productId"] == id)
        {
            alreadyPurchased = true;
            break;
        }
    }

    char str[255];
    if (alreadyPurchased)
        safe_sprintf(str, "Item '%s' was already purchased", id.c_str());
    else
        safe_sprintf(str, "Purchase Item '%s'?", id.c_str());
    d->setTitle(str);


    if (alreadyPurchased)
    {
        d->_btnCancel->setVisible(false);
        d->_btnOk->addEventListener(TouchEvent::CLICK, [ = ](Event*)
        {
            d->detach();
        });
    }
    else
    {
        d->_btnOk->addEventListener(TouchEvent::CLICK, [ = ](Event*)
        {
            d->detach();
            getStage()->addTween(TweenDummy(), rand() % 1000 + 500)->setDoneCallback([ = ](Event*)
            {

                Json::Value data(Json::objectValue);
                data["productId"] = id;
                data["purchaseState"] = 0;
                char str[255];
                safe_sprintf(str, "%lld", getTimeUTCMS());
                data["purchaseToken"] = str;
                data["developerPayload"] = payload;

                Json::Value item(Json::objectValue);
                item["data"] = data;
                item["sign"] = "qwe";

                _purchases.append(item);
                save();

                Json::FastWriter writer;
                billing::internal::purchased(writer.write(data), item["sign"].asCString());
            });
        });

        d->_btnCancel->addEventListener(TouchEvent::CLICK, [ = ](Event*)
        {
            d->detach();
        });
    }
}

void billingSimulatorConsume(const string& token)
{
    for (Json::ArrayIndex i = 0; i < _purchases.size(); ++i)
    {
        string s = _purchases[i]["data"]["purchaseToken"].asString();
        if (s == token)
        {
            Json::Value v;
            _purchases.removeIndex(i, &v);
            break;
        }
    }

    save();
}

void billingSimulatorGetPurchases()
{
    getStage()->addTween(TweenDummy(), rand() % 1000 + 500)->setDoneCallback([ = ](Event*)
    {
        Json::Value copy = _purchases;
        for (Json::ArrayIndex i = 0; i < copy.size(); ++i)
        {
            const Json::Value& item = copy[i];

            Json::FastWriter writer;
            billing::internal::purchased(writer.write(item["data"]), item["sign"].asCString());
        }
    });
}

void billingSimulatorRequestDetails(const vector<string>& items)
{
    getStage()->addTween(TweenDummy(), rand() % 1000 + 500)->setDoneCallback([ = ](Event*)
    {
        Json::Value data(Json::arrayValue);
        for (size_t i = 0; i < items.size(); ++i)
        {
            for (size_t n = 0; n < _details.size(); ++n)
            {
                if (_details[n]["productId"].asString() == items[i])
                    data.append(_details[i]);
            }
        }
        Json::FastWriter writer;
        billing::internal::detailed(writer.write(data));
    });
}

void billingSimulatorSetDetails(const Json::Value& details)
{
    _details.clear();

    for (Json::ArrayIndex i = 0; i < details.size(); ++i)
    {
        const Json::Value& item = details[i];

        _details.append(item);
    }
}