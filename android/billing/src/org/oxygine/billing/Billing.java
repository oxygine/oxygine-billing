package org.oxygine.billing;

import org.oxygine.lib.extension.ActivityObserver;

public abstract class Billing extends ActivityObserver {
    static final public int BILLING_REQUEST_CODE = 1001;

    public static native void nativeBillingPurchased(int responseCode, String data);

    public static native void nativeBillingDetails(String[] items);

    public static native void nativeBillingPurchases(String[] items, String[] signatures);
    public static void nativeBillingPurchase(String item, String signature)
    {
        String[] pr = new String[1];
        pr[0] = item;

        String[] sg = new String[1];
        sg[0] = signature;
        nativeBillingPurchases(pr, sg);
    }


    public abstract void getPurchases();

    public abstract void requestDetails(String[] ids);

    public abstract void purchase(String sku, String payload);

    public abstract void consume(String token);

    public abstract String getCurrency();
    public abstract String getName();
}
