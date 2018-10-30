// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/offline_pages/core/background/request_coordinator.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/containers/circular_deque.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/synchronization/waitable_event.h"
#include "base/sys_info.h"
#include "base/test/histogram_tester.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/test_mock_time_task_runner.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/time/time.h"
#include "components/offline_pages/core/background/device_conditions.h"
#include "components/offline_pages/core/background/network_quality_provider_stub.h"
#include "components/offline_pages/core/background/offliner.h"
#include "components/offline_pages/core/background/offliner_policy.h"
#include "components/offline_pages/core/background/offliner_stub.h"
#include "components/offline_pages/core/background/request_coordinator_stub_taco.h"
#include "components/offline_pages/core/background/request_queue.h"
#include "components/offline_pages/core/background/request_queue_in_memory_store.h"
#include "components/offline_pages/core/background/save_page_request.h"
#include "components/offline_pages/core/background/scheduler.h"
#include "components/offline_pages/core/background/scheduler_stub.h"
#include "components/offline_pages/core/offline_page_feature.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace offline_pages {

namespace {
// put test constants here
const GURL kUrl1("http://universe.com/everything");
const GURL kUrl2("http://universe.com/toinfinityandbeyond");
const std::string kClientNamespace("bookmark");
const std::string kId1("42");
const std::string kId2("life*universe+everything");
const ClientId kClientId1(kClientNamespace, kId1);
const ClientId kClientId2(kClientNamespace, kId2);
const int kRequestId1(1);
const int kRequestId2(2);
const long kTestTimeBudgetSeconds = 200;
const int kBatteryPercentageHigh = 75;
const int kMaxCompletedTries = 3;
const bool kPowerRequired = true;
const bool kUserRequested = true;
const int kAttemptCount = 1;
const std::string kRequestOrigin("abc.xyz");
}  // namespace

class ObserverStub : public RequestCoordinator::Observer {
 public:
  ObserverStub()
      : added_called_(false),
        completed_called_(false),
        changed_called_(false),
        network_progress_called_(false),
        network_progress_bytes_(0),
        last_status_(RequestCoordinator::BackgroundSavePageResult::SUCCESS),
        state_(SavePageRequest::RequestState::OFFLINING) {}

  void Clear() {
    added_called_ = false;
    completed_called_ = false;
    changed_called_ = false;
    network_progress_called_ = false;
    network_progress_bytes_ = 0;
    state_ = SavePageRequest::RequestState::OFFLINING;
    last_status_ = RequestCoordinator::BackgroundSavePageResult::SUCCESS;
  }

  void OnAdded(const SavePageRequest& request) override {
    added_called_ = true;
  }

  void OnCompleted(
      const SavePageRequest& request,
      RequestCoordinator::BackgroundSavePageResult status) override {
    completed_called_ = true;
    last_status_ = status;
  }

  void OnChanged(const SavePageRequest& request) override {
    changed_called_ = true;
    state_ = request.request_state();
  }

  void OnNetworkProgress(const SavePageRequest& request,
                         int64_t received_bytes) override {
    network_progress_called_ = true;
    network_progress_bytes_ = received_bytes;
  }

  bool added_called() { return added_called_; }
  bool completed_called() { return completed_called_; }
  bool changed_called() { return changed_called_; }
  bool network_progress_called() { return network_progress_called_; }
  int64_t network_progress_bytes() { return network_progress_bytes_; }
  RequestCoordinator::BackgroundSavePageResult last_status() {
    return last_status_;
  }
  SavePageRequest::RequestState state() { return state_; }

 private:
  bool added_called_;
  bool completed_called_;
  bool changed_called_;
  bool network_progress_called_;
  int64_t network_progress_bytes_;
  RequestCoordinator::BackgroundSavePageResult last_status_;
  SavePageRequest::RequestState state_;
};

class RequestCoordinatorTest : public testing::Test {
 public:
  using RequestCoordinatorState = RequestCoordinator::RequestCoordinatorState;

  RequestCoordinatorTest();
  ~RequestCoordinatorTest() override;

  void SetUp() override;

  void PumpLoop();

  RequestCoordinator* coordinator() const {
    return coordinator_taco_->request_coordinator();
  }

  RequestCoordinatorState state() { return coordinator()->state(); }

  // Test processing callback function.
  void ProcessingCallbackFunction(bool result) {
    processing_callback_called_ = true;
    processing_callback_result_ = result;
  }

  // Callback function which releases a wait for it.
  void WaitingCallbackFunction(bool result) { waiter_.Signal(); }

  net::NetworkChangeNotifier::ConnectionType GetConnectionType() {
    return coordinator()->current_conditions_->GetNetConnectionType();
  }

  // Callback for Add requests.
  void AddRequestDone(AddRequestResult result, const SavePageRequest& request);

  // Callback for getting requests.
  void GetRequestsDone(GetRequestsResult result,
                       std::vector<std::unique_ptr<SavePageRequest>> requests);

  // Callback for removing requests.
  void RemoveRequestsDone(const MultipleItemStatuses& results);

  // Callback for getting request statuses.
  void GetQueuedRequestsDone(
      std::vector<std::unique_ptr<SavePageRequest>> requests);

  void SetupForOfflinerDoneCallbackTest(
      offline_pages::SavePageRequest* request);

  void SendOfflinerDoneCallback(const SavePageRequest& request,
                                Offliner::RequestStatus status);

  GetRequestsResult last_get_requests_result() const {
    return last_get_requests_result_;
  }

  const std::vector<std::unique_ptr<SavePageRequest>>& last_requests() const {
    return last_requests_;
  }

  const MultipleItemStatuses& last_remove_results() const {
    return last_remove_results_;
  }

  void DisableLoading() { offliner_->disable_loading(); }

  void EnableOfflinerCallback(bool enable) {
    offliner_->enable_callback(enable);
  }

  void EnableSnapshotOnLastRetry() {
    offliner_->enable_snapshot_on_last_retry();
  }

  void SetEffectiveConnectionTypeForTest(net::EffectiveConnectionType type) {
    network_quality_provider_->SetEffectiveConnectionTypeForTest(type);
  }

  void SetNetworkConnected(bool connected) {
    if (connected) {
      DeviceConditions device_conditions(
          !kPowerRequired, kBatteryPercentageHigh,
          net::NetworkChangeNotifier::CONNECTION_3G);
      SetDeviceConditionsForTest(device_conditions);
      SetEffectiveConnectionTypeForTest(
          net::EffectiveConnectionType::EFFECTIVE_CONNECTION_TYPE_3G);
    } else {
      DeviceConditions device_conditions(
          !kPowerRequired, kBatteryPercentageHigh,
          net::NetworkChangeNotifier::CONNECTION_NONE);
      SetDeviceConditionsForTest(device_conditions);
      SetEffectiveConnectionTypeForTest(
          net::EffectiveConnectionType::EFFECTIVE_CONNECTION_TYPE_OFFLINE);
    }
  }

  void CallConnectionTypeObserver() {
    if (coordinator()->connection_notifier_) {
      coordinator()->connection_notifier_->OnConnectionTypeChanged(
          GetConnectionType());
    }
  }

