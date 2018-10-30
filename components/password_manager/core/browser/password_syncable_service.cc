// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/password_manager/core/browser/password_syncable_service.h"

#include <algorithm>
#include <iterator>
#include <memory>
#include <utility>

#include "base/auto_reset.h"
#include "base/location.h"
#include "base/metrics/histogram_macros.h"
#include "base/optional.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/default_clock.h"
#include "components/autofill/core/common/password_form.h"
#include "components/password_manager/core/browser/android_affiliation/affiliation_utils.h"
#include "components/password_manager/core/browser/password_manager_metrics_util.h"
#include "components/password_manager/core/browser/password_store_sync.h"
#include "components/sync/model/sync_error_factory.h"
#include "net/base/escape.h"

namespace password_manager {

// Converts the |password| into a SyncData object.
syncer::SyncData SyncDataFromPassword(const autofill::PasswordForm& password);

// Extracts the |PasswordForm| data from sync's protobuf format.
autofill::PasswordForm PasswordFromSpecifics(
    const sync_pb::PasswordSpecificsData& password);

// Returns the unique tag that will serve as the sync identifier for the
// |password| entry.
std::string MakePasswordSyncTag(const sync_pb::PasswordSpecificsData& password);
std::string MakePasswordSyncTag(const autofill::PasswordForm& password);

namespace {

// Returns true iff |password_specifics| and |password_specifics| are equal
// in the fields which don't comprise the sync tag.
bool AreLocalAndSyncPasswordsNonSyncTagEqual(
    const sync_pb::PasswordSpecificsData& password_specifics,
    const autofill::PasswordForm& password_form) {
  return (password_form.scheme == password_specifics.scheme() &&
          password_form.action.spec() == password_specifics.action() &&
          base::UTF16ToUTF8(password_form.password_value) ==
              password_specifics.password_value() &&
          password_form.preferred == password_specifics.preferred() &&
          password_form.date_created.ToInternalValue() ==
              password_specifics.date_created() &&
          password_form.blacklisted_by_user ==
              password_specifics.blacklisted() &&
          password_form.type == password_specifics.type() &&
          password_form.times_used == password_specifics.times_used() &&
          base::UTF16ToUTF8(password_form.display_name) ==
              password_specifics.display_name() &&
          password_form.icon_url.spec() == password_specifics.avatar_url() &&
          url::Origin(GURL(password_specifics.federation_url())).Serialize() ==
              password_form.federation_origin.Serialize());
}

// Returns true iff |password_specifics| and |password_specifics| are equal
// memberwise.
bool AreLocalAndSyncPasswordsEqual(
    const sync_pb::PasswordSpecificsData& password_specifics,
    const autofill::PasswordForm& password_form) {
  return (password_form.signon_realm == password_specifics.signon_realm() &&
          password_form.origin.spec() == password_specifics.origin() &&
          base::UTF16ToUTF8(password_form.username_element) ==
              password_specifics.username_element() &&
          base::UTF16ToUTF8(password_form.password_element) ==
              password_specifics.password_element() &&
          base::UTF16ToUTF8(password_form.username_value) ==
              password_specifics.username_value() &&
          AreLocalAndSyncPasswordsNonSyncTagEqual(password_specifics,
                                                  password_form));
}

// Compares the fields which are not part of the sync tag.
bool AreNonSyncTagFieldsEqual(const sync_pb::PasswordSpecificsData& left,
                              const sync_pb::PasswordSpecificsData& right) {
  return (left.scheme() == right.scheme() && left.action() == right.action() &&
          left.password_value() == right.password_value() &&
          left.preferred() == right.preferred() &&
          left.date_created() == right.date_created() &&
          left.blacklisted() == right.blacklisted() &&
          left.type() == right.type() &&
          left.times_used() == right.times_used() &&
          left.display_name() == right.display_name() &&
          left.avatar_url() == right.avatar_url() &&
          left.federation_url() == right.federation_url());
}

syncer::SyncChange::SyncChangeType GetSyncChangeType(
    PasswordStoreChange::Type type) {
  switch (type) {
    case PasswordStoreChange::ADD:
      return syncer::SyncChange::ACTION_ADD;
    case PasswordStoreChange::UPDATE:
      return syncer::SyncChange::ACTION_UPDATE;
    case PasswordStoreChange::REMOVE:
      return syncer::SyncChange::ACTION_DELETE;
  }
  NOTREACHED();
  return syncer::SyncChange::ACTION_INVALID;
}

// Creates a PasswordForm from |specifics| and |sync_time|, appends it to
// |entries|.
void AppendPasswordFromSpecifics(
    const sync_pb::PasswordSpecificsData& specifics,
    base::Time sync_time,
    std::vector<std::unique_ptr<autofill::PasswordForm>>* entries) {
  entries->push_back(std::make_unique<autofill::PasswordForm>(
      PasswordFromSpecifics(specifics)));
  entries->back()->date_synced = sync_time;
}

// Android autofill saves credential in a different format without trailing '/'.
std::string GetIncorrectAndroidSignonRealm(std::string android_autofill_realm) {
  if (base::EndsWith(android_autofill_realm, "/", base::CompareCase::SENSITIVE))
    android_autofill_realm.erase(android_autofill_realm.size() - 1);
  return android_autofill_realm;
}

// The correct Android signon_realm should have a trailing '/'.
std::string GetCorrectAndroidSignonRealm(std::string android_realm) {
  if (!base::EndsWith(android_realm, "/", base::CompareCase::SENSITIVE))
    android_realm += '/';
  return android_realm;
}

// Android autofill saves credentials in a different format without trailing
// '/'. Return a sync tag for the style used by Android Autofill in GMS Core
// v12.
std::string AndroidAutofillSyncTag(
    const sync_pb::PasswordSpecificsData& password) {
  // realm has the same value as the origin.
  std::string origin = GetIncorrectAndroidSignonRealm(password.origin());
  std::string signon_realm =
      GetIncorrectAndroidSignonRealm(password.signon_realm());
  return (net::EscapePath(origin) + "|" +
          net::EscapePath(password.username_element()) + "|" +
          net::EscapePath(password.username_value()) + "|" +
          net::EscapePath(password.password_element()) + "|" +
          net::EscapePath(signon_realm));
}

// Return a sync tag for the correct format used by Android.
std::string AndroidCorrectSyncTag(
    const sync_pb::PasswordSpecificsData& password) {
  // realm has the same value as the origin.
  std::string origin = GetCorrectAndroidSignonRealm(password.origin());
  std::string signon_realm =
      GetCorrectAndroidSignonRealm(password.signon_realm());
  return (net::EscapePath(origin) + "|" +
          net::EscapePath(password.username_element()) + "|" +
          net::EscapePath(password.username_value()) + "|" +
          net::EscapePath(password.password_element()) + "|" +
          net::EscapePath(signon_realm));
}

void PasswordSpecificsFromPassword(
    const autofill::PasswordForm& password_form,
    sync_pb::PasswordSpecificsData* password_specifics) {
#define CopyField(field) password_specifics->set_##field(password_form.field)
#define CopyStringField(field) \
  password_specifics->set_##field(base::UTF16ToUTF8(password_form.field))
  CopyField(scheme);
  CopyField(signon_realm);
  password_specifics->set_origin(password_form.origin.spec());
  password_specifics->set_action(password_form.action.spec());
  CopyStringField(username_element);
  CopyStringField(password_element);
  CopyStringField(username_value);
  CopyStringField(password_value);
  CopyField(preferred);
  password_specifics->set_date_created(
      password_form.date_created.ToInternalValue());
  password_specifics->set_blacklisted(password_form.blacklisted_by_user);
  CopyField(type);
  CopyField(times_used);
  CopyStringField(display_name);
  password_specifics->set_avatar_url(password_form.icon_url.spec());
  password_specifics->set_federation_url(
      password_form.federation_origin.unique()
          ? std::string()
          : password_form.federation_origin.Serialize());
#undef CopyStringField
#undef CopyField
}

struct AndroidMergeResult {
  // New value for Android entry in the correct format.
  base::Optional<syncer::SyncData> new_android_correct;
  // New value for Android autofill entry.
  base::Optional<syncer::SyncData> new_android_incorrect;
  // New value for local entry in the correct format.
  base::Optional<autofill::PasswordForm> new_local_correct;
  // New value for local entry in the Android autofill format.
  base::Optional<autofill::PasswordForm> new_local_incorrect;
};

// Perform deduplication of Android credentials saved in the wrong format. As
// the result all the four entries should be created and have the same value.
AndroidMergeResult Perform4WayMergeAndroidCredentials(
    const sync_pb::PasswordSpecificsData* correct_android,
    const sync_pb::PasswordSpecificsData* incorrect_android,
    const autofill::PasswordForm* correct_local,
    const autofill::PasswordForm* incorrect_local) {
  AndroidMergeResult result;

  base::Optional<sync_pb::PasswordSpecificsData> local_correct_ps;
  if (correct_local) {
    local_correct_ps = sync_pb::PasswordSpecificsData();
    PasswordSpecificsFromPassword(*correct_local, &local_correct_ps.value());
  }

  base::Optional<sync_pb::PasswordSpecificsData> local_incorrect_ps;
  if (incorrect_local) {
    local_incorrect_ps = sync_pb::PasswordSpecificsData();
    PasswordSpecificsFromPassword(*incorrect_local,
                                  &local_incorrect_ps.value());
  }

  const sync_pb::PasswordSpecificsData* all_data[4] = {
      correct_android, incorrect_android,
      local_correct_ps ? &local_correct_ps.value() : nullptr,
      local_incorrect_ps ? &local_incorrect_ps.value() : nullptr};

  // |newest_data| will point to the newest entry out of all 4.
  const sync_pb::PasswordSpecificsData* newest_data = nullptr;
  for (int i = 0; i < 4; ++i) {
    if (newest_data && all_data[i]) {
      if (all_data[i]->date_created() > newest_data->date_created())
        newest_data = all_data[i];
    } else if (all_data[i]) {
      newest_data = all_data[i];
    }
  }
  DCHECK(newest_data);

  const std::string correct_tag = AndroidCorrectSyncTag(*newest_data);
  const std::string incorrect_tag = AndroidAutofillSyncTag(*newest_data);
  const std::string correct_signon_realm =
      GetCorrectAndroidSignonRealm(newest_data->signon_realm());
  const std::string incorrect_signon_realm =
      GetIncorrectAndroidSignonRealm(newest_data->signon_realm());
  const std::string correct_origin =
      GetCorrectAndroidSignonRealm(newest_data->origin());
  const std::string incorrect_origin =
      GetIncorrectAndroidSignonRealm(newest_data->origin());
  DCHECK_EQ(GURL(incorrect_origin).spec(), incorrect_origin);

  // Set the correct Sync entry if needed.
  if (newest_data != correct_android &&
      (!correct_android ||
       !AreNonSyncTagFieldsEqual(*correct_android, *newest_data))) {
    sync_pb::EntitySpecifics password_data;
    sync_pb::PasswordSpecificsData* password_specifics =
        password_data.mutable_password()->mutable_client_only_encrypted_data();
    *password_specifics = *newest_data;
    password_specifics->set_origin(correct_origin);
    password_specifics->set_signon_realm(correct_signon_realm);
    result.new_android_correct = syncer::SyncData::CreateLocalData(
        correct_tag, correct_tag, password_data);
  }

  // Set the Andoroid Autofill Sync entry if needed.
  if (newest_data != incorrect_android &&
      (!incorrect_android ||
       !AreNonSyncTagFieldsEqual(*incorrect_android, *newest_data))) {
    sync_pb::EntitySpecifics password_data;
    sync_pb::PasswordSpecificsData* password_specifics =
        password_data.mutable_password()->mutable_client_only_encrypted_data();
    *password_specifics = *newest_data;
    password_specifics->set_origin(incorrect_origin);
    password_specifics->set_signon_realm(incorrect_signon_realm);
    result.new_android_incorrect = syncer::SyncData::CreateLocalData(
        incorrect_tag, incorrect_tag, password_data);
  }

  // Set the correct local entry if needed.
  if (!local_correct_ps ||
      (newest_data != &local_correct_ps.value() &&
       !AreNonSyncTagFieldsEqual(local_correct_ps.value(), *newest_data))) {
    result.new_local_correct = PasswordFromSpecifics(*newest_data);
    result.new_local_correct.value().origin = GURL(correct_origin);
    result.new_local_correct.value().signon_realm = correct_signon_realm;
  }

  // Set the incorrect local entry if needed.
  if (!local_incorrect_ps ||
      (newest_data != &local_incorrect_ps.value() &&
       !AreNonSyncTagFieldsEqual(local_incorrect_ps.value(), *newest_data))) {
    result.new_local_incorrect = PasswordFromSpecifics(*newest_data);
    result.new_local_incorrect.value().origin = GURL(incorrect_origin);
    result.new_local_incorrect.value().signon_realm = incorrect_signon_realm;
  }
  return result;
}

}  // namespace

struct PasswordSyncableService::SyncEntries {
  std::vector<std::unique_ptr<autofill::PasswordForm>>* EntriesForChangeType(
      syncer::SyncChange::SyncChangeType type) {
    switch (type) {
      case syncer::SyncChange::ACTION_ADD:
        return &new_entries;
      case syncer::SyncChange::ACTION_UPDATE:
        return &updated_entries;
      case syncer::SyncChange::ACTION_DELETE:
        return &deleted_entries;
      case syncer::SyncChange::ACTION_INVALID:
        return nullptr;
    }
    NOTREACHED();
    return nullptr;
  }

