#include <jni.h>
#include <android/log.h>
#include <assert.h>
#include "ox/oxygine.hpp"
#include "ox/Object.hpp"
#include "ox/ThreadDispatcher.hpp"
#include "ox/oxygine.hpp"
#include "AndroidBilling.h"
#include "oxygine/core/android/jniHelper.h"
#include "oxygine/core/android/jniUtils.h"
#include "billing.h"

#include "ox/json.hpp"

using namespace oxygine;

void billingDetails(JNIEnv* env, jclass cl, jobjectArray array);
void billingPurchases(JNIEnv* env, jclass cl, jobjectArray array);


jclass _jBillingClass = 0;
jobject _jBillingObject = 0;

bool isBillinEnabled()
{
    return _jBillingClass && _jBillingObject;
}

extern "C"
{
    JNIEnv* Android_JNI_GetEnv(void);

    JNIEXPORT void JNICALL Java_org_oxygine_billing_Billing_nativeBillingDetails(JNIEnv* env, jclass cl, jobjectArray jItems)
    {
        vector<string> items;
        jniGetStringArray(items, env, jItems);

        Json::Value vls(Json::arrayValue);

        Json::Reader reader;


        for (const auto& item : items)
        {
            Json::Value v;
            reader.parse(item, v, false);
            vls.append(v);
        }


        Json::FastWriter writer;
        std::string str = writer.write(vls);

        core::getMainThreadDispatcher().postCallback([ = ]()
        {
            billing::internal::detailed(str);
        });
    }

    JNIEXPORT void JNICALL Java_org_oxygine_billing_Billing_nativeBillingPurchases(JNIEnv* env, jclass cl, jint requestCode, jint resultCode, jobjectArray jItems, jobjectArray jSignatures, jobjectArray jPayloads)
    {
        log::messageln("Java_org_oxygine_billing_Billing_nativeBillingPurchases2");
        vector<string> items;
        jniGetStringArray(items, env, jItems);

        vector<string> signatures;
        jniGetStringArray(signatures, env, jSignatures);

        vector<string> payloads;
        jniGetStringArray(payloads, env, jPayloads);

        OX_ASSERT(signatures.size() == items.size());
        OX_ASSERT(signatures.size() == payloads.size());

        core::getMainThreadDispatcher().postCallback([ = ]()
        {
            if (items.empty())
                billing::internal::purchased(requestCode, resultCode, "", "", "");
            else
            {
                for (size_t i = 0; i < items.size(); ++i)
                {

                    billing::internal::purchased(requestCode, resultCode, items[i], signatures[i], payloads[i]);
                }
            }
        });
    }

}

void jniBillingInit()
{
    try
    {
        JNIEnv* env = jniGetEnv();
        LOCAL_REF_HOLDER(env);

        _jBillingClass = (jclass) env->NewGlobalRef(env->FindClass("org/oxygine/billing/Billing"));
        JNI_NOT_NULL(_jBillingClass);

        _jBillingObject = env->NewGlobalRef(jniFindExtension(env, _jBillingClass));
        JNI_NOT_NULL(_jBillingObject);
    }
    catch (const notFound&)
    {
        log::error("jniBillingInit failed, class/member not found");
    }

    log::messageln("jniBillingType %s", jniBillingGetType().c_str());
}

void jniBillingFree()
{
    if (!isBillinEnabled())
        return;

    try
    {
        JNIEnv* env = jniGetEnv();
        LOCAL_REF_HOLDER(env);

        env->DeleteGlobalRef(_jBillingClass);
        _jBillingClass = 0;

        env->DeleteGlobalRef(_jBillingObject);
        _jBillingObject = 0;
    }
    catch (const notFound&)
    {

    }
}