  void SetIsLowEndDeviceForTest(bool is_low_end_device) {
    coordinator()->is_low_end_device_ = is_low_end_device;
  }

  void SetProcessingStateForTest(
      RequestCoordinator::ProcessingWindowState processing_state) {
    coordinator()->processing_state_ = processing_state;
  }

  void SetOperationStartTimeForTest(base::Time start_time) {
    coordinator()->operation_start_time_ = start_time;
  }

  void ScheduleForTest() { coordinator()->ScheduleAsNeeded(); }

  void CallRequestNotPicked(bool non_user_requested_tasks_remaining,
                            bool disabled_tasks_remaining) {
    if (disabled_tasks_remaining)
      coordinator()->disabled_requests_.insert(kRequestId1);
    else
      coordinator()->disabled_requests_.clear();

    coordinator()->RequestNotPicked(non_user_requested_tasks_remaining, false);
  }

  void SetDeviceConditionsForTest(DeviceConditions device_conditions) {
    coordinator()->SetDeviceConditionsForTest(device_conditions);
  }

  DeviceConditions* GetDeviceConditions() {
    return coordinator()->current_conditions_.get();
  }

  void WaitForCallback() { waiter_.Wait(); }

  void AdvanceClockBy(base::TimeDelta delta) {
    task_runner_->FastForwardBy(delta);
  }

  SavePageRequest AddRequest1();

  SavePageRequest AddRequest2();

  int64_t SavePageLater() {
    RequestCoordinator::SavePageLaterParams params;
    params.url = kUrl1;
    params.client_id = kClientId1;
    params.user_requested = kUserRequested;
    params.request_origin = kRequestOrigin;
    return coordinator()->SavePageLater(params);
  }

  int64_t SavePageLaterWithAvailability(
      RequestCoordinator::RequestAvailability availability) {
    RequestCoordinator::SavePageLaterParams params;
    params.url = kUrl1;
    params.client_id = kClientId1;
    params.user_requested = kUserRequested;
    params.availability = availability;
    params.request_origin = kRequestOrigin;
    return coordinator()->SavePageLater(params);
  }

  Offliner::RequestStatus last_offlining_status() const {
    return coordinator()->last_offlining_status_;
  }

  bool OfflinerWasCanceled() const { return offliner_->cancel_called(); }

  void ResetOfflinerWasCanceled() { offliner_->reset_cancel_called(); }

  ObserverStub observer() { return observer_; }

  DeviceConditions device_conditions() { return device_conditions_; }

  base::Callback<void(bool)> processing_callback() {
    return processing_callback_;
  }

  base::Callback<void(bool)> waiting_callback() { return waiting_callback_; }
  bool processing_callback_called() const {
    return processing_callback_called_;
  }

  bool processing_callback_result() const {
    return processing_callback_result_;
  }

  const base::HistogramTester& histograms() const { return histogram_tester_; }

  const std::set<int64_t>& disabled_requests() {
    return coordinator()->disabled_requests_;
  }

  const base::circular_deque<int64_t>& prioritized_requests() {
    return coordinator()->prioritized_requests_;
  }

 private:
  GetRequestsResult last_get_requests_result_;
  MultipleItemStatuses last_remove_results_;
  std::vector<std::unique_ptr<SavePageRequest>> last_requests_;
  scoped_refptr<base::TestMockTimeTaskRunner> task_runner_;
  base::ThreadTaskRunnerHandle task_runner_handle_;
  NetworkQualityProviderStub* network_quality_provider_;
  std::unique_ptr<RequestCoordinatorStubTaco> coordinator_taco_;
  OfflinerStub* offliner_;
  base::WaitableEvent waiter_;
  ObserverStub observer_;
  bool processing_callback_called_;
  bool processing_callback_result_;
  DeviceConditions device_conditions_;
  base::Callback<void(bool)> processing_callback_;
  base::Callback<void(bool)> waiting_callback_;
  base::HistogramTester histogram_tester_;
};

RequestCoordinatorTest::RequestCoordinatorTest()
    : last_get_requests_result_(GetRequestsResult::STORE_FAILURE),
      task_runner_(new base::TestMockTimeTaskRunner),
      task_runner_handle_(task_runner_),
      offliner_(nullptr),
      waiter_(base::WaitableEvent::ResetPolicy::MANUAL,
              base::WaitableEvent::InitialState::NOT_SIGNALED),
      processing_callback_called_(false),
      processing_callback_result_(false),
      device_conditions_(!kPowerRequired,
                         kBatteryPercentageHigh,
                         net::NetworkChangeNotifier::CONNECTION_3G) {}

RequestCoordinatorTest::~RequestCoordinatorTest() {}

void RequestCoordinatorTest::SetUp() {
  coordinator_taco_ = base::MakeUnique<RequestCoordinatorStubTaco>();

  std::unique_ptr<OfflinerStub> offliner(new OfflinerStub());
  // Save raw pointer for use by the tests.
  offliner_ = offliner.get();
  coordinator_taco_->SetOffliner(std::move(offliner));

  std::unique_ptr<NetworkQualityProviderStub> network_quality_provider =
      base::MakeUnique<NetworkQualityProviderStub>();
  // Save raw pointer for use by the tests.
  network_quality_provider_ = network_quality_provider.get();
  coordinator_taco_->SetNetworkQualityProvider(
      std::move(network_quality_provider));

  coordinator_taco_->CreateRequestCoordinator();

  coordinator()->AddObserver(&observer_);
  SetNetworkConnected(true);
  processing_callback_ =
      base::Bind(&RequestCoordinatorTest::ProcessingCallbackFunction,
                 base::Unretained(this));
  // Override the normal immediate callback with a wait releasing callback.
  waiting_callback_ = base::Bind(
      &RequestCoordinatorTest::WaitingCallbackFunction, base::Unretained(this));
  SetDeviceConditionsForTest(device_conditions_);
  // Ensure not low-end device so immediate start can happen for most tests.
  SetIsLowEndDeviceForTest(false);
  EnableOfflinerCallback(true);
}

void RequestCoordinatorTest::PumpLoop() {
  task_runner_->RunUntilIdle();
}

void RequestCoordinatorTest::GetRequestsDone(
    GetRequestsResult result,
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  last_get_requests_result_ = result;
  last_requests_ = std::move(requests);
}

void RequestCoordinatorTest::RemoveRequestsDone(
    const MultipleItemStatuses& results) {
  last_remove_results_ = results;
  waiter_.Signal();
}

void RequestCoordinatorTest::GetQueuedRequestsDone(
    std::vector<std::unique_ptr<SavePageRequest>> requests) {
  last_requests_ = std::move(requests);
  waiter_.Signal();
}

void RequestCoordinatorTest::AddRequestDone(AddRequestResult result,
                                            const SavePageRequest& request) {}