  // List that contains the entries that are known only to sync.
  std::vector<std::unique_ptr<autofill::PasswordForm>> new_entries;

  // List that contains the entries that are known to both sync and the local
  // database but have updates in sync. They need to be updated in the local
  // database.
  std::vector<std::unique_ptr<autofill::PasswordForm>> updated_entries;

  // The list of entries to be deleted from the local database.
  std::vector<std::unique_ptr<autofill::PasswordForm>> deleted_entries;
};

PasswordSyncableService::PasswordSyncableService(
    PasswordStoreSync* password_store)
    : password_store_(password_store),
      clock_(new base::DefaultClock),
      is_processing_sync_changes_(false) {}

PasswordSyncableService::~PasswordSyncableService() = default;

syncer::SyncMergeResult PasswordSyncableService::MergeDataAndStartSyncing(
    syncer::ModelType type,
    const syncer::SyncDataList& initial_sync_data,
    std::unique_ptr<syncer::SyncChangeProcessor> sync_processor,
    std::unique_ptr<syncer::SyncErrorFactory> sync_error_factory) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(syncer::PASSWORDS, type);
  base::AutoReset<bool> processing_changes(&is_processing_sync_changes_, true);
  syncer::SyncMergeResult merge_result(type);

  // We add all the db entries as |new_local_entries| initially. During model
  // association entries that match a sync entry will be removed and this list
  // will only contain entries that are not in sync.
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_entries;
  PasswordEntryMap new_local_entries;
  if (!ReadFromPasswordStore(&password_entries, &new_local_entries)) {
    merge_result.set_error(sync_error_factory->CreateAndUploadError(
        FROM_HERE, "Failed to get passwords from store."));
    metrics_util::LogPasswordSyncState(metrics_util::NOT_SYNCING_FAILED_READ);
    return merge_result;
  }

