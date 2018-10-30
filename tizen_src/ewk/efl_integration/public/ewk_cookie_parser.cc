/**
 * @file  ewk_cookie_parser
 * @brief EWK Cookie Parser
 *
 * This class exposes the Chromium cookie parser. It allows
 * for ewk components to handle cookie-alike structures without
 * re-inventing the wheel.
 *
 * Copyright 2018 by Samsung Electronics, Inc.,
 *
 * This software is the confidential and proprietary information
 * of Samsung Electronics, Inc. ("Confidential Information").  You
 * shall not disclose such Confidential Information and shall use
 * it only in accordance with the terms of the license agreement
 * you entered into with Samsung.
 */

#include <ctime>
#include <string>
#include "ewk_cookie_parser.h"
#include "net/cookies/cookie_util.h"
#include "net/cookies/parsed_cookie.h"
#include "private/ewk_private.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LCOV_EXCL_START */
EXPORT_API Eina_Bool ewk_parse_cookie(const std::string& cookie_str, EWKCookieContents& cookie)
{
  net::ParsedCookie new_cookie(cookie_str);

  if (new_cookie.IsValid() && !new_cookie.IsHttpOnly() && !new_cookie.IsSecure()) {
    cookie.name = new_cookie.Name();
    cookie.value = new_cookie.Value();
    cookie.domain = new_cookie.Domain();
    cookie.path = new_cookie.Path();

    // Take "max-age" over "Expires" as expires is depreciated, so only supported
    // for backwards compatibility. With the assumption that if you are using both
    // then "Expires" is there to support old browsers. (see HTTP - spec).
    if (new_cookie.HasMaxAge()) {
      cookie.expiry_date_utc = time(nullptr) + stoi(new_cookie.MaxAge());
    }
    else if (new_cookie.HasExpires()) {
      base::Time expiration_time =
          net::cookie_util::ParseCookieExpirationTime(new_cookie.Expires());
      cookie.expiry_date_utc = expiration_time.ToTimeT();
    }
    else {
      // This is a session cookie. Set expiry time to 0
      cookie.expiry_date_utc = 0;
    }
    return EINA_TRUE;
  }
  return EINA_FALSE;
}
/* LCOV_EXCL_STOP */

#ifdef __cplusplus
}
#endif