void RequestCoordinatorTest::SetupForOfflinerDoneCallbackTest(
    offline_pages::SavePageRequest* request) {
  // Mark request as started and add it to the queue,
  // then wait for callback to finish.
  request->MarkAttemptStarted(base::Time::Now());
  coordinator()->queue()->AddRequest(
      *request, base::Bind(&RequestCoordinatorTest::AddRequestDone,
                           base::Unretained(this)));
  PumpLoop();

  // Override the processing callback for test visiblity.
  base::Callback<void(bool)> callback =
      base::Bind(&RequestCoordinatorTest::ProcessingCallbackFunction,
                 base::Unretained(this));
  coordinator()->SetProcessingCallbackForTest(callback);

  // Mock that coordinator is in actively processing state starting now.
  SetProcessingStateForTest(
      RequestCoordinator::ProcessingWindowState::IMMEDIATE_WINDOW);
  SetOperationStartTimeForTest(base::Time::Now());
}

void RequestCoordinatorTest::SendOfflinerDoneCallback(
    const SavePageRequest& request,
    Offliner::RequestStatus status) {
  // Using the fact that the test class is a friend, call to the callback
  coordinator()->OfflinerDoneCallback(request, status);
}

SavePageRequest RequestCoordinatorTest::AddRequest1() {
  offline_pages::SavePageRequest request1(kRequestId1, kUrl1, kClientId1,
                                          base::Time::Now(), kUserRequested);
  coordinator()->queue()->AddRequest(
      request1, base::Bind(&RequestCoordinatorTest::AddRequestDone,
                           base::Unretained(this)));
  return request1;
}

SavePageRequest RequestCoordinatorTest::AddRequest2() {
  offline_pages::SavePageRequest request2(kRequestId2, kUrl2, kClientId2,
                                          base::Time::Now(), kUserRequested);
  coordinator()->queue()->AddRequest(
      request2, base::Bind(&RequestCoordinatorTest::AddRequestDone,
                           base::Unretained(this)));
  return request2;
}

TEST_F(RequestCoordinatorTest, StartScheduledProcessingWithNoRequests) {
  // Set low-end device status to actual status.
  SetIsLowEndDeviceForTest(base::SysInfo::IsLowEndDevice());

  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      processing_callback()));
  PumpLoop();

  EXPECT_TRUE(processing_callback_called());

  // Verify queue depth UMA for starting scheduled processing on empty queue.
  if (base::SysInfo::IsLowEndDevice()) {
    histograms().ExpectBucketCount(
        "OfflinePages.Background.ScheduledStart.AvailableRequestCount.Svelte",
        0, 1);
  } else {
    histograms().ExpectBucketCount(
        "OfflinePages.Background.ScheduledStart.AvailableRequestCount", 0, 1);
  }
}

TEST_F(RequestCoordinatorTest, NetworkProgressCallback) {
  EXPECT_NE(0, SavePageLater());
  PumpLoop();
  EXPECT_TRUE(observer().network_progress_called());
  EXPECT_GT(observer().network_progress_bytes(), 0LL);
}

TEST_F(RequestCoordinatorTest, StartScheduledProcessingWithRequestInProgress) {
  // Start processing for this request.
  EXPECT_NE(0, SavePageLater());

  // Ensure that the forthcoming request does not finish - we simulate it being
  // in progress by asking it to skip making the completion callback.
  EnableOfflinerCallback(false);

  // Sending the request to the offliner should make it busy.
  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      processing_callback()));
  PumpLoop();

  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
  // Since the offliner is disabled, this callback should not be called.
  EXPECT_FALSE(processing_callback_called());

  // Now trying to start processing should return false since already busy.
  EXPECT_FALSE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                       processing_callback()));
}

TEST_F(RequestCoordinatorTest, StartImmediateProcessingWithNoRequests) {
  EXPECT_TRUE(coordinator()->StartImmediateProcessing(processing_callback()));
  PumpLoop();

  EXPECT_TRUE(processing_callback_called());

  histograms().ExpectBucketCount("OfflinePages.Background.ImmediateStartStatus",
                                 0 /* STARTED */, 1);
}

TEST_F(RequestCoordinatorTest, StartImmediateProcessingOnSvelte) {
  // Set as low-end device to verfiy immediate processing will not start.
  SetIsLowEndDeviceForTest(true);

  EXPECT_FALSE(coordinator()->StartImmediateProcessing(processing_callback()));
  histograms().ExpectBucketCount("OfflinePages.Background.ImmediateStartStatus",
                                 5 /* NOT_STARTED_ON_SVELTE */, 1);
}

TEST_F(RequestCoordinatorTest, StartImmediateProcessingWhenDisconnected) {
  DeviceConditions disconnected_conditions(
      !kPowerRequired, kBatteryPercentageHigh,
      net::NetworkChangeNotifier::CONNECTION_NONE);
  SetDeviceConditionsForTest(disconnected_conditions);
  EXPECT_FALSE(coordinator()->StartImmediateProcessing(processing_callback()));
  histograms().ExpectBucketCount("OfflinePages.Background.ImmediateStartStatus",
                                 3 /* NO_CONNECTION */, 1);
}

TEST_F(RequestCoordinatorTest, StartImmediateProcessingWithRequestInProgress) {
  // Start processing for this request.
  EXPECT_NE(0, SavePageLater());

  // Disable the automatic offliner callback.
  EnableOfflinerCallback(false);

  // Sending the request to the offliner should make it busy.
  EXPECT_TRUE(coordinator()->StartImmediateProcessing(processing_callback()));
  PumpLoop();

  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
  // Since the offliner is disabled, this callback should not be called.
  EXPECT_FALSE(processing_callback_called());

  // Now trying to start processing should return false since already busy.
  EXPECT_FALSE(coordinator()->StartImmediateProcessing(processing_callback()));

  histograms().ExpectBucketCount("OfflinePages.Background.ImmediateStartStatus",
                                 1 /* BUSY */, 1);
}

TEST_F(RequestCoordinatorTest, SavePageLater) {
  // The user-requested request which gets processed by SavePageLater
  // would invoke user request callback.
  coordinator()->SetInternalStartProcessingCallbackForTest(
      processing_callback());

  // Use default values for |user_requested| and |availability|.
  RequestCoordinator::SavePageLaterParams params;
  params.url = kUrl1;
  params.client_id = kClientId1;
  params.original_url = kUrl2;
  params.request_origin = kRequestOrigin;
  EXPECT_NE(0, coordinator()->SavePageLater(params));

  // Expect that a request got placed on the queue.
  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));

  // Expect that the request is not added to the disabled list by default.
  EXPECT_TRUE(disabled_requests().empty());

  // Wait for callbacks to finish, both request queue and offliner.
  PumpLoop();
  EXPECT_TRUE(processing_callback_called());

  // Check the request queue is as expected.
  ASSERT_EQ(1UL, last_requests().size());
  EXPECT_EQ(kUrl1, last_requests().at(0)->url());
  EXPECT_EQ(kClientId1, last_requests().at(0)->client_id());
  EXPECT_TRUE(last_requests().at(0)->user_requested());
  EXPECT_EQ(kUrl2, last_requests().at(0)->original_url());
  EXPECT_EQ(kRequestOrigin, last_requests().at(0)->request_origin());

  // Expect that the scheduler got notified.
  SchedulerStub* scheduler_stub =
      reinterpret_cast<SchedulerStub*>(coordinator()->scheduler());
  EXPECT_TRUE(scheduler_stub->schedule_called());
  EXPECT_EQ(coordinator()
                ->GetTriggerConditions(last_requests()[0]->user_requested())
                .minimum_battery_percentage,
            scheduler_stub->trigger_conditions()->minimum_battery_percentage);

  // Check that the observer got the notification that a page is available
  EXPECT_TRUE(observer().added_called());

  // Verify queue depth UMA for starting immediate processing.
  histograms().ExpectBucketCount(
      "OfflinePages.Background.ImmediateStart.AvailableRequestCount", 1, 1);
}

