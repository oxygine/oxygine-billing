package org.oxygine.billing;

import android.content.pm.PackageManager;
import org.oxygine.lib.extension.ActivityObserver;
import android.app.Activity;

public abstract class Billing extends ActivityObserver {
    static final public int BILLING_REQUEST_CODE = 1001;

    public static native void nativeBillingDetails(String[] items);
    public static native void nativeBillingPurchases(int requestCode, int resultCode, String[] items, String[] signatures, String[] payloads);

    public static void nativeBillingStatus(int requestCode, int resultCode)
    {
        nativeBillingPurchases(requestCode, resultCode, null, null, null);
    }

    public static void nativeBillingPurchase(int requestCode, int resultCode, String item, String signature, String payloads)
    {
        String[] pr = null;
        if (item != null) {
            pr = new String[1];
            pr[0] = item;   
        }

        String[] sg = null;
        if (signature != null){
            sg = new String[1];
            sg[0] = signature;   
        }

        String[] pl = null;
        if (payloads != null){
            pl = new String[1];
            pl[0] = payloads;
        }

        nativeBillingPurchases(requestCode, resultCode, pr, sg, pl);
    }

    public static Billing create(Activity act)
    {
        PackageManager pm = act.getPackageManager();
        String installer = pm.getInstallerPackageName(act.getPackageName());

        if (installer == "com.amazon.venezia")
        	return new BillingAmazon();
        else        	
            return new BillingGoogle();
    }


    public abstract void getPurchases();

    public abstract void requestDetails(String[] ids);

    public abstract void purchase(String sku, String payload);

    public abstract void consume(String token);

    public abstract String getCurrency();
    public abstract String getName();
}
