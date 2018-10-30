// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NavigatorContentUtilsClientMock_h
#define NavigatorContentUtilsClientMock_h

#include "modules/navigatorcontentutils/NavigatorContentUtilsClient.h"
#include "platform/heap/Handle.h"
#include "platform/weborigin/KURL.h"
#include "platform/wtf/HashSet.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

// Provides a mock object for the navigatorcontentutils client.
class NavigatorContentUtilsClientMock final
    : public NavigatorContentUtilsClient {
 public:
  static NavigatorContentUtilsClientMock* Create() {
    return new NavigatorContentUtilsClientMock;
  }

  ~NavigatorContentUtilsClientMock() override {}

  virtual void RegisterProtocolHandler(const String& scheme,
                                       const KURL&,
                                       const String& title);

  virtual CustomHandlersState IsProtocolHandlerRegistered(const String& scheme,
                                                          const KURL&);
  virtual void UnregisterProtocolHandler(const String& scheme, const KURL&);

  virtual void registerContentHandler(const String& scheme, const KURL& baseURL, const KURL&, const String& title);

  virtual CustomHandlersState isContentHandlerRegistered(const String& scheme, const KURL& baseURL, const KURL&);
  virtual void unregisterContentHandler(const String& scheme, const KURL& baseURL, const KURL&);

 private:
  // TODO(sashab): Make NavigatorContentUtilsClientMock non-virtual and test it
  // using a WebFrameClient mock.
  NavigatorContentUtilsClientMock() : NavigatorContentUtilsClient(nullptr) {}

  typedef struct {
    String scheme;
    KURL url;
    String title;
  } ProtocolInfo;

  typedef struct {
    String mimetype;
    KURL baseURL;
    KURL url;
    String title;
  } ContentInfo;

  typedef HashMap<String, ProtocolInfo> RegisteredProtocolMap;
  typedef HashMap<String, ContentInfo> RegisteredContentMap;
  RegisteredContentMap m_contentMap;
  RegisteredProtocolMap protocol_map_;
};

}  // namespace blink

#endif  // NavigatorContentUtilsClientMock_h