TEST_F(RequestCoordinatorTest, SavePageLaterFailed) {
  // Set low-end device status to actual status.
  SetIsLowEndDeviceForTest(base::SysInfo::IsLowEndDevice());

  // The user-requested request which gets processed by SavePageLater
  // would invoke user request callback.
  coordinator()->SetInternalStartProcessingCallbackForTest(
      processing_callback());

  EXPECT_NE(0, SavePageLater());

  // Expect that a request got placed on the queue.
  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));

  // Wait for callbacks to finish, both request queue and offliner.
  PumpLoop();

  // On low-end devices the callback will be called with false since the
  // processing started but failed due to svelte devices.
  EXPECT_TRUE(processing_callback_called());
  if (base::SysInfo::IsLowEndDevice()) {
    EXPECT_FALSE(processing_callback_result());
  } else {
    EXPECT_TRUE(processing_callback_result());
  }

  // Check the request queue is as expected.
  EXPECT_EQ(1UL, last_requests().size());
  EXPECT_EQ(kUrl1, last_requests().at(0)->url());
  EXPECT_EQ(kClientId1, last_requests().at(0)->client_id());

  // Expect that the scheduler got notified.
  SchedulerStub* scheduler_stub =
      reinterpret_cast<SchedulerStub*>(coordinator()->scheduler());
  EXPECT_TRUE(scheduler_stub->schedule_called());
  EXPECT_EQ(coordinator()
                ->GetTriggerConditions(last_requests()[0]->user_requested())
                .minimum_battery_percentage,
            scheduler_stub->trigger_conditions()->minimum_battery_percentage);

  // Check that the observer got the notification that a page is available
  EXPECT_TRUE(observer().added_called());
}

TEST_F(RequestCoordinatorTest, OfflinerDoneRequestSucceeded) {
  // Add a request to the queue, wait for callbacks to finish.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  SetupForOfflinerDoneCallbackTest(&request);

  // Call the OfflinerDoneCallback to simulate the page being completed, wait
  // for callbacks.
  SendOfflinerDoneCallback(request, Offliner::RequestStatus::SAVED);
  PumpLoop();
  EXPECT_TRUE(processing_callback_called());

  // Verify the request gets removed from the queue, and wait for callbacks.
  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // We should not find any requests in the queue anymore.
  // RequestPicker should *not* have tried to start an additional job,
  // because the request queue is empty now.
  EXPECT_EQ(0UL, last_requests().size());
  // Check that the observer got the notification that we succeeded, and that
  // the request got removed from the queue.
  EXPECT_TRUE(observer().completed_called());
  histograms().ExpectBucketCount(
      "OfflinePages.Background.FinalSavePageResult.bookmark", 0 /* SUCCESS */,
      1);
  EXPECT_EQ(RequestCoordinator::BackgroundSavePageResult::SUCCESS,
            observer().last_status());
}

TEST_F(RequestCoordinatorTest, OfflinerDoneRequestSucceededButLostNetwork) {
  // Add a request to the queue and set offliner done callback for it.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  SetupForOfflinerDoneCallbackTest(&request);
  EnableOfflinerCallback(false);

  // Add a 2nd request to the queue.
  AddRequest2();

  // Disconnect network.
  SetNetworkConnected(false);

  // Call the OfflinerDoneCallback to simulate the page being completed, wait
  // for callbacks.
  SendOfflinerDoneCallback(request, Offliner::RequestStatus::SAVED);
  PumpLoop();
  EXPECT_TRUE(processing_callback_called());

  // Verify not busy with 2nd request (since no connection).
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);

  // Now connect network and verify processing starts.
  SetNetworkConnected(true);
  CallConnectionTypeObserver();
  PumpLoop();
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
}

TEST_F(RequestCoordinatorTest, OfflinerDoneRequestFailed) {
  // Add a request to the queue, wait for callbacks to finish.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  request.set_completed_attempt_count(kMaxCompletedTries - 1);
  SetupForOfflinerDoneCallbackTest(&request);
  // Stop processing before completing the second request on the queue.
  EnableOfflinerCallback(false);

  // Add second request to the queue to check handling when first fails.
  AddRequest2();
  PumpLoop();

  // Call the OfflinerDoneCallback to simulate the request failed, wait
  // for callbacks.
  SendOfflinerDoneCallback(request, Offliner::RequestStatus::LOADING_FAILED);
  PumpLoop();

  // For retriable failure, processing should continue to 2nd request so
  // no scheduler callback yet.
  EXPECT_FALSE(processing_callback_called());

  // Busy processing 2nd request.
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);

  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // Now just one request in the queue since failed request removed
  // (max number of attempts exceeded).
  EXPECT_EQ(1UL, last_requests().size());
  // Check that the observer got the notification that we failed (and the
  // subsequent notification that the request was removed) since we exceeded
  // retry count.
  EXPECT_TRUE(observer().completed_called());
  histograms().ExpectBucketCount(
      "OfflinePages.Background.FinalSavePageResult.bookmark",
      6 /* RETRY_COUNT_EXCEEDED */, 1);
  EXPECT_EQ(RequestCoordinator::BackgroundSavePageResult::RETRY_COUNT_EXCEEDED,
            observer().last_status());
}

TEST_F(RequestCoordinatorTest, OfflinerDoneRequestFailedNoRetryFailure) {
  // Add a request to the queue, wait for callbacks to finish.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  SetupForOfflinerDoneCallbackTest(&request);
  EnableOfflinerCallback(false);

  // Add second request to the queue to check handling when first fails.
  AddRequest2();
  PumpLoop();

  // Call the OfflinerDoneCallback to simulate the request failed, wait
  // for callbacks.
  SendOfflinerDoneCallback(request,
                           Offliner::RequestStatus::LOADING_FAILED_NO_RETRY);
  PumpLoop();

  // For no retry failure, processing should continue to 2nd request so
  // no scheduler callback yet.
  EXPECT_FALSE(processing_callback_called());

  // Busy processing 2nd request.
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);

  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // Now just one request in the queue since non-retryable failure.
  EXPECT_EQ(1UL, last_requests().size());
  // Check that the observer got the notification that we failed (and the
  // subsequent notification that the request was removed).
  EXPECT_TRUE(observer().completed_called());
  EXPECT_EQ(RequestCoordinator::BackgroundSavePageResult::LOADING_FAILURE,
            observer().last_status());
  // We should have a histogram entry for the effective network conditions
  // when this failed request began.
  histograms().ExpectBucketCount(
      "OfflinePages.Background.EffectiveConnectionType.OffliningStartType."
      "bookmark",
      net::NetworkChangeNotifier::CONNECTION_UNKNOWN, 1);
}

