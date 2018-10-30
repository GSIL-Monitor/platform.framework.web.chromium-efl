// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.gcm_driver;

import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;

import org.chromium.base.ContextUtils;
import org.chromium.base.PackageUtils;

import java.io.IOException;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

import javax.annotation.Nullable;

/**
 * Temporary code for sending subtypes when (un)subscribing with GCM.
 * Subtypes are experimental and may change without notice!
 * TODO(johnme): Remove this file, once we switch to the GMS client library.
 */
public class GoogleCloudMessagingV2 implements GoogleCloudMessagingSubscriber {
    private static final String GOOGLE_PLAY_SERVICES_PACKAGE = "com.google.android.gms";
    private static final long REGISTER_TIMEOUT = 5000;
    private static final String ACTION_C2DM_REGISTER = "com.google.android.c2dm.intent.REGISTER";
    private static final String C2DM_EXTRA_ERROR = "error";
    private static final String INTENT_PARAM_APP = "app";
    private static final String ERROR_MAIN_THREAD = "MAIN_THREAD";
    private static final String ERROR_SERVICE_NOT_AVAILABLE = "SERVICE_NOT_AVAILABLE";
    private static final String EXTRA_DELETE = "delete";
    private static final String EXTRA_REGISTRATION_ID = "registration_id";
    private static final String EXTRA_SENDER = "sender";
    private static final String EXTRA_MESSENGER = "google.messenger";
    private static final String EXTRA_SUBTYPE = "subtype";
    private static final String EXTRA_SUBSCRIPTION = "subscription";

    // SAMSUNG : Security Guide for China
    private static final String GOOGLE_SERVICES_FRAMEWORK = "com.google.android.gsf";
    private static final Signature SIGNATURES[] = new android.content.pm.Signature[] {
            new android.content.pm.Signature("308204433082032ba003020102020900c2e08746644a308d300d"
                    + "06092a864886f70d01010405003074310b30090603550406130255533113301106035504081"
                    + "30a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773114"
                    + "3012060355040a130b476f6f676c6520496e632e3110300e060355040b1307416e64726f696"
                    + "43110300e06035504031307416e64726f6964301e170d3038303832313233313333345a170d"
                    + "3336303130373233313333345a3074310b30090603550406130255533113301106035504081"
                    + "30a43616c69666f726e6961311630140603550407130d4d6f756e7461696e20566965773114"
                    + "3012060355040a130b476f6f676c6520496e632e3110300e060355040b1307416e64726f696"
                    + "43110300e06035504031307416e64726f696430820120300d06092a864886f70d0101010500"
                    + "0382010d00308201080282010100ab562e00d83ba208ae0a966f124e29da11f2ab56d08f58e"
                    + "2cca91303e9b754d372f640a71b1dcb130967624e4656a7776a92193db2e5bfb724a91e7718"
                    + "8b0e6a47a43b33d9609b77183145ccdf7b2e586674c9e1565b1f4c6a5955bff251a63dabf9c"
                    + "55c27222252e875e4f8154a645f897168c0b1bfc612eabf785769bb34aa7984dc7e2ea2764c"
                    + "ae8307d8c17154d7ee5f64a51a44a602c249054157dc02cd5f5c0e55fbef8519fbe327f0b15"
                    + "11692c5a06f19d18385f5c4dbc2d6b93f68cc2979c70e18ab93866b3bd5db8999552a0e3b4c"
                    + "99df58fb918bedc182ba35e003c1b4b10dd244a8ee24fffd333872ab5221985edab0fc0d0b1"
                    + "45b6aa192858e79020103a381d93081d6301d0603551d0e04160414c77d8cc2211756259a7f"
                    + "d382df6be398e4d786a53081a60603551d2304819e30819b8014c77d8cc2211756259a7fd38"
                    + "2df6be398e4d786a5a178a4763074310b300906035504061302555331133011060355040813"
                    + "0a43616c69666f726e6961311630140603550407130d4d6f756e7461696e205669657731143"
                    + "012060355040a130b476f6f676c6520496e632e3110300e060355040b1307416e64726f6964"
                    + "3110300e06035504031307416e64726f6964820900c2e08746644a308d300c0603551d13040"
                    + "530030101ff300d06092a864886f70d010104050003820101006dd252ceef85302c360aaace"
                    + "939bcff2cca904bb5d7a1661f8ae46b2994204d0ff4a68c7ed1a531ec4595a623ce60763b16"
                    + "7297a7ae35712c407f208f0cb109429124d7b106219c084ca3eb3f9ad5fb871ef92269a8be2"
                    + "8bf16d44c8d9a08e6cb2f005bb3fe2cb96447e868e731076ad45b33f6009ea19c161e62641a"
                    + "a99271dfd5228c5c587875ddb7f452758d661f6cc0cccb7352e424cc4365c523532f7325137"
                    + "593c4ae341f4db41edda0d0b1071a7c440f0fe9ea01cb627ca674369d084bd2fd911ff06cdb"
                    + "f2cfa10dc0f893ae35762919048c7efc64c7144178342f70581c9de573af55b390dd7fdb941"
                    + "8631895d5f759f30112687ff621410c069308a")};

    private PendingIntent mAppPendingIntent;
    private final Object mAppPendingIntentLock = new Object();

    public GoogleCloudMessagingV2() {}

    @Override
    public String subscribe(String source, String subtype, @Nullable Bundle data)
            throws IOException {
        if (data == null) {
            data = new Bundle();
        }
        data.putString(EXTRA_SUBTYPE, subtype);
        Bundle result = subscribe(source, data);
        return result.getString(EXTRA_REGISTRATION_ID);
    }