void jniBillingUpdate(const vector<string>& ids)
{
    if (!isBillinEnabled())
        return;

    try
    {
        JNIEnv* env = jniGetEnv();
        LOCAL_REF_HOLDER(env);

        jclass jclassString = env->FindClass("java/lang/String");
        JNI_NOT_NULL(jclassString);


        jobjectArray array = env->NewObjectArray(ids.size(), jclassString, 0);
        for (size_t i = 0; i < ids.size(); ++i)
        {
            jstring jstr = env->NewStringUTF(ids[i].c_str());
            env->SetObjectArrayElement(array, i, jstr);
        }

        jmethodID requestBillingDetails = env->GetMethodID(_jBillingClass, "requestDetails", "([Ljava/lang/String;)V");
        JNI_NOT_NULL(requestBillingDetails);

        env->CallVoidMethod(_jBillingObject, requestBillingDetails, array);

        __android_log_print(ANDROID_LOG_DEBUG, "SDL", "initBillingDetails end2");
    }
    catch (const notFound&)
    {

    }
}

std::string jniBillingGetType()
{
    if (!isBillinEnabled())
        return "";

    JNIEnv* env = jniGetEnv();

    LOCAL_REF_HOLDER(env);

    jmethodID jfunc = env->GetMethodID(_jBillingClass, "getName", "()Ljava/lang/String;");
    jstring jstr = (jstring)env->CallObjectMethod(_jBillingObject, jfunc);

    return jniGetString(env, jstr);
}

void jniBillingConsume(const string& token)
{
    if (!isBillinEnabled())
        return;

    try
    {
        JNIEnv* env = jniGetEnv();
        LOCAL_REF_HOLDER(env);

        jmethodID jbillingConsume = env->GetMethodID(_jBillingClass, "consume", "(Ljava/lang/String;)V");
        JNI_NOT_NULL(jbillingConsume);

        jstring jstr = env->NewStringUTF(token.c_str());

        env->CallVoidMethod(_jBillingObject, jbillingConsume, jstr);

    }
    catch (const notFound&)
    {

    }
}


string jniBillingGetCurrency()
{
    if (!isBillinEnabled())
        return "";

    try
    {
        JNIEnv* env = jniGetEnv();
        LOCAL_REF_HOLDER(env);

        jmethodID jbillingGetCurrency = env->GetMethodID(_jBillingClass, "getCurrency", "()Ljava/lang/String;");
        JNI_NOT_NULL(jbillingGetCurrency);

        jobject obj = env->CallObjectMethod(_jBillingObject, jbillingGetCurrency);
        jstring my_java_string = (jstring)obj;

        const char* my_c_string = env->GetStringUTFChars(my_java_string, NULL);
        string cs = "";
        if (my_c_string != NULL)
        {
            cs = my_c_string;
        }
        env->ReleaseStringUTFChars(my_java_string, my_c_string);

        return cs;

    }
    catch (const notFound&)
    {

    }

    return "";
}

void _jniGetPurchases(JNIEnv* env)
{
    if (!_jBillingClass)
        return;

    try
    {
        LOCAL_REF_HOLDER(env);

        jmethodID jgetPurchases = env->GetMethodID(_jBillingClass, "getPurchases", "()V");
        JNI_NOT_NULL(jgetPurchases);

        env->CallVoidMethod(_jBillingObject, jgetPurchases);
    }
    catch (const notFound&)
    {

    }
}

void jniBillingGetPurchases()
{
    JNIEnv* env = jniGetEnv();
    _jniGetPurchases(env);
}


void jniBillingPurchase(const string& sku, const string& payload)
{
    try
    {
        JNIEnv* env = jniGetEnv();
        LOCAL_REF_HOLDER(env);

        jmethodID jpurchase = env->GetMethodID(_jBillingClass, "purchase", "(Ljava/lang/String;Ljava/lang/String;)V");
        JNI_NOT_NULL(jpurchase);

        jstring jsku = env->NewStringUTF(sku.c_str());
        jstring jpayload = env->NewStringUTF(payload.c_str());

        env->CallVoidMethod(_jBillingObject, jpurchase, jsku, jpayload);

    }
    catch (const notFound&)
    {

    }
}