TEST_F(RequestCoordinatorTest, OfflinerDoneRequestFailedNoNextFailure) {
  // Add a request to the queue, wait for callbacks to finish.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  SetupForOfflinerDoneCallbackTest(&request);
  EnableOfflinerCallback(false);

  // Add second request to the queue to check handling when first fails.
  AddRequest2();
  PumpLoop();

  // Call the OfflinerDoneCallback to simulate the request failed, wait
  // for callbacks.
  SendOfflinerDoneCallback(request,
                           Offliner::RequestStatus::LOADING_FAILED_NO_NEXT);
  PumpLoop();

  // For no next failure, processing should not continue to 2nd request so
  // expect scheduler callback.
  EXPECT_TRUE(processing_callback_called());

  // Not busy for NO_NEXT failure.
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);

  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // Both requests still in queue.
  EXPECT_EQ(2UL, last_requests().size());
}

TEST_F(RequestCoordinatorTest, OfflinerDoneForegroundCancel) {
  // Add a request to the queue, wait for callbacks to finish.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  SetupForOfflinerDoneCallbackTest(&request);

  // Call the OfflinerDoneCallback to simulate the request failed, wait
  // for callbacks.
  SendOfflinerDoneCallback(request,
                           Offliner::RequestStatus::FOREGROUND_CANCELED);
  PumpLoop();
  EXPECT_TRUE(processing_callback_called());

  // Verify the request is not removed from the queue, and wait for callbacks.
  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // Request no longer in the queue (for single attempt policy).
  EXPECT_EQ(1UL, last_requests().size());
  // Verify foreground cancel not counted as an attempt after all.
  EXPECT_EQ(0L, last_requests().at(0)->completed_attempt_count());
}

TEST_F(RequestCoordinatorTest, OfflinerDoneOffliningCancel) {
  // Add a request to the queue, wait for callbacks to finish.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  SetupForOfflinerDoneCallbackTest(&request);

  // Call the OfflinerDoneCallback to simulate the request failed, wait
  // for callbacks.
  SendOfflinerDoneCallback(request, Offliner::RequestStatus::LOADING_CANCELED);
  PumpLoop();
  EXPECT_TRUE(processing_callback_called());

  // Verify the request is not removed from the queue, and wait for callbacks.
  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // Request still in the queue.
  EXPECT_EQ(1UL, last_requests().size());
  // Verify offlining cancel not counted as an attempt after all.
  const std::unique_ptr<SavePageRequest>& found_request =
      last_requests().front();
  EXPECT_EQ(0L, found_request->completed_attempt_count());
}

// If one item completes, and there are no more user requeted items left,
// we should make a scheduler entry for a non-user requested item.
TEST_F(RequestCoordinatorTest, RequestNotPickedDisabledItemsRemain) {
  coordinator()->StartScheduledProcessing(device_conditions(),
                                          processing_callback());
  EXPECT_TRUE(state() == RequestCoordinatorState::PICKING);

  // Call RequestNotPicked, simulating a request on the disabled list.
  CallRequestNotPicked(false, true);
  PumpLoop();

  EXPECT_FALSE(state() == RequestCoordinatorState::PICKING);

  // The scheduler should have been called to schedule the disabled task for
  // 5 minutes from now.
  SchedulerStub* scheduler_stub =
      reinterpret_cast<SchedulerStub*>(coordinator()->scheduler());
  EXPECT_TRUE(scheduler_stub->backup_schedule_called());
  EXPECT_TRUE(scheduler_stub->unschedule_called());
}

// If one item completes, and there are no more user requeted items left,
// we should make a scheduler entry for a non-user requested item.
TEST_F(RequestCoordinatorTest, RequestNotPickedNonUserRequestedItemsRemain) {
  coordinator()->StartScheduledProcessing(device_conditions(),
                                          processing_callback());
  EXPECT_TRUE(state() == RequestCoordinatorState::PICKING);

  // Call RequestNotPicked, and make sure we pick schedule a task for non user
  // requested conditions, with no tasks on the disabled list.
  CallRequestNotPicked(true, false);
  PumpLoop();

  EXPECT_FALSE(state() == RequestCoordinatorState::PICKING);
  EXPECT_TRUE(processing_callback_called());

  // The scheduler should have been called to schedule the non-user requested
  // task.
  SchedulerStub* scheduler_stub =
      reinterpret_cast<SchedulerStub*>(coordinator()->scheduler());
  EXPECT_TRUE(scheduler_stub->schedule_called());
  EXPECT_TRUE(scheduler_stub->unschedule_called());
  const Scheduler::TriggerConditions* conditions =
      scheduler_stub->trigger_conditions();
  EXPECT_EQ(conditions->require_power_connected,
            coordinator()->policy()->PowerRequired(!kUserRequested));
  EXPECT_EQ(
      conditions->minimum_battery_percentage,
      coordinator()->policy()->BatteryPercentageRequired(!kUserRequested));
  EXPECT_EQ(conditions->require_unmetered_network,
            coordinator()->policy()->UnmeteredNetworkRequired(!kUserRequested));
}

TEST_F(RequestCoordinatorTest, SchedulerGetsLeastRestrictiveConditions) {
  // Put two requests on the queue - The first is user requested, and
  // the second is not user requested.
  AddRequest1();
  offline_pages::SavePageRequest request2(kRequestId2, kUrl2, kClientId2,
                                          base::Time::Now(), !kUserRequested);
  coordinator()->queue()->AddRequest(
      request2, base::Bind(&RequestCoordinatorTest::AddRequestDone,
                           base::Unretained(this)));
  PumpLoop();

  // Trigger the scheduler to schedule for the least restrictive condition.
  ScheduleForTest();
  PumpLoop();

  // Expect that the scheduler got notified, and it is at user_requested
  // priority.
  SchedulerStub* scheduler_stub =
      reinterpret_cast<SchedulerStub*>(coordinator()->scheduler());
  const Scheduler::TriggerConditions* conditions =
      scheduler_stub->trigger_conditions();
  EXPECT_TRUE(scheduler_stub->schedule_called());
  EXPECT_EQ(conditions->require_power_connected,
            coordinator()->policy()->PowerRequired(kUserRequested));
  EXPECT_EQ(conditions->minimum_battery_percentage,
            coordinator()->policy()->BatteryPercentageRequired(kUserRequested));
  EXPECT_EQ(conditions->require_unmetered_network,
            coordinator()->policy()->UnmeteredNetworkRequired(kUserRequested));
}

TEST_F(RequestCoordinatorTest, StartScheduledProcessingWithLoadingDisabled) {
  // Add a request to the queue, wait for callbacks to finish.
  AddRequest1();
  PumpLoop();

  DisableLoading();
  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      processing_callback()));

  // Let the async callbacks in the request coordinator run.
  PumpLoop();
  EXPECT_TRUE(processing_callback_called());

  EXPECT_FALSE(state() == RequestCoordinatorState::PICKING);
  EXPECT_EQ(Offliner::LOADING_NOT_ACCEPTED, last_offlining_status());
}

// TODO(dougarnett): Add StartScheduledProcessing test for QUEUE_UPDATE_FAILED.

