package org.oxygine.HelloWorldBilling;

import org.oxygine.lib.OxygineActivity;
import org.oxygine.billing.BillingAmazon;
import org.oxygine.billing.BillingGoogle;
import android.content.pm.PackageManager;
import android.os.Bundle;

public class MainActivity extends OxygineActivity
{
	private boolean google = true;

	@Override
    protected void onCreate(Bundle savedInstanceState) {
        PackageManager pm = getPackageManager();
        String installer = pm.getInstallerPackageName(getPackageName());
        if (installer == "com.amazon.venezia")
            google = false;

        //google = false;

        if (google)
            addObserver(new BillingGoogle("MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAos3ihZdP/Fqw+kaWgHl+2Yrk9Y+gswF+pGY/ePQLk26iFUE4XWb/XcWq+dDWu8fxl6hqZd8RWHwc5tIo6ewlnZP4QVmAU03uTVzt7jTJl7OX+JADVB4e4tPrFEoKdvG/TtmANifqSfFoXh5SvL64E+uyvTDzbxTynzLfyUW7MCZPfn/nneO9A2xmJM/icu75HSvuUviwcMhtXFV6lOdYoYJG2VUMK8cicJ7T1w4MDrfWwpybUEjya4WTIbO4rqcBkwYDisOGpt5hnu1NI7TGEhh7QlzSbZlS25+hOBRCikfyUAOCkWI8WSZzP204FUjJRGNzUnVfQbCvlBTZdMAMKQIDAQAB"));
        else
            addObserver(new BillingAmazon());

        //!!!!!!!!!!!!!!!!!!! IT IS IMPORTANT !!!! super.onCreate should be AT THE END of method
        super.onCreate(savedInstanceState);
    }
}
