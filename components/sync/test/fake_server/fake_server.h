// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_H_
#define COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/files/scoped_temp_dir.h"
#include "base/observer_list.h"
#include "base/threading/thread_checker.h"
#include "base/values.h"
#include "components/sync/base/model_type.h"
#include "components/sync/engine_impl/loopback_server/loopback_server.h"
#include "components/sync/engine_impl/loopback_server/loopback_server_entity.h"
#include "components/sync/engine_impl/loopback_server/persistent_bookmark_entity.h"
#include "components/sync/engine_impl/loopback_server/persistent_tombstone_entity.h"
#include "components/sync/engine_impl/loopback_server/persistent_unique_client_entity.h"
#include "components/sync/protocol/sync.pb.h"

namespace fake_server {

// A fake version of the Sync server used for testing. This class is not thread
// safe.
class FakeServer : public syncer::LoopbackServer::ObserverForTests {
 public:
  class Observer {
   public:
    virtual ~Observer() {}

    // Called after FakeServer has processed a successful commit. The types
    // updated as part of the commit are passed in |committed_model_types|.
    virtual void OnCommit(const std::string& committer_id,
                          syncer::ModelTypeSet committed_model_types) = 0;
  };

  FakeServer();
  ~FakeServer() override;

  // Handles a /command POST (with the given |request|) to the server. Three
  // output arguments, |error_code|, |response_code|, and |response|, are used
  // to pass data back to the caller. The command has failed if the value
  // pointed to by |error_code| is nonzero. |completion_closure| will be called
  // immediately before return.
  void HandleCommand(const std::string& request,
                     const base::Closure& completion_closure,
                     int* error_code,
                     int* response_code,
                     std::string* response);

  // Helpers for fetching the last Commit or GetUpdates messages, respectively.
  // Returns true if the specified message existed, and false if no message has
  // been received.
  bool GetLastCommitMessage(sync_pb::ClientToServerMessage* message);
  bool GetLastGetUpdatesMessage(sync_pb::ClientToServerMessage* message);

  // Creates a DicionaryValue representation of all entities present in the
  // server. The dictionary keys are the strings generated by ModelTypeToString
  // and the values are ListValues containing StringValue versions of entity
  // names.
  std::unique_ptr<base::DictionaryValue> GetEntitiesAsDictionaryValue();

  // Returns all entities stored by the server of the given |model_type|.
  // This method returns SyncEntity protocol buffer objects (instead of
  // LoopbackServerEntity) so that callers can inspect datatype-specific data
  // (e.g., the URL of a session tab).
  std::vector<sync_pb::SyncEntity> GetSyncEntitiesByModelType(
      syncer::ModelType model_type);

  // Adds |entity| to the server's collection of entities. This method makes no
  // guarantees that the added entity will result in successful server
  // operations.
  void InjectEntity(std::unique_ptr<syncer::LoopbackServerEntity> entity);

  // Modifies the entity on the server with the given |id|. The entity's
  // EntitySpecifics are replaced with |updated_specifics| and its version is
  // updated. If the given |id| does not exist or the ModelType of
  // |updated_specifics| does not match the entity, false is returned.
  // Otherwise, true is returned to represent a successful modification.
  //
  // This method sometimes updates entity data beyond EntitySpecifics. For
  // example, in the case of a bookmark, changing the BookmarkSpecifics title
  // field will modify the top-level entity's name field.
  bool ModifyEntitySpecifics(const std::string& id,
                             const sync_pb::EntitySpecifics& updated_specifics);

  bool ModifyBookmarkEntity(const std::string& id,
                            const std::string& parent_id,
                            const sync_pb::EntitySpecifics& updated_specifics);

  // Clears server data simulating a "dashboard stop and clear" and sets a new
  // store birthday.
  void ClearServerData();

  // Puts the server in a state where it acts as if authentication has
  // succeeded.
  void SetAuthenticated();

  // Puts the server in a state where all commands will fail with an
  // authentication error.
  void SetUnauthenticated();

  // Force the server to return |error_type| in the error_code field of
  // ClientToServerResponse on all subsequent sync requests. This method should
  // not be called if TriggerActionableError has previously been called. Returns
  // true if error triggering was successfully configured.
  bool TriggerError(const sync_pb::SyncEnums::ErrorType& error_type);