// This tests a StopProcessing call before we have actually started the
// offliner.
TEST_F(RequestCoordinatorTest,
       StartScheduledProcessingThenStopProcessingImmediately) {
  // Add a request to the queue, wait for callbacks to finish.
  AddRequest1();
  PumpLoop();

  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      processing_callback()));
  EXPECT_TRUE(state() == RequestCoordinatorState::PICKING);

  // Now, quick, before it can do much (we haven't called PumpLoop), cancel it.
  coordinator()->StopProcessing(Offliner::REQUEST_COORDINATOR_CANCELED);

  // Let the async callbacks in the request coordinator run.
  PumpLoop();
  EXPECT_TRUE(processing_callback_called());

  EXPECT_FALSE(state() == RequestCoordinatorState::PICKING);

  // OfflinerDoneCallback will not end up getting called with status SAVED,
  // since we cancelled the event before it called offliner_->LoadAndSave().
  EXPECT_EQ(Offliner::RequestStatus::REQUEST_COORDINATOR_CANCELED,
            last_offlining_status());

  // Since offliner was not started, it will not have seen cancel call.
  EXPECT_FALSE(OfflinerWasCanceled());
}

// This tests a StopProcessing call after the background loading has been
// started.
TEST_F(RequestCoordinatorTest,
       StartScheduledProcessingThenStopProcessingLater) {
  // Add a request to the queue, wait for callbacks to finish.
  AddRequest1();
  PumpLoop();

  // Ensure the start processing request stops before the completion callback.
  EnableOfflinerCallback(false);

  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      processing_callback()));
  EXPECT_TRUE(state() == RequestCoordinatorState::PICKING);

  // Let all the async parts of the start processing pipeline run to completion.
  PumpLoop();

  // Observer called for starting processing.
  EXPECT_TRUE(observer().changed_called());
  EXPECT_EQ(SavePageRequest::RequestState::OFFLINING, observer().state());
  observer().Clear();

  // Since the offliner is disabled, this callback should not be called.
  EXPECT_FALSE(processing_callback_called());

  // Coordinator should now be busy.
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
  EXPECT_FALSE(state() == RequestCoordinatorState::PICKING);

  // Now we cancel it while the background loader is busy.
  coordinator()->StopProcessing(Offliner::REQUEST_COORDINATOR_CANCELED);

  // Let the async callbacks in the cancel run.
  PumpLoop();

  // Observer called for stopped processing.
  EXPECT_TRUE(observer().changed_called());
  EXPECT_EQ(SavePageRequest::RequestState::AVAILABLE, observer().state());
  observer().Clear();

  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);

  // OfflinerDoneCallback will not end up getting called with status SAVED,
  // since we cancelled the event before the LoadAndSave completed.
  EXPECT_EQ(Offliner::RequestStatus::REQUEST_COORDINATOR_CANCELED,
            last_offlining_status());

  // Since offliner was started, it will have seen cancel call.
  EXPECT_TRUE(OfflinerWasCanceled());
}

// This tests that canceling a request will result in TryNextRequest() getting
// called.
TEST_F(RequestCoordinatorTest, RemoveInflightRequest) {
  // Add a request to the queue, wait for callbacks to finish.
  AddRequest1();
  PumpLoop();

  // Ensure the start processing request stops before the completion callback.
  EnableOfflinerCallback(false);

  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      processing_callback()));

  // Let all the async parts of the start processing pipeline run to completion.
  PumpLoop();
  // Since the offliner is disabled, this callback should not be called.
  EXPECT_FALSE(processing_callback_called());

  // Remove the request while it is processing.
  std::vector<int64_t> request_ids{kRequestId1};
  coordinator()->RemoveRequests(
      request_ids, base::Bind(&RequestCoordinatorTest::RemoveRequestsDone,
                              base::Unretained(this)));

  // Let the async callbacks in the cancel run.
  PumpLoop();

  // Since offliner was started, it will have seen cancel call.
  EXPECT_TRUE(OfflinerWasCanceled());
}

TEST_F(RequestCoordinatorTest, MarkRequestCompleted) {
  int64_t request_id = SavePageLaterWithAvailability(
      RequestCoordinator::RequestAvailability::DISABLED_FOR_OFFLINER);

  // Verify that the request is added to the disabled list.
  EXPECT_FALSE(disabled_requests().empty());

  PumpLoop();
  EXPECT_NE(request_id, 0l);

  // Verify request added in OFFLINING state.
  EXPECT_TRUE(observer().added_called());
  EXPECT_EQ(SavePageRequest::RequestState::OFFLINING, observer().state());

  // Call the method under test, making sure we send SUCCESS to the observer.
  coordinator()->MarkRequestCompleted(request_id);
  PumpLoop();

  // Our observer should have seen SUCCESS instead of USER_CANCELED.
  EXPECT_EQ(RequestCoordinator::BackgroundSavePageResult::SUCCESS,
            observer().last_status());
  EXPECT_TRUE(observer().completed_called());
}

TEST_F(RequestCoordinatorTest, EnableForOffliner) {
  // Pretend we are on low-end device so immediate start won't happen.
  SetIsLowEndDeviceForTest(true);

  int64_t request_id = SavePageLaterWithAvailability(
      RequestCoordinator::RequestAvailability::DISABLED_FOR_OFFLINER);

  // Verify that the request is added to the disabled list.
  EXPECT_FALSE(disabled_requests().empty());

  PumpLoop();
  EXPECT_NE(request_id, 0l);

  // Verify request added and initial change to OFFLINING (in foreground).
  EXPECT_TRUE(observer().added_called());
  EXPECT_TRUE(observer().changed_called());
  EXPECT_EQ(SavePageRequest::RequestState::OFFLINING, observer().state());
  observer().Clear();

  // Ensure that the new request does not finish so we can verify state change.
  EnableOfflinerCallback(false);

  coordinator()->EnableForOffliner(request_id, kClientId1);
  PumpLoop();

  // Verify request changed again.
  EXPECT_TRUE(observer().changed_called());
  EXPECT_EQ(SavePageRequest::RequestState::AVAILABLE, observer().state());
}

TEST_F(RequestCoordinatorTest,
       WatchdogTimeoutForScheduledProcessingNoLastSnapshot) {
  // Build a request to use with the pre-renderer, and put it on the queue.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  // Set request to allow one more completed attempt.
  int max_tries = coordinator()->policy()->GetMaxCompletedTries();
  request.set_completed_attempt_count(max_tries - 1);
  coordinator()->queue()->AddRequest(
      request, base::Bind(&RequestCoordinatorTest::AddRequestDone,
                          base::Unretained(this)));
  PumpLoop();

  // Ensure that the new request does not finish - we simulate it being
  // in progress by asking it to skip making the completion callback.
  EnableOfflinerCallback(false);

  // Sending the request to the offliner.
  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      waiting_callback()));
  PumpLoop();

  // Advance the mock clock far enough to cause a watchdog timeout
  AdvanceClockBy(base::TimeDelta::FromSeconds(
      coordinator()
          ->policy()
          ->GetSinglePageTimeLimitWhenBackgroundScheduledInSeconds() +
      1));
  PumpLoop();

  // Wait for timeout to expire.  Use a TaskRunner with a DelayedTaskRunner
  // which won't time out immediately, so the watchdog thread doesn't kill valid
  // tasks too soon.
  WaitForCallback();
  PumpLoop();

  EXPECT_FALSE(state() == RequestCoordinatorState::PICKING);
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);
  EXPECT_TRUE(OfflinerWasCanceled());
}