  if (password_entries.size() != new_local_entries.size()) {
    merge_result.set_error(sync_error_factory->CreateAndUploadError(
        FROM_HERE,
        "There are passwords with identical sync tags in the database."));
    metrics_util::LogPasswordSyncState(
        metrics_util::NOT_SYNCING_DUPLICATE_TAGS);
    return merge_result;
  }
  merge_result.set_num_items_before_association(new_local_entries.size());

  // Changes from Sync to be applied locally.
  SyncEntries sync_entries;
  // Changes from password db that need to be propagated to sync.
  syncer::SyncChangeList updated_db_entries;
  MergeSyncDataWithLocalData(initial_sync_data, &new_local_entries,
                             &sync_entries, &updated_db_entries);

  for (PasswordEntryMap::iterator it = new_local_entries.begin();
       it != new_local_entries.end(); ++it) {
    updated_db_entries.push_back(
        syncer::SyncChange(FROM_HERE, syncer::SyncChange::ACTION_ADD,
                           SyncDataFromPassword(*it->second)));
  }

  WriteToPasswordStore(sync_entries);
  merge_result.set_error(
      sync_processor->ProcessSyncChanges(FROM_HERE, updated_db_entries));
  if (merge_result.error().IsSet()) {
    metrics_util::LogPasswordSyncState(metrics_util::NOT_SYNCING_SERVER_ERROR);
    return merge_result;
  }