  // Force the server to return the given data as part of the error field of
  // ClientToServerResponse on all subsequent sync requests. This method should
  // not be called if TriggerError has previously been called. Returns true if
  // error triggering was successfully configured.
  bool TriggerActionableError(const sync_pb::SyncEnums::ErrorType& error_type,
                              const std::string& description,
                              const std::string& url,
                              const sync_pb::SyncEnums::Action& action);

  // Instructs the server to send triggered errors on every other request
  // (starting with the first one after this call). This feature can be used to
  // test the resiliency of the client when communicating with a problematic
  // server or flaky network connection. This method should only be called
  // after a call to TriggerError or TriggerActionableError. Returns true if
  // triggered error alternating was successful.
  bool EnableAlternatingTriggeredErrors();

  // Adds |observer| to FakeServer's observer list. This should be called
  // before the Profile associated with |observer| is connected to the server.
  void AddObserver(Observer* observer);

  // Removes |observer| from the FakeServer's observer list. This method
  // must be called if AddObserver was ever called with |observer|.
  void RemoveObserver(Observer* observer);

  // Undoes the effects of DisableNetwork.
  void EnableNetwork();

  // Forces every request to fail in a way that simulates a network failure.
  // This can be used to trigger exponential backoff in the client.
  void DisableNetwork();

  // Implement LoopbackServer::ObserverForTests:
  void OnCommit(const std::string& committer_id,
                syncer::ModelTypeSet committed_model_types) override;

  // Returns the current FakeServer as a WeakPtr.
  base::WeakPtr<FakeServer> AsWeakPtr();

  // Use this callback to generate response types for entities. They will still
  // be "committed" and stored as normal, this only affects the response type
  // the client sees. This allows tests to still inspect what the client has
  // done, although not as useful of a mechanism for multi client tests. Care
  // should be taken when failing responses, as the client will go into
  // exponential backoff, which can cause tests to be slow or time out.
  void OverrideResponseType(
      syncer::LoopbackServer::ResponseTypeProvider response_type_override);

 private:
  // Returns whether a triggered error should be sent for the request.
  bool ShouldSendTriggeredError() const;

  // Whether the server should act as if incoming connections are properly
  // authenticated.
  bool authenticated_;

  // All Keystore keys known to the server.
  std::vector<std::string> keystore_keys_;

  // Used as the error_code field of ClientToServerResponse on all responses
  // except when |triggered_actionable_error_| is set.
  sync_pb::SyncEnums::ErrorType error_type_;

  // Used as the error field of ClientToServerResponse when its pointer is not
  // null.
  std::unique_ptr<sync_pb::ClientToServerResponse_Error>
      triggered_actionable_error_;

  // These values are used in tandem to return a triggered error (either
  // |error_type_| or |triggered_actionable_error_|) on every other request.
  // |alternate_triggered_errors_| is set if this feature is enabled and
  // |request_counter_| is used to send triggered errors on odd-numbered
  // requests. Note that |request_counter_| can be reset and is not necessarily
  // indicative of the total number of requests handled during the object's
  // lifetime.
  bool alternate_triggered_errors_;
  int request_counter_;

  // FakeServer's observers.
  base::ObserverList<Observer, true> observers_;

  // When true, the server operates normally. When false, a failure is returned
  // on every request. This is used to simulate a network failure on the client.
  bool network_enabled_;

  // The last received client to server messages.
  sync_pb::ClientToServerMessage last_commit_message_;
  sync_pb::ClientToServerMessage last_getupdates_message_;

  // Used to verify that FakeServer is only used from one thread.
  base::ThreadChecker thread_checker_;

  std::unique_ptr<syncer::LoopbackServer> loopback_server_;
  std::unique_ptr<base::ScopedTempDir> loopback_server_storage_;

  // Creates WeakPtr versions of the current FakeServer. This must be the last
  // data member!
  base::WeakPtrFactory<FakeServer> weak_ptr_factory_;
};

}  // namespace fake_server

#endif  // COMPONENTS_SYNC_TEST_FAKE_SERVER_FAKE_SERVER_H_
