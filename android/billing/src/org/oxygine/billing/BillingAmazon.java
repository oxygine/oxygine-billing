package org.oxygine.billing;


import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.util.Log;
import com.amazon.device.iap.PurchasingListener;
import com.amazon.device.iap.PurchasingService;
import com.amazon.device.iap.model.*;
import org.json.*;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

class Payloads
{
    public static SharedPreferences data;
}

class MyPurchasingListener implements PurchasingListener
{
    private static final String TAG = "MyPurchasingListener";

    private String currentUserId = null;
    private String currentMarketplace = null;



    public void handleReceipt(Receipt receipt, RequestId rq)
    {
        /*
        {
            "sku": "com.abc.qwe",
            "purchaseDate": "Wed Dec 17 20:07:12 GMT+06:00 2014",
            "itemType": "CONSUMABLE",
            "receiptId": "q1YqVrJSSs7P1UsrLSqq1MstKSxNLS5JzE2sys_TS8lMzM3PSzEyyFXSUUoBKjQ0MbSwMDI1MjayMLAEipUCxXKMPXzMIwoKXH0zivzdXfJKiyx1S3PKCvNzi4LTzQor_V28cxMzzM1yvEJtgVpKlKwMagE"
        }
        */

        String payload = Payloads.data.getString(rq.toString(), "");
        if (payload.isEmpty()) {
            //try to restore payload by Sku
            payload = Payloads.data.getString(receipt.getSku(), "");
        }

        Billing.nativeBillingPurchase(Activity.RESULT_OK, 0, receipt.toString(), currentUserId, payload);
    }

    @Override
    public void onPurchaseResponse(PurchaseResponse response)
    {
        final String requestId = response.getRequestId().toString();
        final String userId = response.getUserData().getUserId();
        final PurchaseResponse.RequestStatus status = response.getRequestStatus();
        Log.d(TAG, "onPurchaseResponse: requestId (" + requestId
                + ") userId ("
                + userId
                + ") purchaseRequestStatus ("
                + status
                + ")");

        switch (status) {
            case SUCCESSFUL:
                final Receipt receipt = response.getReceipt();
                //iapManager.setAmazonUserId(response.getUserData().getUserId(), response.getUserData().getMarketplace());
                Log.d(TAG, "onPurchaseResponse: receipt json:" + receipt.toJSON());
                handleReceipt(receipt, response.getRequestId());
                break;
            case ALREADY_PURCHASED:
                Log.d(TAG, "onPurchaseResponse: already purchased, should never get here for a consumable.");
                // This is not applicable for consumable item. It is only
                // application for entitlement and subscription.
                // check related samples for more details.
                Billing.nativeBillingStatus(Activity.RESULT_OK, 7);
                break;
            case INVALID_SKU:
                Log.d(TAG, "onPurchaseResponse: invalid SKU!  onProductDataResponse should have disabled buy button already.");
                Billing.nativeBillingStatus(Activity.RESULT_OK, 4);
                break;
            case FAILED:
                Billing.nativeBillingStatus(Activity.RESULT_OK, 1);
                break;
            case NOT_SUPPORTED:
                Log.d(TAG, "onPurchaseResponse: failed so remove purchase request from local storage");
                Billing.nativeBillingStatus(Activity.RESULT_OK, 2);
                break;
        }
    }

    @Override
    public void onUserDataResponse(final UserDataResponse response)
    {
        final UserDataResponse.RequestStatus status = response.getRequestStatus();

        switch(status) {
            case SUCCESSFUL:
                currentUserId = response.getUserData().getUserId();
                currentMarketplace = response.getUserData().getMarketplace();
                break;

            case FAILED:
            case NOT_SUPPORTED:
                // Fail gracefully.
                break;
        }
    }