  merge_result.set_num_items_after_association(
      merge_result.num_items_before_association() +
      sync_entries.new_entries.size());
  merge_result.set_num_items_added(sync_entries.new_entries.size());
  merge_result.set_num_items_modified(sync_entries.updated_entries.size());
  merge_result.set_num_items_deleted(sync_entries.deleted_entries.size());

  // Save |sync_processor_| only if the whole procedure succeeded. In case of
  // failure Sync shouldn't receive any updates from the PasswordStore.
  sync_error_factory_ = std::move(sync_error_factory);
  sync_processor_ = std::move(sync_processor);

  metrics_util::LogPasswordSyncState(metrics_util::SYNCING_OK);
  return merge_result;
}

void PasswordSyncableService::StopSyncing(syncer::ModelType type) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(syncer::PASSWORDS, type);

  sync_processor_.reset();
  sync_error_factory_.reset();
}

syncer::SyncDataList PasswordSyncableService::GetAllSyncData(
    syncer::ModelType type) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DCHECK_EQ(syncer::PASSWORDS, type);
  std::vector<std::unique_ptr<autofill::PasswordForm>> password_entries;
  ReadFromPasswordStore(&password_entries, nullptr);

  syncer::SyncDataList sync_data;
  sync_data.reserve(password_entries.size());
  std::transform(password_entries.begin(), password_entries.end(),
                 std::back_inserter(sync_data),
                 [](const std::unique_ptr<autofill::PasswordForm>& form) {
                   return SyncDataFromPassword(*form);
                 });
  return sync_data;
}

