// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/navigatorcontentutils/testing/NavigatorContentUtilsClientMock.h"

#include "modules/navigatorcontentutils/NavigatorContentUtilsClient.h"
#include "platform/wtf/text/StringHash.h"

namespace blink {

void NavigatorContentUtilsClientMock::RegisterProtocolHandler(
    const String& scheme,
    const KURL& url,
    const String& title) {
  ProtocolInfo info;
  info.scheme = scheme;
  info.url = url;
  info.title = title;

  protocol_map_.Set(scheme, info);
}

NavigatorContentUtilsClient::CustomHandlersState
NavigatorContentUtilsClientMock::IsProtocolHandlerRegistered(
    const String& scheme,
    const KURL& url) {
  // "declined" state is checked by
  // NavigatorContentUtils::isProtocolHandlerRegistered() before calling this
  // function.
  if (protocol_map_.Contains(scheme))
    return NavigatorContentUtilsClient::kCustomHandlersRegistered;

  return NavigatorContentUtilsClient::kCustomHandlersNew;
}

void NavigatorContentUtilsClientMock::UnregisterProtocolHandler(
    const String& scheme,
    const KURL& url) {
  protocol_map_.erase(scheme);
}

void NavigatorContentUtilsClientMock::registerContentHandler(const String& mimetype, const KURL& baseURL,
    const KURL& url, const String& title)
{
    ContentInfo info;
    info.mimetype = mimetype;
    info.baseURL = baseURL;
    info.url = url;
    info.title = title;

    m_contentMap.Set(mimetype, info);
}

NavigatorContentUtilsClient::CustomHandlersState NavigatorContentUtilsClientMock::isContentHandlerRegistered(const String& mimetype,
    const KURL& baseURL, const KURL& url)
{
    // "declined" state is checked by NavigatorContentUtils::isContentHandlerRegistered() before calling this function.
    if (m_contentMap.Contains(mimetype))
        return NavigatorContentUtilsClient::kCustomHandlersRegistered;

    return NavigatorContentUtilsClient::kCustomHandlersNew;
}

void NavigatorContentUtilsClientMock::unregisterContentHandler(const String& mimetype, const KURL& baseURL,
    const KURL& url)
{
    //m_contentMap.remove(mimetype);
}

} // namespace blink