    @Override
    public void unsubscribe(String source, String subtype, @Nullable Bundle data)
            throws IOException {
        if (data == null) {
            data = new Bundle();
        }
        data.putString(EXTRA_SUBTYPE, subtype);
        unsubscribe(source, data);
        return;
    }

    /**
     * Subscribe to receive GCM messages from a specific source.
     * <p>
     * Source Types:
     * <ul>
     * <li>Sender ID - if you have multiple senders you can call this method
     * for each additional sender. Each sender can use the corresponding
     * {@link #REGISTRATION_ID} returned in the bundle to send messages
     * from the server.</li>
     * <li>Cloud Pub/Sub topic - You can subscribe to a topic and receive
     * notifications from the owner of that topic, when something changes.
     * For more information see
     * <a href="https://cloud.google.com/pubsub">Cloud Pub/Sub</a>.</li>
     * </ul>
     * This function is blocking and should not be called on the main thread.
     *
     * @param source of the desired notifications.
     * @param data (optional) additional information.
     * @return Bundle containing subscription information including {@link #REGISTRATION_ID}
     * @throws IOException if the request fails.
     */
    public Bundle subscribe(String source, Bundle data) throws IOException {
        if (data == null) {
            data = new Bundle();
        }
        // Expected by older versions of GMS and servlet
        data.putString(EXTRA_SENDER, source);
        // New name of the sender parameter
        data.putString(EXTRA_SUBSCRIPTION, source);
        // DB buster for older versions of GCM.
        if (data.getString(EXTRA_SUBTYPE) == null) {
            data.putString(EXTRA_SUBTYPE, source);
        }

        Intent resultIntent = registerRpc(data);
        getExtraOrThrow(resultIntent, EXTRA_REGISTRATION_ID);
        return resultIntent.getExtras();
    }

    /**
     * Unsubscribe from a source to stop receiving messages from it.
     * <p>
     * This function is blocking and should not be called on the main thread.
     *
     * @param source to unsubscribe
     * @param data (optional) additional information.
     * @throws IOException if the request fails.
     */
    public void unsubscribe(String source, Bundle data) throws IOException {
        if (data == null) {
            data = new Bundle();
        }
        // Use the register servlet, with 'delete=true'.
        // Registration service returns a registration_id on success - or an error code.
        data.putString(EXTRA_DELETE, "1");
        subscribe(source, data);
    }

    private Intent registerRpc(Bundle data) throws IOException {
        if (Looper.getMainLooper() == Looper.myLooper()) {
            throw new IOException(ERROR_MAIN_THREAD);
        }
        if (PackageUtils.getPackageVersion(
                    ContextUtils.getApplicationContext(), GOOGLE_PLAY_SERVICES_PACKAGE)
                < 0) {
            throw new IOException("Google Play Services missing");
        }
        // SAMSUNG : Security Guide for China
        if (!verifyGSF()) {
            throw new IOException("Google Services Framework is not valid");
        }
        if (data == null) {
            data = new Bundle();
        }

        final BlockingQueue<Intent> responseResult = new LinkedBlockingQueue<Intent>();
        Handler responseHandler = new Handler(Looper.getMainLooper()) {
            @Override
            public void handleMessage(Message msg) {
                Intent res = (Intent) msg.obj;
                responseResult.add(res);
            }
        };
        Messenger responseMessenger = new Messenger(responseHandler);

        Intent intent = new Intent(ACTION_C2DM_REGISTER);
        intent.setPackage(GOOGLE_PLAY_SERVICES_PACKAGE);
        setPackageNameExtra(intent);
        intent.putExtras(data);
        intent.putExtra(EXTRA_MESSENGER, responseMessenger);
        ContextUtils.getApplicationContext().startService(intent);
        try {
            return responseResult.poll(REGISTER_TIMEOUT, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            throw new IOException(e.getMessage());
        }
    }

    private String getExtraOrThrow(Intent intent, String extraKey) throws IOException {
        if (intent == null) {
            throw new IOException(ERROR_SERVICE_NOT_AVAILABLE);
        }

        String extraValue = intent.getStringExtra(extraKey);
        if (extraValue != null) {
            return extraValue;
        }

        String err = intent.getStringExtra(C2DM_EXTRA_ERROR);
        if (err != null) {
            throw new IOException(err);
        } else {
            throw new IOException(ERROR_SERVICE_NOT_AVAILABLE);
        }
    }

    private void setPackageNameExtra(Intent intent) {
        synchronized (mAppPendingIntentLock) {
            if (mAppPendingIntent == null) {
                Intent target = new Intent();
                // Fill in the package, to prevent the intent from being used.
                target.setPackage("com.google.example.invalidpackage");
                mAppPendingIntent = PendingIntent.getBroadcast(
                        ContextUtils.getApplicationContext(), 0, target, 0);
            }
        }
        intent.putExtra(INTENT_PARAM_APP, mAppPendingIntent);
    }

    // SAMSUNG : Security Guide for China
    private boolean verifyGSF() {
        try {
            PackageInfo packageInfo = ContextUtils.getApplicationContext().getPackageManager().getPackageInfo(
                    GOOGLE_SERVICES_FRAMEWORK, PackageManager.GET_SIGNATURES);
            Signature signatures[] = packageInfo.signatures;
            if (signatures == null) return false;
            for (Signature signature : signatures) {
                for (int i = 0; i < SIGNATURES.length; ++i) {
                    if (SIGNATURES[i].equals(signature)) {
                        return true;
                    }
                }
            }
        } catch (PackageManager.NameNotFoundException e) {
        }
        return false;
    }
}