syncer::SyncError PasswordSyncableService::ProcessSyncChanges(
    const base::Location& from_here,
    const syncer::SyncChangeList& change_list) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::AutoReset<bool> processing_changes(&is_processing_sync_changes_, true);
  SyncEntries sync_entries;
  base::Time time_now = clock_->Now();

  for (syncer::SyncChangeList::const_iterator it = change_list.begin();
       it != change_list.end(); ++it) {
    const sync_pb::EntitySpecifics& specifics = it->sync_data().GetSpecifics();
    std::vector<std::unique_ptr<autofill::PasswordForm>>* entries =
        sync_entries.EntriesForChangeType(it->change_type());
    if (!entries) {
      return sync_error_factory_->CreateAndUploadError(
          FROM_HERE, "Failed to process sync changes for passwords datatype.");
    }
    AppendPasswordFromSpecifics(
        specifics.password().client_only_encrypted_data(), time_now, entries);
    if (IsValidAndroidFacetURI(entries->back()->signon_realm)) {
      // Fix the Android Autofill credentials if needed.
      entries->back()->signon_realm =
          GetCorrectAndroidSignonRealm(entries->back()->signon_realm);
      entries->back()->origin =
          GURL(GetCorrectAndroidSignonRealm(entries->back()->origin.spec()));
    }
  }

  WriteToPasswordStore(sync_entries);
  return syncer::SyncError();
}

void PasswordSyncableService::ActOnPasswordStoreChanges(
    const PasswordStoreChangeList& local_changes) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!sync_processor_) {
    if (!flare_.is_null()) {
      flare_.Run(syncer::PASSWORDS);
      flare_.Reset();
    }
    return;
  }

  // ActOnPasswordStoreChanges() can be called from ProcessSyncChanges(). Do
  // nothing in this case.
  if (is_processing_sync_changes_)
    return;
  syncer::SyncChangeList sync_changes;
  for (PasswordStoreChangeList::const_iterator it = local_changes.begin();
       it != local_changes.end(); ++it) {
    syncer::SyncData data = (it->type() == PasswordStoreChange::REMOVE ?
        syncer::SyncData::CreateLocalDelete(MakePasswordSyncTag(it->form()),
                                            syncer::PASSWORDS) :
        SyncDataFromPassword(it->form()));
    sync_changes.push_back(
        syncer::SyncChange(FROM_HERE, GetSyncChangeType(it->type()), data));
  }
  sync_processor_->ProcessSyncChanges(FROM_HERE, sync_changes);
}

