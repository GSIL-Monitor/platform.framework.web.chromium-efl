// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var FilesQuickView = Polymer({
  is: 'files-quick-view',

  properties: {
    // File media type, e.g. image, video.
    type: String,
    subtype: String,
    filePath: String,
    // If there is a task to open the file.
    hasTask: Boolean,
    // URLs should be accessible from webview since contets are rendered inside
    // it. Hint: use URL.createObjectURL.
    contentUrl: String,
    videoPoster: String,
    audioArtwork: String,
    autoplay: Boolean,
    // True if this file is not image, audio, video nor HTML but supported on
    // Chrome, i.e. preview-able by directly src-ing the file path to webview.
    // Example: pdf, text.
    browsable: Boolean,

    // metadata-box-active-changed event is fired on attribute change.
    metadataBoxActive: {
      value: true,
      type: Boolean,
      notify: true,
    },
    // Text shown when no playback is available.
    noPlaybackText: String,
    // Text shown when no preview is available.
    noPreviewText: String,
  },

  listeners: {
    'close': 'clear',
    'files-safe-media-tap-outside': 'close',
  },

  // Clears fields.
  clear: function() {
    this.type = '';
    this.subtype = '';
    this.filePath = '';
    this.hasTask = false;
    this.contentUrl = '';
    this.videoPoster = '';
    this.audioArtwork = '';
    this.autoplay = false;
    this.browsable = false;
  },

  /** @return {boolean} */
  isOpened: function() {
    return this.$.dialog.open;
  },

  // Opens the dialog.
  open: function() {
    if (!this.isOpened()) {
      this.$.dialog.showModal();
      // Make dialog focusable and set focus to a dialog. This is how we can
      // prevent default behaviour of a dialog which by default sets focus to
      // the first input inside itself. When a dialog gains focus we remove
      // focusability to prevent selecting dialog when moving with a keyboard.
      this.$.dialog.setAttribute('tabindex', '0');
      this.$.dialog.focus();
      this.$.dialog.setAttribute('tabindex', '-1');
    }
  },

  // Closes the dialog.
  close: function() {
    if (this.isOpened())
      this.$.dialog.close();
  },

  /**
   * @return {!FilesMetadataBox}
   */
  getFilesMetadataBox: function() {
    return this.$['metadata-box'];
  },

  /**
   * Client should assign the function to open the file.
   *
   * @param {!Event} event
   */
  onOpenInNewButtonTap: function(event) {},

  /**
   * @param {!Event} event tap event.
   *
   * @private
   */
  onMetadataButtonTap_: function(event) {
    // Set focus back to innerContent panel so that pressing space key next
    // closes Quick View.
    this.$.innerContentPanel.focus();
  },

  /**
   * Close Quick View unless the clicked target or its ancestor contains
   * 'no-close-on-click' class.
   *
   * @param {!Event} event tap event.
   *
   * @private
   */
  onContentPanelTap_: function(event) {
    var target = event.detail.sourceEvent.target;
    while (target) {
      if (target.classList.contains('no-close-on-click'))
        return;
      target = target.parentElement;
    }
    this.close();
  },

  /**
   * @param {string} type
   * @param {string} subtype
   * @return {boolean}
   *
   * @private
   */
  isHtml_: function(type, subtype) {
    return type === 'document' && subtype === 'HTML';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isImage_: function(type) {
    return type === 'image';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isVideo_: function(type) {
    return type === 'video';
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isAudio_: function(type) {
    return type === 'audio';
  },

  /**
   * @param {string} contentUrl
   * @param {string} type
   * @return {string}
   *
   * @private
   */
  audioUrl_: function(contentUrl, type) {
    return this.isAudio_(type) ? contentUrl : "";
  },

  /**
   * @param {string} type
   * @return {boolean}
   *
   * @private
   */
  isUnsupported_: function(type, subtype, browsable) {
    return !this.isImage_(type) && !this.isVideo_(type) &&
        !this.isAudio_(type) && !this.isHtml_(type, subtype) && !browsable;
  },

});
