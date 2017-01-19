package org.oxygine.HelloWorldBilling;

import org.oxygine.lib.OxygineActivity;
import org.oxygine.billing.BillingAmazon;
import org.oxygine.billing.BillingGoogle;
import android.os.Bundle;

public class MainActivity extends OxygineActivity
{
	private boolean google = true;

	@Override
    protected void onCreate(Bundle savedInstanceState) {

        addObserver(new BillingGoogle());
        //addObserver(new BillingAmazon());
        //addObserver(Billing::create(this));

        //!!!!!!!!!!!!!!!!!!! IT IS IMPORTANT !!!! super.onCreate should be AT THE END of method
        super.onCreate(savedInstanceState);
    }
}
