/*
 * Copyright (c) 2014 Samsung Electronics. All Rights Reserved
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of
 * SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall
 * use it only in accordance with the terms of the license agreement
 * you entered into with SAMSUNG ELECTRONICS. SAMSUNG make no
 * representations or warranties about the suitability
 * of the software, either express or implied, including but not
 * limited to the implied warranties of merchantability, fitness fora particular purpose,
 * or non-infringement. SAMSUNG shall not be liable for any damages suffered by licensee
 * as a result of using, modifying or distributing this software or its derivatives.
 */
package com.samsung.partner.reaktor;

import com.android.uiautomator.core.UiObject;
import com.android.uiautomator.core.UiObjectNotFoundException;
import com.android.uiautomator.core.UiSelector;
import com.android.uiautomator.testrunner.UiAutomatorTestCase;

import java.io.IOException;

/* This uiautomator script is used to disable Samsung predictive keyboard, as there is no public
 * API to disable this feature. Used by buildbot and trybot to make sure benchmarks won't be
 * affected by "enable predictive keyboard" dialog during benchmark runs.
 *
 * More information how to use/modify/build this snippet can be found at
 * http://developer.android.com/tools/testing/testing_ui.html
 */
public class TextPredictorDisabler extends UiAutomatorTestCase {

    public void setUp() throws UiObjectNotFoundException, IOException {
       getUiDevice().pressHome();
       String packageName = "com.sec.android.inputmethod";
       String component = "/com.touchtype.personalizer.PersonalizerSettingsActivity";
       String cmd = "am start -n " + packageName + component;

        // start settings application
        Runtime.getRuntime().exec(cmd);

        UiObject settingsUi = new UiObject(new UiSelector().packageName(packageName));
        assertTrue("TextPredictor settings not started", settingsUi.waitForExists(5000));
    }

    @Override
    protected void tearDown() throws Exception {
        getUiDevice().pressHome();
    }

    public void testPredictor() throws UiObjectNotFoundException {
        disablePredictor();
    }

    private void disablePredictor() throws UiObjectNotFoundException {
        UiObject parent = new UiObject(new UiSelector()
                .resourceId("android:id/action_bar_container"));
        UiObject predictorToggle = parent.getChild(new UiSelector().className(android.widget.Switch.class));

        if (predictorToggle.isChecked()) {
            predictorToggle.click();
        }
    }
}
