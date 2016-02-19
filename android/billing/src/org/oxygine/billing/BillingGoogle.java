package org.oxygine.billing;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.*;
import android.content.pm.ApplicationInfo;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.util.Base64;
import android.util.Log;
import com.android.vending.billing.IInAppBillingService;
//import com.google.android.gms.analytics.HitBuilders;
//import com.google.android.gms.analytics.Tracker;
import org.json.JSONException;
import org.json.JSONObject;

import java.security.*;
import java.security.spec.InvalidKeySpecException;
import java.security.spec.X509EncodedKeySpec;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Map;

/**
 * Created by Denis on 27.01.14.
 */
public class BillingGoogle extends Billing {
    private static final String TAG = "SDL";
    static String ORDER_ID = "";
    String _publicKey;
    Map<String, ItemData> prices = new HashMap<String, ItemData>();
    IInAppBillingService _service;
    ServiceConnection _serviceConn = new ServiceConnection() {
        @Override
        public void onServiceDisconnected(ComponentName name) {
            _service = null;
            
            Log.d(TAG, "billing.service disconnected");
            return;
                    
        }

        @Override
        public void onServiceConnected(ComponentName name, IBinder service) {
            _service = IInAppBillingService.Stub.asInterface(service);
            Log.d(TAG, "billing.service connected");
        }
    };

    public BillingGoogle(String publicKey) {
        _publicKey = publicKey;
    }

    @Override
    public String getCurrency() {
        if (!prices.isEmpty()) {
            String firstKey = prices.keySet().iterator().next();
            return prices.get(firstKey).curCode;
        }

        return "";
    }

    @Override
    public String getName()
    {
        return "google";
    }

    @Override
    public void onCreate() {
        Intent serviceIntent = new Intent("com.android.vending.billing.InAppBillingService.BIND");
        serviceIntent.setPackage("com.android.vending");
        _activity.bindService(serviceIntent, _serviceConn, Context.BIND_AUTO_CREATE);               
    }


    @Override
    public void onResume() {

    }

    @Override
    public void onDestroy() {
        if (_service != null) {
            _activity.unbindService(_serviceConn);
        }
    }

