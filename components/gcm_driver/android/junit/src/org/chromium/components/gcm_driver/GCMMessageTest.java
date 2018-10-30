// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.components.gcm_driver;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;

import android.os.Build;
import android.os.Bundle;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.robolectric.annotation.Config;

import org.chromium.base.test.util.MinAndroidSdkLevel;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for GCMMessage.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class GCMMessageTest {
    /**
     * Tests that a message object can be created based on data received from GCM. Note that the raw
     * data field is tested separately.
     */
    @Test
    public void testCreateMessageFromGCM() {
        Bundle extras = new Bundle();

        // Compose a simple message that lacks all optional fields.
        extras.putString("subtype", "MyAppId");

        {
            GCMMessage message = new GCMMessage("MySenderId", extras);

            assertEquals("MySenderId", message.getSenderId());
            assertEquals("MyAppId", message.getAppId());
            assertEquals(null, message.getCollapseKey());
            assertArrayEquals(new String[] {}, message.getDataKeysAndValuesArray());
        }

        // Add the optional fields: collapse key, raw binary data and a custom property.
        extras.putString("collapse_key", "MyCollapseKey");
        extras.putByteArray("rawData", new byte[] {0x00, 0x15, 0x30, 0x45});
        extras.putString("property", "value");

        {
            GCMMessage message = new GCMMessage("MySenderId", extras);

            assertEquals("MySenderId", message.getSenderId());
            assertEquals("MyAppId", message.getAppId());
            assertEquals("MyCollapseKey", message.getCollapseKey());
            assertArrayEquals(
                    new String[] {"property", "value"}, message.getDataKeysAndValuesArray());
        }
    }

    /**
     * Tests that the bundle containing extras from GCM will be filtered for values that we either
     * pass through by other means, or that we know to be irrelevant to regular GCM messages.
     */
    @Test
    public void testFiltersExtraBundle() {
        Bundle extras = new Bundle();

        // These should be filtered by full key.
        extras.putString("collapse_key", "collapseKey");
        extras.putString("rawData", "rawData");
        extras.putString("from", "from");
        extras.putString("subtype", "subtype");

        // These should be filtered by prefix matching.
        extras.putString("com.google.ipc.invalidation.gcmmplex.foo", "bar");
        extras.putString("com.google.ipc.invalidation.gcmmplex.bar", "baz");

        // These should be filtered because they're not strings.
        extras.putBoolean("myBoolean", true);
        extras.putInt("myInteger", 42);

        // These should not be filtered.
        extras.putString("collapse_key2", "secondCollapseKey");
        extras.putString("myString", "foobar");

        GCMMessage message = new GCMMessage("senderId", extras);

        assertArrayEquals(new String[] {"collapse_key2", "secondCollapseKey", "myString", "foobar"},
                message.getDataKeysAndValuesArray());
    }

    /**
     * Tests that a GCMMessage object can be serialized to and deserialized from a persistable
     * bundle. Note that the raw data field is tested separately. Only run on Android L and beyond
     * because it depends on PersistableBundle.
     */
    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.LOLLIPOP)
    public void testSerializationToPersistableBundle() {
        Bundle extras = new Bundle();

        // Compose a simple message that lacks all optional fields.
        extras.putString("subtype", "MyAppId");

        {
            GCMMessage message = new GCMMessage("MySenderId", extras);
            GCMMessage copiedMessage = new GCMMessage(message.toBundle());

            assertEquals(message.getSenderId(), copiedMessage.getSenderId());
            assertEquals(message.getAppId(), copiedMessage.getAppId());
            assertEquals(message.getCollapseKey(), copiedMessage.getCollapseKey());
            assertArrayEquals(
                    message.getDataKeysAndValuesArray(), copiedMessage.getDataKeysAndValuesArray());
        }

        // Add the optional fields: collapse key, raw binary data and a custom property.
        extras.putString("collapse_key", "MyCollapseKey");
        extras.putString("property", "value");

        {
            GCMMessage message = new GCMMessage("MySenderId", extras);
            GCMMessage copiedMessage = new GCMMessage(message.toBundle());

            assertEquals(message.getSenderId(), copiedMessage.getSenderId());
            assertEquals(message.getAppId(), copiedMessage.getAppId());
            assertEquals(message.getCollapseKey(), copiedMessage.getCollapseKey());
            assertArrayEquals(
                    message.getDataKeysAndValuesArray(), copiedMessage.getDataKeysAndValuesArray());
        }
    }

    /**
     * Tests that the raw data field can be serialized and deserialized as expected. It should be
     * NULL when undefined, an empty byte array when defined but empty, and a regular, filled
     * byte array when data has been provided. Only run on Android L and beyond because it depends
     * on PersistableBundle.
     */
    @Test
    @MinAndroidSdkLevel(Build.VERSION_CODES.LOLLIPOP)
    public void testRawDataSerializationBehaviour() {
        Bundle extras = new Bundle();
        extras.putString("subtype", "MyAppId");

        // Case 1: No raw data supplied. Should be NULL.
        {
            GCMMessage message = new GCMMessage("MySenderId", extras);
            GCMMessage copiedMessage = new GCMMessage(message.toBundle());

            assertArrayEquals(null, message.getRawData());
            assertArrayEquals(null, copiedMessage.getRawData());
        }

        extras.putByteArray("rawData", new byte[] {});

        // Case 2: Empty byte array of raw data supplied. Should be just that.
        {
            GCMMessage message = new GCMMessage("MySenderId", extras);
            GCMMessage copiedMessage = new GCMMessage(message.toBundle());

            assertArrayEquals(new byte[] {}, message.getRawData());
            assertArrayEquals(new byte[] {}, copiedMessage.getRawData());
        }

        extras.putByteArray("rawData", new byte[] {0x00, 0x15, 0x30, 0x45});

        // Case 3: Byte array with data supplied.
        {
            GCMMessage message = new GCMMessage("MySenderId", extras);
            GCMMessage copiedMessage = new GCMMessage(message.toBundle());

            assertArrayEquals(new byte[] {0x00, 0x15, 0x30, 0x45}, message.getRawData());
            assertArrayEquals(new byte[] {0x00, 0x15, 0x30, 0x45}, copiedMessage.getRawData());
        }
    }
}
