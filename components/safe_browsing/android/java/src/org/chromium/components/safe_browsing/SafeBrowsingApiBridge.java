// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.safe_browsing;

import org.chromium.base.ContextUtils;
import org.chromium.base.CommandLine;
import org.chromium.base.Log;
import org.chromium.base.annotations.CalledByNative;
import org.chromium.base.annotations.JNINamespace;

/**
 * Helper for calling GMSCore Safe Browsing API from native code.
 */
@JNINamespace("safe_browsing")
public final class SafeBrowsingApiBridge {
    private static final String TAG = "ApiBridge";

    private static Class<? extends SafeBrowsingApiHandler> sHandler;
    // TODO(pritam.nikam): At present the implementation for sbrowser tab, custom tab and webapk are
    // fragmented. There is no common place for initialization of common data structures. Consider
    // removing this static block and instead migrate it on browser boot up.
    static {
        final String kEnableSafeBrowsing = "enable-safe-browsing";
        if (CommandLine.getInstance().hasSwitch(kEnableSafeBrowsing)) {
            final String safeBrowsingApiHandler =
                    "org.chromium.components.safe_browsing.SafeBrowsingApiHandlerImpl";

            // Try to get a specialized service bridge.
            try {
                Class<? extends SafeBrowsingApiHandler> cls =
                        (Class<? extends SafeBrowsingApiHandler>) Class.forName(
                                safeBrowsingApiHandler);
                SafeBrowsingApiBridge.setSafeBrowsingHandlerType(cls);
            } catch (ClassNotFoundException e) {
                // This is not an error; it just means this device doesn't have specialized
                // services.
            }
        }
    }

    private SafeBrowsingApiBridge() {
        // Util class, do not instantiate.
    }

    /**
     * Set the class-file for the implementation of SafeBrowsingApiHandler to use when the safe
     * browsing api is invoked.
     */
    public static void setSafeBrowsingHandlerType(Class<? extends SafeBrowsingApiHandler> handler) {
        sHandler = handler;
    }

    /**
     * Create a SafeBrowsingApiHandler obj and initialize its client, if supported.
     * Should be called on IO thread.
     *
     * @return the handler if it's usable, or null if the API is not supported.
     */
    @CalledByNative
    private static SafeBrowsingApiHandler create() {
        SafeBrowsingApiHandler handler;
        try {
            handler = sHandler.newInstance();
        } catch (NullPointerException | InstantiationException | IllegalAccessException e) {
            Log.e(TAG, "Failed to init handler: " + e.getMessage());
            return null;
        }
        boolean initSuccesssful = handler.init(
                ContextUtils.getApplicationContext(), new SafeBrowsingApiHandler.Observer() {
                    @Override
                    public void onUrlCheckDone(long callbackId, int resultStatus, String metadata) {
                        nativeOnUrlCheckDone(callbackId, resultStatus, metadata);
                    }
                });
        return initSuccesssful ? handler : null;
    }

    @CalledByNative
    private static void startUriLookup(
            SafeBrowsingApiHandler handler, long callbackId, String uri, int[] threatsOfInterest) {
        handler.startUriLookup(callbackId, uri, threatsOfInterest);
        Log.d(TAG, "Done starting request");
    }

    private static native void nativeOnUrlCheckDone(
            long callbackId, int resultStatus, String metadata);
}