void PasswordSyncableService::InjectStartSyncFlare(
    const syncer::SyncableService::StartSyncFlare& flare) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  flare_ = flare;
}

bool PasswordSyncableService::ReadFromPasswordStore(
    std::vector<std::unique_ptr<autofill::PasswordForm>>* password_entries,
    PasswordEntryMap* passwords_entry_map) const {
  DCHECK(password_entries);
  std::vector<std::unique_ptr<autofill::PasswordForm>> autofillable_entries;
  std::vector<std::unique_ptr<autofill::PasswordForm>> blacklist_entries;
  if (!password_store_->FillAutofillableLogins(&autofillable_entries) ||
      !password_store_->FillBlacklistLogins(&blacklist_entries)) {
    // Password store often fails to load passwords. Track failures with UMA.
    // (http://crbug.com/249000)
    // TODO(wychen): enum uma should be strongly typed. crbug.com/661401
    UMA_HISTOGRAM_ENUMERATION("Sync.LocalDataFailedToLoad",
                              ModelTypeToHistogramInt(syncer::PASSWORDS),
                              static_cast<int>(syncer::MODEL_TYPE_COUNT));
    return false;
  }
  password_entries->resize(autofillable_entries.size() +
                           blacklist_entries.size());
  std::move(autofillable_entries.begin(), autofillable_entries.end(),
            password_entries->begin());
  std::move(blacklist_entries.begin(), blacklist_entries.end(),
            password_entries->begin() + autofillable_entries.size());

  if (!passwords_entry_map)
    return true;

  PasswordEntryMap& entry_map = *passwords_entry_map;
  for (const auto& form : *password_entries) {
    autofill::PasswordForm* password_form = form.get();
    entry_map[MakePasswordSyncTag(*password_form)] = password_form;
  }

  return true;
}

void PasswordSyncableService::WriteToPasswordStore(const SyncEntries& entries) {
  PasswordStoreChangeList changes;
  WriteEntriesToDatabase(&PasswordStoreSync::AddLoginSync, entries.new_entries,
                         &changes);
  WriteEntriesToDatabase(&PasswordStoreSync::UpdateLoginSync,
                         entries.updated_entries, &changes);
  WriteEntriesToDatabase(&PasswordStoreSync::RemoveLoginSync,
                         entries.deleted_entries, &changes);

  // We have to notify password store observers of the change by hand since
  // we use internal password store interfaces to make changes synchronously.
  password_store_->NotifyLoginsChanged(changes);
}