    @Override
    public void onProductDataResponse(final ProductDataResponse response)
    {
        final ProductDataResponse.RequestStatus status = response.getRequestStatus();
        Log.d(TAG, "onProductDataResponse: RequestStatus (" + status + ")");

        switch (status) {
            case SUCCESSFUL:
                Log.d(TAG, "onProductDataResponse: successful.  The item data map in this response includes the valid SKUs");
                final Set<String> unavailableSkus = response.getUnavailableSkus();
                Log.d(TAG, "onProductDataResponse: " + unavailableSkus.size() + " unavailable skus");
                //iapManager.enablePurchaseForSkus(response.getProductData());
                //iapManager.disablePurchaseForSkus(response.getUnavailableSkus());


                Map<String, Product> items = response.getProductData();
                ArrayList<String> ar = new ArrayList<String>();
                for (Product item:items.values())
                {
                    JSONObject obj = new JSONObject();
                    try
                    {
                        obj.put("productId", item.getSku());
                        obj.put("price", item.getPrice());

                    } catch (JSONException exc)
                    {

                    }

                    ar.add(obj.toString());
                }

                String[] data = ar.toArray(new String[ar.size()]);
                Billing.nativeBillingDetails(data);

                break;
            case FAILED:
            case NOT_SUPPORTED:
                Log.d(TAG, "onProductDataResponse: failed, should retry request");
                //iapManager.disableAllPurchases();
                break;
        }
    }


    @Override
    public void onPurchaseUpdatesResponse(final PurchaseUpdatesResponse response)
    {
        Log.d(TAG, "onPurchaseUpdatesResponse: requestId (" + response.getRequestId()
                + ") purchaseUpdatesResponseStatus ("
                + response.getRequestStatus()
                + ") userId ("
                + response.getUserData().getUserId()
                + ")");
        final PurchaseUpdatesResponse.RequestStatus status = response.getRequestStatus();
        switch (status) {
            case SUCCESSFUL:
                //iapManager.setAmazonUserId(response.getUserData().getUserId(), response.getUserData().getMarketplace());
                for (final Receipt receipt : response.getReceipts()) {
                    handleReceipt(receipt, response.getRequestId());
                }
                if (response.hasMore()) {
                    PurchasingService.getPurchaseUpdates(false);
                }
                //iapManager.refreshOranges();
                break;
            case FAILED:
            case NOT_SUPPORTED:
                Log.d(TAG, "onProductDataResponse: failed, should retry request");
                //iapManager.disableAllPurchases();
                break;
        }

    }
}

public class BillingAmazon extends Billing
{
    private static final String TAG = "BillingAmazon";

    MyPurchasingListener _listener;

    public BillingAmazon()
    {

    }

    @Override
    public String getCurrency()
    {
        return "USD";
    }

    @Override
    public String getName()
    {
        return "amazon";
    }

    @Override
    public void onCreate()
    {
        Payloads.data = _activity.getSharedPreferences("OxAmazonPayloads", Context.MODE_PRIVATE);

        _listener = new MyPurchasingListener();
        PurchasingService.registerListener(_activity.getApplicationContext(), _listener);

        Log.i(TAG, "onCreate: sandbox mode is:" + PurchasingService.IS_SANDBOX_MODE);
    }

    @Override
    public void onResume()
    {
        PurchasingService.getUserData();
        PurchasingService.getPurchaseUpdates(false);
    }

    @Override
    public void onDestroy()
    {

    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data)
    {

    }

    @Override
    public void getPurchases()
    {
        PurchasingService.getPurchaseUpdates(true);
    }

    @Override
    public void requestDetails(final String[] ids)
    {
        new Thread(new Runnable()
        {
            public void run()
            {
                Set<String> st = new HashSet<String>();
                for (String s:ids)
                {
                    st.add(s);
                }

                PurchasingService.getProductData(st);
            }
        }).start();
    }

    @Override
    public void purchase(String sku, String payload)
    {
        RequestId r = PurchasingService.purchase(sku);

        Log.i(TAG, "amazon purchase :" + r.toString() + " " + payload);

        SharedPreferences.Editor editor = Payloads.data.edit();
        editor.putString(r.toString(), payload);
        editor.putString(sku, payload);//in case of crash
        editor.commit();
    }

    @Override
    public void consume(String token)
    {
        PurchasingService.notifyFulfillment(token, FulfillmentResult.FULFILLED);
    }
}