    public boolean verify(String data, String signature) {
        return true;
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == BILLING_REQUEST_CODE) {
            if (resultCode != Activity.RESULT_OK)
                return;

            int responseCode = data.getIntExtra("RESPONSE_CODE", 0);
            if (responseCode != 0)
                return;

            String purchaseData = data.getStringExtra("INAPP_PURCHASE_DATA");
            String dataSignature = data.getStringExtra("INAPP_DATA_SIGNATURE");
            if (verify(purchaseData, dataSignature)) {
                nativeBillingPurchase(purchaseData, dataSignature);
            }
        }
    }

    protected String processPaymentData(String data, String signature) {
        try {
            JSONObject paymentData = new JSONObject("{}");

            JSONObject object = new JSONObject(data);
            String sku = object.getString("productId");
            ItemData item = prices.get(sku);
            if (item != null) {
                object.put("price", item.price);
                object.put("currencyCode", item.curCode);
                object.put("micros", item.micros);
                paymentData.put("customData", object);
            }

            paymentData.put("data", data);
            paymentData.put("signature", signature);

            return paymentData.toString();
        } catch (JSONException e) {
            e.printStackTrace();
        }

        return null;
    }

    @Override
    public void getPurchases() {
        new Thread(new Runnable() {
            public void run() {
                try {
                    if (_service == null)
                        return;

                    Log.d(TAG, "billing.getPurchases");
                    Bundle purchases = _service.getPurchases(3, _activity.getPackageName(), "inapp", null);
                    int response = purchases.getInt("RESPONSE_CODE");
                    if (response == 0) {
                        ArrayList<String> details = purchases.getStringArrayList("INAPP_PURCHASE_DATA_LIST");
                        ArrayList<String> signatures = purchases.getStringArrayList("INAPP_DATA_SIGNATURE_LIST");

                        nativeBillingPurchases(
                                details.toArray(new String[details.size()]),
                                signatures.toArray(new String[signatures.size()]));
                    }
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        }).start();
    }

    @Override
    public void requestDetails(String[] ids) {

        final ArrayList<String> skuList = new ArrayList<String>();
        for (String id : ids) {
            skuList.add(id);
        }

        new Thread(new Runnable() {
            public void run() {
                try {
                    if (_service == null)
                    {
                        Log.d(TAG, "billing.requestDetails failed service is null");
                        return;
                    }

                    Log.d(TAG, "billing.requestDetails");

                    ArrayList<String> details = new ArrayList<String>();

                    for (int i = 0; i < skuList.size(); i += 20) {
                        ArrayList<String> items = new ArrayList<String>();
                        items.addAll(skuList.subList(i, Math.min(i + 20, skuList.size())));

                        Bundle querySkus = new Bundle();
                        querySkus.putStringArrayList("ITEM_ID_LIST", items);

                        Bundle skuDetails = _service.getSkuDetails(3, _activity.getPackageName(), "inapp", querySkus);
                        int response = skuDetails.getInt("RESPONSE_CODE");
                        if (response == 0) {
                            ArrayList<String> responseList = skuDetails.getStringArrayList("DETAILS_LIST");
                            details.addAll(responseList);

                            for (String thisResponse : responseList) {
                                try {
                                    JSONObject object = new JSONObject(thisResponse);
                                    String sku = object.getString("productId");

                                    //Log.i(TAG, "ecommerceTracker put: " + sku + " '" + String.valueOf(price_amount_micros)+"'" + " cur:"+price_currency_code);
                                    ItemData item = new ItemData();
                                    item.price = object.getString("price");
                                    item.curCode = object.getString("price_currency_code");
                                    item.micros = (float) (Double.valueOf(object.getString("price_amount_micros")) / 1000000);
                                    prices.put(sku, item);

                                } catch (JSONException e) {
                                    e.printStackTrace();
                                }
                            }
                        }
                    }

                    String[] ar = details.toArray(new String[details.size()]);
                    nativeBillingDetails(ar);
                } catch (RemoteException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
        }).start();
    }

    @Override
    public void consume(final String token) {
        new Thread(new Runnable() {
            public void run() {
                if (_service == null)
                    {
                        Log.d(TAG, "billing.consume failed service is null");
                        return;
                    }

                try {
                    Log.d(TAG, "billing.consume");
                    int response = _service.consumePurchase(3, _activity.getPackageName(), token);
                    if (response == 0) {
                    }
                } catch (RemoteException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                int i = 0;
            }
        }).start();
    }

    @Override
    public void purchase(final String sku, final String payload) {
        _activity.runOnUiThread(new Runnable() {
            public void run() {
                if (_service == null)
                    {
                        Log.d(TAG, "billing.purchase failed service is null");
                        return;
                    }

                try {
                    Log.d(TAG, "billing.purchase");
                    //item = "android.test.purchased";
                    Bundle buyIntentBundle = _service.getBuyIntent(3, _activity.getPackageName(), sku, "inapp", payload);
                    PendingIntent pendingIntent = buyIntentBundle.getParcelable("BUY_INTENT");

                    _activity.startIntentSenderForResult(pendingIntent.getIntentSender(),
                            BILLING_REQUEST_CODE, new Intent(), Integer.valueOf(0), Integer.valueOf(0), Integer.valueOf(0));
                } catch (RemoteException e) {
                    e.printStackTrace();
                } catch (IntentSender.SendIntentException e) {
                    e.printStackTrace();
                } catch (NullPointerException e) {
                    e.printStackTrace();
                }
            }
        });
    }

    class ItemData {
        public String price;
        public float micros;
        public String curCode;
    }
}