void PasswordSyncableService::MergeSyncDataWithLocalData(
    const syncer::SyncDataList& sync_data,
    PasswordEntryMap* unmatched_data_from_password_db,
    SyncEntries* sync_entries,
    syncer::SyncChangeList* updated_db_entries) {
  std::map<std::string, const sync_pb::PasswordSpecificsData*> sync_data_map;
  for (const auto& data : sync_data) {
    const sync_pb::EntitySpecifics& specifics = data.GetSpecifics();
    const sync_pb::PasswordSpecificsData* password_specifics =
        &specifics.password().client_only_encrypted_data();
    sync_data_map[MakePasswordSyncTag(*password_specifics)] =
        password_specifics;
  }
  DCHECK_EQ(sync_data_map.size(), sync_data.size());

  for (auto it = sync_data_map.begin(); it != sync_data_map.end();) {
    if (IsValidAndroidFacetURI(it->second->signon_realm())) {
      // Perform deduplication of Android credentials saved in the wrong format.
      // For each incorrect entry, a duplicate of it is created in the correct
      // format, so Chrome can make use of it. The incorrect sync entries are
      // not deleted for now.
      std::string incorrect_tag = AndroidAutofillSyncTag(*it->second);
      std::string correct_tag = AndroidCorrectSyncTag(*it->second);
      auto it_sync_incorrect = sync_data_map.find(incorrect_tag);
      auto it_sync_correct = sync_data_map.find(correct_tag);
      auto it_local_data_correct =
          unmatched_data_from_password_db->find(correct_tag);
      auto it_local_data_incorrect =
          unmatched_data_from_password_db->find(incorrect_tag);
      if ((it != it_sync_incorrect && it != it_sync_correct) ||
          (it_sync_incorrect == sync_data_map.end() &&
           it_local_data_incorrect == unmatched_data_from_password_db->end())) {
        // The current credential is in an unexpected format or incorrect
        // credential don't exist. Just do what Sync would normally do.
        CreateOrUpdateEntry(*it->second, unmatched_data_from_password_db,
                            sync_entries, updated_db_entries);
        ++it;
      } else {
        AndroidMergeResult result = Perform4WayMergeAndroidCredentials(
            it_sync_correct == sync_data_map.end() ? nullptr
                                                   : it_sync_correct->second,
            it_sync_incorrect == sync_data_map.end()
                ? nullptr
                : it_sync_incorrect->second,
            it_local_data_correct == unmatched_data_from_password_db->end()
                ? nullptr
                : it_local_data_correct->second,
            it_local_data_incorrect == unmatched_data_from_password_db->end()
                ? nullptr
                : it_local_data_incorrect->second);
        // Add or update the correct local entry.
        if (result.new_local_correct) {
          auto* local_changes = sync_entries->EntriesForChangeType(
              it_local_data_correct == unmatched_data_from_password_db->end()
                  ? syncer::SyncChange::ACTION_ADD
                  : syncer::SyncChange::ACTION_UPDATE);
          local_changes->push_back(std::make_unique<autofill::PasswordForm>(
              result.new_local_correct.value()));
          local_changes->back()->date_synced = clock_->Now();
        }
        // Add or update the incorrect local entry.
        if (result.new_local_incorrect) {
          auto* local_changes = sync_entries->EntriesForChangeType(
              it_local_data_incorrect == unmatched_data_from_password_db->end()
                  ? syncer::SyncChange::ACTION_ADD
                  : syncer::SyncChange::ACTION_UPDATE);
          local_changes->push_back(std::make_unique<autofill::PasswordForm>(
              result.new_local_incorrect.value()));
          local_changes->back()->date_synced = clock_->Now();
        }
        if (it_local_data_correct != unmatched_data_from_password_db->end())
          unmatched_data_from_password_db->erase(it_local_data_correct);
        if (it_local_data_incorrect != unmatched_data_from_password_db->end())
          unmatched_data_from_password_db->erase(it_local_data_incorrect);
        // Add or update the correct sync entry.
        if (result.new_android_correct) {
          updated_db_entries->push_back(
              syncer::SyncChange(FROM_HERE,
                                 it_sync_correct == sync_data_map.end()
                                     ? syncer::SyncChange::ACTION_ADD
                                     : syncer::SyncChange::ACTION_UPDATE,
                                 result.new_android_correct.value()));
        }
        // Add or update the Android Autofill sync entry.
        if (result.new_android_incorrect) {
          updated_db_entries->push_back(
              syncer::SyncChange(FROM_HERE,
                                 it_sync_incorrect == sync_data_map.end()
                                     ? syncer::SyncChange::ACTION_ADD
                                     : syncer::SyncChange::ACTION_UPDATE,
                                 result.new_android_incorrect.value()));
        }
        bool increment = true;
        for (auto sync_data_it : {it_sync_incorrect, it_sync_correct}) {
          if (sync_data_it != sync_data_map.end()) {
            if (sync_data_it == it) {
              it = sync_data_map.erase(it);
              increment = false;
            } else {
              sync_data_map.erase(sync_data_it);
            }
          }
        }
        if (increment)
          ++it;
      }
    } else {
      // Not Android.
      CreateOrUpdateEntry(*it->second, unmatched_data_from_password_db,
                          sync_entries, updated_db_entries);
      ++it;
    }
  }
}