TEST_F(RequestCoordinatorTest,
       WatchdogTimeoutForImmediateProcessingNoLastSnapshot) {
  // Ensure that the new request does not finish - we simulate it being
  // in progress by asking it to skip making the completion callback.
  EnableOfflinerCallback(false);

  EXPECT_NE(0, SavePageLater());
  PumpLoop();

  // Verify that immediate start from adding the request did happen.
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);

  // Advance the mock clock 1 second before the watchdog timeout.
  AdvanceClockBy(base::TimeDelta::FromSeconds(
      coordinator()
          ->policy()
          ->GetSinglePageTimeLimitForImmediateLoadInSeconds() -
      1));
  PumpLoop();

  // Verify still busy.
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
  EXPECT_FALSE(OfflinerWasCanceled());

  // Advance the mock clock past the watchdog timeout now.
  AdvanceClockBy(base::TimeDelta::FromSeconds(2));
  PumpLoop();

  // Verify the request timed out.
  EXPECT_TRUE(OfflinerWasCanceled());
}

TEST_F(RequestCoordinatorTest, TimeBudgetExceeded) {
  EnableOfflinerCallback(false);
  // Build two requests to use with the pre-renderer, and put it on the queue.
  AddRequest1();
  // The second request will have a larger completed attempt count.
  offline_pages::SavePageRequest request2(kRequestId1 + 1, kUrl1, kClientId1,
                                          base::Time::Now(), kUserRequested);
  request2.set_completed_attempt_count(kAttemptCount);
  coordinator()->queue()->AddRequest(
      request2, base::Bind(&RequestCoordinatorTest::AddRequestDone,
                           base::Unretained(this)));
  PumpLoop();

  // Sending the request to the offliner.
  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      waiting_callback()));
  PumpLoop();

  // Advance the mock clock far enough to exceed our time budget.
  // The first request will time out, and because we are over time budget,
  // the second request will not be started.
  AdvanceClockBy(base::TimeDelta::FromSeconds(kTestTimeBudgetSeconds));
  PumpLoop();

  // TryNextRequest should decide that there is no more work to be done,
  // and call back to the scheduler, even though there is another request in the
  // queue.  Both requests should be left in the queue.
  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // We should find two requests in the queue.
  // The first request should now have a completed count of 1.
  EXPECT_EQ(2UL, last_requests().size());
  EXPECT_EQ(1L, last_requests().at(0)->completed_attempt_count());
}

TEST_F(RequestCoordinatorTest, TryNextRequestWithNoNetwork) {
  SavePageRequest request1 = AddRequest1();
  AddRequest2();
  PumpLoop();

  // Set up for the call to StartScheduledProcessing.
  EnableOfflinerCallback(false);

  // Sending the request to the offliner.
  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      waiting_callback()));
  PumpLoop();
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);

  // Now lose the network connection.
  SetNetworkConnected(false);

  // Complete first request and then TryNextRequest should decide not
  // to pick another request (because of no network connection).
  SendOfflinerDoneCallback(request1, Offliner::RequestStatus::SAVED);
  PumpLoop();

  // Not starting nor busy with next request.
  EXPECT_FALSE(state() == RequestCoordinatorState::PICKING);
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);

  // Get queued requests.
  coordinator()->queue()->GetRequests(base::Bind(
      &RequestCoordinatorTest::GetRequestsDone, base::Unretained(this)));
  PumpLoop();

  // We should find one request in the queue.
  EXPECT_EQ(1UL, last_requests().size());
}

TEST_F(RequestCoordinatorTest, GetAllRequests) {
  // Add two requests to the queue.
  AddRequest1();
  AddRequest2();
  PumpLoop();

  // Start the async status fetching.
  coordinator()->GetAllRequests(base::Bind(
      &RequestCoordinatorTest::GetQueuedRequestsDone, base::Unretained(this)));
  PumpLoop();

  // Wait for async get to finish.
  WaitForCallback();
  PumpLoop();

  // Check that the statuses found in the callback match what we expect.
  EXPECT_EQ(2UL, last_requests().size());
  EXPECT_EQ(kRequestId1, last_requests().at(0)->request_id());
  EXPECT_EQ(kRequestId2, last_requests().at(1)->request_id());
}

TEST_F(RequestCoordinatorTest, PauseAndResumeObserver) {
  // Set low-end device status to actual status.
  SetIsLowEndDeviceForTest(base::SysInfo::IsLowEndDevice());

  // Add a request to the queue.
  AddRequest1();
  PumpLoop();

  // Pause the request.
  std::vector<int64_t> request_ids;
  request_ids.push_back(kRequestId1);
  coordinator()->PauseRequests(request_ids);
  PumpLoop();

  EXPECT_TRUE(observer().changed_called());
  EXPECT_EQ(SavePageRequest::RequestState::PAUSED, observer().state());

  // Clear out the observer before the next call.
  observer().Clear();

  // Resume the request.
  coordinator()->ResumeRequests(request_ids);
  PumpLoop();

  EXPECT_TRUE(observer().changed_called());

  // Now whether request is offlining or just available depends on whether test
  // is run on svelte device or not.
  if (base::SysInfo::IsLowEndDevice()) {
    EXPECT_EQ(SavePageRequest::RequestState::AVAILABLE, observer().state());
  } else {
    EXPECT_EQ(SavePageRequest::RequestState::OFFLINING, observer().state());
  }
}

TEST_F(RequestCoordinatorTest, RemoveRequest) {
  // Add a request to the queue.
  AddRequest1();
  PumpLoop();

  // Remove the request.
  std::vector<int64_t> request_ids;
  request_ids.push_back(kRequestId1);
  coordinator()->RemoveRequests(
      request_ids, base::Bind(&RequestCoordinatorTest::RemoveRequestsDone,
                              base::Unretained(this)));

  PumpLoop();
  WaitForCallback();
  PumpLoop();

  EXPECT_TRUE(observer().completed_called());
  EXPECT_EQ(RequestCoordinator::BackgroundSavePageResult::USER_CANCELED,
            observer().last_status());
  EXPECT_EQ(1UL, last_remove_results().size());
  EXPECT_EQ(kRequestId1, std::get<0>(last_remove_results().at(0)));
}

TEST_F(RequestCoordinatorTest,
       SavePageStartsProcessingWhenConnectedAndNotLowEndDevice) {
  // Turn off the callback so that the request stops before processing in
  // PumpLoop.
  EnableOfflinerCallback(false);
  EXPECT_NE(0, SavePageLater());
  PumpLoop();

  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
}

