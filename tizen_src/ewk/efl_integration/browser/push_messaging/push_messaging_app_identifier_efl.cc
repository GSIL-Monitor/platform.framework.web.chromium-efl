// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ewk/efl_integration/browser/push_messaging/push_messaging_app_identifier_efl.h"

#include <string.h>

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "content/public/browser/browser_context.h"

namespace {

const char kSeparator = '#';  // Ok as only the origin of the url is used.
const char* kDataSeparator = "##";

std::string MakePrefValue(const GURL& origin,
                          int64_t service_worker_registration_id) {
  return origin.spec() + kSeparator +
         base::Int64ToString(service_worker_registration_id);
}

bool GetOriginAndSWRFromPrefValue(const std::string& pref_value,
                                  GURL* origin,
                                  int64_t* service_worker_registration_id) {
  std::vector<std::string> parts =
      base::SplitString(pref_value, std::string(1, kSeparator),
                        base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  if (parts.size() != 2)
    return false;

  if (!base::StringToInt64(parts[1], service_worker_registration_id))
    return false;

  *origin = GURL(parts[0]);
  return origin->is_valid();
}

std::string MakePushData(const std::string& app_id, const std::string& data) {
  return app_id + kDataSeparator + data;
}

bool GetAppIDAndData(const std::string& push_data,
                     std::string& app_id,
                     std::string& data) {
  std::string::size_type pos = push_data.find(kDataSeparator);
  if (pos == std::string::npos || pos == 0)
    return false;

  app_id = push_data.substr(0, pos);
  data = push_data.substr(pos + strlen(kDataSeparator));
  return true;
}

}  // namespace

// static
PushMessagingAppIdentifierEfl PushMessagingAppIdentifierEfl::Generate(
    const GURL& origin,
    int64_t service_worker_registration_id) {
  std::string app_id = MakePrefValue(origin, service_worker_registration_id);

  PushMessagingAppIdentifierEfl app_identifier(app_id, origin,
                                               service_worker_registration_id);
  app_identifier.DCheckValid();
  return app_identifier;
}

// static
PushMessagingAppIdentifierEfl PushMessagingAppIdentifierEfl::Generate(
    const std::string& app_id) {
  GURL origin;
  int64_t service_worker_registration_id;
  if (!GetOriginAndSWRFromPrefValue(app_id, &origin,
                                    &service_worker_registration_id))
    return PushMessagingAppIdentifierEfl();

  PushMessagingAppIdentifierEfl app_identifier(app_id, origin,
                                               service_worker_registration_id);
  app_identifier.DCheckValid();
  return app_identifier;
}

// static
PushMessagingAppIdentifierEfl
PushMessagingAppIdentifierEfl::GenerateFromPushData(
    const std::string& push_data,
    std::string& data) {
  std::string app_id;
  if (!GetAppIDAndData(push_data, app_id, data))
    return PushMessagingAppIdentifierEfl();

  return Generate(app_id);
}

PushMessagingAppIdentifierEfl::PushMessagingAppIdentifierEfl()
    : origin_(GURL::EmptyGURL()), service_worker_registration_id_(-1) {}

PushMessagingAppIdentifierEfl::PushMessagingAppIdentifierEfl(
    const std::string& app_id,
    const GURL& origin,
    int64_t service_worker_registration_id)
    : app_id_(app_id),
      origin_(origin),
      service_worker_registration_id_(service_worker_registration_id) {}

PushMessagingAppIdentifierEfl::~PushMessagingAppIdentifierEfl() {}

std::string PushMessagingAppIdentifierEfl::push_data(
    const std::string& data) const {
  return MakePushData(app_id_, data);
}

void PushMessagingAppIdentifierEfl::DCheckValid() const {
  DCHECK_GE(service_worker_registration_id_, 0);

  DCHECK(origin_.is_valid());
  DCHECK_EQ(origin_.GetOrigin(), origin_);

  GURL origin;
  int64_t service_worker_registration_id;
  DCHECK(GetOriginAndSWRFromPrefValue(app_id_, &origin,
                                      &service_worker_registration_id));
  DCHECK_EQ(origin_, origin);
  DCHECK_EQ(service_worker_registration_id_, service_worker_registration_id);
}