void PasswordSyncableService::CreateOrUpdateEntry(
    const sync_pb::PasswordSpecificsData& password_specifics,
    PasswordEntryMap* unmatched_data_from_password_db,
    SyncEntries* sync_entries,
    syncer::SyncChangeList* updated_db_entries) {
  std::string tag = MakePasswordSyncTag(password_specifics);

  // Check whether the data from sync is already in the password store.
  PasswordEntryMap::iterator existing_local_entry_iter =
      unmatched_data_from_password_db->find(tag);
  base::Time time_now = clock_->Now();
  if (existing_local_entry_iter == unmatched_data_from_password_db->end()) {
      // The sync data is not in the password store, so we need to create it in
      // the password store. Add the entry to the new_entries list.
      AppendPasswordFromSpecifics(password_specifics, time_now,
                                  &sync_entries->new_entries);
  } else {
    // The entry is in password store. If the entries are not identical, then
    // the entries need to be merged.
    // If the passwords differ, take the one that was created more recently.
    const autofill::PasswordForm& password_form =
        *existing_local_entry_iter->second;
    if (!AreLocalAndSyncPasswordsEqual(password_specifics, password_form)) {
      if (base::Time::FromInternalValue(password_specifics.date_created()) <
          password_form.date_created) {
        updated_db_entries->push_back(
            syncer::SyncChange(FROM_HERE,
                               syncer::SyncChange::ACTION_UPDATE,
                               SyncDataFromPassword(password_form)));
      } else {
        AppendPasswordFromSpecifics(password_specifics, time_now,
                                    &sync_entries->updated_entries);
      }
    }
    // Remove the entry from the entry map to indicate a match has been found.
    // Entries that remain in the map at the end of associating all sync entries
    // will be treated as additions that need to be propagated to sync.
    unmatched_data_from_password_db->erase(existing_local_entry_iter);
  }
}

void PasswordSyncableService::WriteEntriesToDatabase(
    DatabaseOperation operation,
    const std::vector<std::unique_ptr<autofill::PasswordForm>>& entries,
    PasswordStoreChangeList* all_changes) {
  for (const std::unique_ptr<autofill::PasswordForm>& form : entries) {
    PasswordStoreChangeList new_changes = (password_store_->*operation)(*form);
    all_changes->insert(all_changes->end(),
                        new_changes.begin(),
                        new_changes.end());
  }
}

syncer::SyncData SyncDataFromPassword(
    const autofill::PasswordForm& password_form) {
  sync_pb::EntitySpecifics password_data;
  sync_pb::PasswordSpecificsData* password_specifics =
      password_data.mutable_password()->mutable_client_only_encrypted_data();
  PasswordSpecificsFromPassword(password_form, password_specifics);

  std::string tag = MakePasswordSyncTag(*password_specifics);
  return syncer::SyncData::CreateLocalData(tag, tag, password_data);
}

autofill::PasswordForm PasswordFromSpecifics(
    const sync_pb::PasswordSpecificsData& password) {
  autofill::PasswordForm new_password;
  new_password.scheme =
      static_cast<autofill::PasswordForm::Scheme>(password.scheme());
  new_password.signon_realm = password.signon_realm();
  new_password.origin = GURL(password.origin());
  new_password.action = GURL(password.action());
  new_password.username_element =
      base::UTF8ToUTF16(password.username_element());
  new_password.password_element =
      base::UTF8ToUTF16(password.password_element());
  new_password.username_value = base::UTF8ToUTF16(password.username_value());
  new_password.password_value = base::UTF8ToUTF16(password.password_value());
  new_password.preferred = password.preferred();
  new_password.date_created =
      base::Time::FromInternalValue(password.date_created());
  new_password.blacklisted_by_user = password.blacklisted();
  new_password.type =
      static_cast<autofill::PasswordForm::Type>(password.type());
  new_password.times_used = password.times_used();
  new_password.display_name = base::UTF8ToUTF16(password.display_name());
  new_password.icon_url = GURL(password.avatar_url());
  new_password.federation_origin = url::Origin(GURL(password.federation_url()));
  return new_password;
}

std::string MakePasswordSyncTag(
    const sync_pb::PasswordSpecificsData& password) {
  return MakePasswordSyncTag(PasswordFromSpecifics(password));
}

std::string MakePasswordSyncTag(const autofill::PasswordForm& password) {
  return (net::EscapePath(password.origin.spec()) + "|" +
          net::EscapePath(base::UTF16ToUTF8(password.username_element)) + "|" +
          net::EscapePath(base::UTF16ToUTF8(password.username_value)) + "|" +
          net::EscapePath(base::UTF16ToUTF8(password.password_element)) + "|" +
          net::EscapePath(password.signon_realm));
}

}  // namespace password_manager