TEST_F(RequestCoordinatorTest,
       SavePageStartsProcessingWhenConnectedOnLowEndDeviceIfFlagEnabled) {
  // Mark device as low-end device.
  SetIsLowEndDeviceForTest(true);
  EXPECT_FALSE(offline_pages::IsOfflinePagesSvelteConcurrentLoadingEnabled());

  // Make a request.
  EXPECT_NE(0, SavePageLater());
  PumpLoop();

  // Verify not immediately busy (since low-end device).
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);

  // Set feature flag to allow concurrent loads.
  base::test::ScopedFeatureList scoped_feature_list;
  scoped_feature_list.InitAndEnableFeature(
      kOfflinePagesSvelteConcurrentLoadingFeature);
  EXPECT_TRUE(offline_pages::IsOfflinePagesSvelteConcurrentLoadingEnabled());

  // Turn off the callback so that the request stops before processing in
  // PumpLoop.
  EnableOfflinerCallback(false);

  // Make another request.
  RequestCoordinator::SavePageLaterParams params;
  params.url = kUrl2;
  params.client_id = kClientId2;
  params.user_requested = kUserRequested;
  EXPECT_NE(0, coordinator()->SavePageLater(params));
  PumpLoop();

  // Verify immediate processing did start this time.
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
}

TEST_F(RequestCoordinatorTest, SavePageDoesntStartProcessingWhenDisconnected) {
  SetNetworkConnected(false);
  EnableOfflinerCallback(false);
  EXPECT_NE(0, SavePageLater());
  PumpLoop();
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);

  // Now connect network and verify processing starts.
  SetNetworkConnected(true);
  CallConnectionTypeObserver();
  PumpLoop();
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
}

TEST_F(RequestCoordinatorTest,
       SavePageDoesStartProcessingWhenPoorlyConnected) {
  // Set specific network type for 2G with poor effective connection.
  DeviceConditions device_conditions(!kPowerRequired, kBatteryPercentageHigh,
                                     net::NetworkChangeNotifier::CONNECTION_2G);
  SetDeviceConditionsForTest(device_conditions);
  SetEffectiveConnectionTypeForTest(
      net::EffectiveConnectionType::EFFECTIVE_CONNECTION_TYPE_SLOW_2G);

  // Turn off the callback so that the request stops before processing in
  // PumpLoop.
  EnableOfflinerCallback(false);

  EXPECT_NE(0, SavePageLater());
  PumpLoop();
  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
}

TEST_F(RequestCoordinatorTest,
       ResumeStartsProcessingWhenConnectedAndNotLowEndDevice) {
  // Start unconnected.
  SetNetworkConnected(false);

  // Turn off the callback so that the request stops before processing in
  // PumpLoop.
  EnableOfflinerCallback(false);

  // Add a request to the queue.
  AddRequest1();
  PumpLoop();
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);

  // Pause the request.
  std::vector<int64_t> request_ids;
  request_ids.push_back(kRequestId1);
  coordinator()->PauseRequests(request_ids);
  PumpLoop();

  // Resume the request while disconnected.
  coordinator()->ResumeRequests(request_ids);
  PumpLoop();
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);
  EXPECT_EQ(1UL, prioritized_requests().size());

  // Pause the request again.
  coordinator()->PauseRequests(request_ids);
  PumpLoop();
  EXPECT_EQ(0UL, prioritized_requests().size());

  // Now simulate reasonable connection.
  SetNetworkConnected(true);

  // Resume the request while connected.
  coordinator()->ResumeRequests(request_ids);
  EXPECT_FALSE(state() == RequestCoordinatorState::OFFLINING);
  PumpLoop();
  EXPECT_EQ(1UL, prioritized_requests().size());

  EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);
}

TEST_F(RequestCoordinatorTest, SnapshotOnLastTryForScheduledProcessing) {
  // Build a request to use with the pre-renderer, and put it on the queue.
  offline_pages::SavePageRequest request(kRequestId1, kUrl1, kClientId1,
                                         base::Time::Now(), kUserRequested);
  // Set request to allow one more completed attempt. So that the next try would
  // be the last retry.
  int max_tries = coordinator()->policy()->GetMaxCompletedTries();
  request.set_completed_attempt_count(max_tries - 1);
  coordinator()->queue()->AddRequest(
      request,
      base::Bind(&RequestCoordinatorTest::AddRequestDone,
                 base::Unretained(this)));
  PumpLoop();

  // Ensure that the new request does not finish - we simulate it being
  // in progress by asking it to skip making the completion callback.
  // Also make snapshot on last retry happen in this case.
  EnableOfflinerCallback(false);
  EnableSnapshotOnLastRetry();

  // Sending the request to the offliner.
  EXPECT_TRUE(coordinator()->StartScheduledProcessing(device_conditions(),
                                                      waiting_callback()));
  PumpLoop();

  // Advance the mock clock far enough to cause a watchdog timeout
  AdvanceClockBy(base::TimeDelta::FromSeconds(
      coordinator()
          ->policy()
          ->GetSinglePageTimeLimitWhenBackgroundScheduledInSeconds() +
      1));
  PumpLoop();

  // Wait for timeout to expire.  Use a TaskRunner with a DelayedTaskRunner
  // which won't time out immediately, so the watchdog thread doesn't kill valid
  // tasks too soon.
  WaitForCallback();
  PumpLoop();

  // Check the offliner didn't get a cancel and the result was success.
  EXPECT_FALSE(OfflinerWasCanceled());
  EXPECT_EQ(RequestCoordinator::BackgroundSavePageResult::SUCCESS,
            observer().last_status());
  EXPECT_TRUE(observer().completed_called());
}

TEST_F(RequestCoordinatorTest, SnapshotOnLastTryForImmediateProcessing) {
  // Ensure that the new request does not finish - we simulate it being
  // in progress by asking it to skip making the completion callback.
  EnableOfflinerCallback(false);

  EXPECT_NE(0, SavePageLater());

  // Repeat the timeout for MaxCompleteTries - 1 times in order to increase the
  // completed tries on this request.
  int max_tries = coordinator()->policy()->GetMaxCompletedTries();
  for (int i = 0; i < max_tries - 1; i++) {
    PumpLoop();
    // Reset states.
    ResetOfflinerWasCanceled();
    observer().Clear();

    // Verify that the request is being processed.
    EXPECT_TRUE(state() == RequestCoordinatorState::OFFLINING);

    // Advance the mock clock 1 second more than the watchdog timeout.
    AdvanceClockBy(base::TimeDelta::FromSeconds(
        coordinator()
            ->policy()
            ->GetSinglePageTimeLimitForImmediateLoadInSeconds() +
        1));
    PumpLoop();

    // Verify the request timed out.
    EXPECT_TRUE(OfflinerWasCanceled());
    EXPECT_TRUE(observer().changed_called());
  }

  // Reset states.
  ResetOfflinerWasCanceled();
  observer().Clear();
  // Make snapshot on last retry happen.
  EnableSnapshotOnLastRetry();

  // Advance the mock clock 1 second more than the watchdog timeout.
  AdvanceClockBy(base::TimeDelta::FromSeconds(
      coordinator()
          ->policy()
          ->GetSinglePageTimeLimitForImmediateLoadInSeconds() +
      1));
  PumpLoop();

  // The last time would trigger the snapshot on last retry and succeed.
  EXPECT_FALSE(OfflinerWasCanceled());
  EXPECT_EQ(RequestCoordinator::BackgroundSavePageResult::SUCCESS,
            observer().last_status());
  EXPECT_TRUE(observer().completed_called());
}

}  // namespace offline_pages
