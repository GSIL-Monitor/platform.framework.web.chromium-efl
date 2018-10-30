// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Callback from the backend with the executable path to display.
 * @param {string} execPath The executable path to display.
 */
function returnExecutablePath(execPath) {
  $('executable_path').textContent = execPath;
}

/* All the work we do onload. */
function onLoadWork() {
  chrome.send('requestVersionInfo');
}

document.addEventListener('DOMContentLoaded', onLoadWork);