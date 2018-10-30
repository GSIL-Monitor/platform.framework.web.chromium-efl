// Copyright 2018 Samsung Electronics Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_TIZEN_RESOURCE_MANAGER_H_
#define CONTENT_COMMON_TIZEN_RESOURCE_MANAGER_H_

#include <string>
#include <vector>

#include "base/synchronization/waitable_event.h"
#include "media/base/video_codecs.h"

namespace base {
class WaitableEvent;
}

namespace content {

/* Describes allocated resource. When releasing a resource can be identified
 * by |id|. This should be kept in sync with struct rm_device_return_s
 * from rm_type.h. */
struct TizenAllocatedResource {
  int id;
  std::string device_node;
  std::string omx_comp_name;
  TizenAllocatedResource();
  TizenAllocatedResource(int _id, const char* node, const char* omx_comp);
  ~TizenAllocatedResource() {}
};

typedef std::vector<std::unique_ptr<TizenAllocatedResource>> TizenResourceList;

/* These describe a mode in which resource can be allocated. Each mode
 * influence behavior of actual system resource manager when a resource
 * conflict occurs on a given resource. Unfortunately documentation is very
 * scarce in this region so, no detailed description of each state can be
 * provided here. If unsure, use RESOURCE_STATE_EXCLUSIVE.
 * Note: When changing this enum, one must also update kResourceStateMap in
 * browser_tizen_resource_manager.c */
enum TizenResourceState {
  RESOURCE_STATE_PASSIVE,
  RESOURCE_STATE_SHARABLE,
  RESOURCE_STATE_EXCLUSIVE,
  RESOURCE_STATE_EXCLUSIVE_CONDITIONAL,
  RESOURCE_STATE_EXCLUSIVE_AUTO,
  RESOURCE_STATE_MAX,
};

/* Represents a type of resource. Platform resource manager is capable of
 * allocating a broad spectrum of resource types. However for now only those
 * are used in chromium. When adding new resource type (that translates
 * to RM_CATEGORY_* enum from rm_type.h) please add also corresponding
 * TizenResourceManager::Prepare*Request() method.
 * Note: When changing this enum, one must also update kResourceCategoryMap in
 * browser_tizen_resource_manager.cc */
enum TizenResourceCategoryId {
  RESOURCE_CATEGORY_VIDEO_DECODER,
  RESOURCE_CATEGORY_VIDEO_DECODER_SUB,
  RESOURCE_CATEGORY_SCALER,
  RESOURCE_CATEGORY_SCALER_SUB,
  RESOURCE_CATEGORY_MAX,
};

/* This type is used to describe any resource to be allocated. It consists of
 * three components:
 * |category_id| denotes a coarse type of resource e.g. video decoder or camera
 * |category_option| describes a resource with particular capabilities. For
 *                   example video decoder capable of decoding codec X with
 *                   resolution Y, will have different category value that one,
 *                   capable of decoding codec Z with resolution Y.
 * |state| look at description of TizenResourceState.
 *
 * This structure corresponds to struct rm_category_request_s from rm_type.h.
 */
struct TizenRequestedResource {
  TizenResourceCategoryId category_id;
  int category_option;
  TizenResourceState state;
};

/* This should be kept in sync with struct ri_sampling_format from
 * ri-common-type.h. */
enum VideoDecoderSamplingFormat {
  VIDEO_DECODER_SAMPLING_OTHER = 0x00,
  VIDEO_DECODER_SAMPLING_420 = 0x01,
  VIDEO_DECODER_SAMPLING_422 = 0x02,
  VIDEO_DECODER_SAMPLING_DEFAULT = VIDEO_DECODER_SAMPLING_420,
};

/* Can be used to describe capabilities of video decoder. This can be turned
 * into a particular value of TizenResourceCategory with help of
 * TizenResourceManager::GetVideoDecoderCategory().
 * |framerate| and |bpp| can be set to -1 to use their default values.
 * This should be kept in sync with struct ri_video_category_option_request_s
 * from ri-common-type.h. */
struct TizenVideoDecoderDescription {
  media::VideoCodec codec;
  int width;
  int height;
  /* Default is 60 fps */
  int framerate;
  /* Default is 8 */
  int bpp;
  /* Default is VIDEO_DECODER_SAMPLING_420 */
  VideoDecoderSamplingFormat sampling_format;
};

enum TizenResourceConflictType {
  CONFLICT_TYPE_UNKNOWN = 1,
  CONFLICT_TYPE_RESOURCE_CONFLICT,    // common resource conflict
  CONFLICT_TYPE_RESOURCE_CONFLICT_UD  // resource conflict by ud device
};

class TizenResourceManager {
 public:
  class ResourceClient {
   public:
    virtual ~ResourceClient() {}
    // Callback from TizenResourceManager to inform resource user to release it
    // as fast as possible. It have two modes of operations depending on return
    // value from client:
    // |type| currently doesn't have any special meaning. Client should
    // release its resources as fast as possible after getting callback.
    // |ids| is a list of resources allocated by client getting callback.
    //
    // Return value:
    // false - TizenResourceManager will wait until wait event is
    //         signaled by user, meaning resource was released.
    //         Wait is indefinite, however when not signaled
    //         the RM server will eventually kill process from
    //         which rm_register was called (browser).
    // true  - resource is already released. Wait pointer musn't
    //         be accessed by client anymore.
    virtual bool RMResourceCallback(base::WaitableEvent* wait,
                                    TizenResourceConflictType type,
                                    const std::vector<int>& ids) = 0;
  };

  /* Allocate a set of resources, each described by value of
   * TizenResourceCategory. Resources are returned via |ret| pointer.
   * Maximum request size is kMaxResourceRequestSize.
   *
   * Return value:
   * false    - no resources were allocated.
   * true     - all requested resource are allocated.
   *
   * Note: this function can be called from any thread. However classes
   *       that implements this interface can pose additional restrictions
   *       on this rule. */
  virtual bool AllocateResources(ResourceClient* client,
                                 const std::vector<TizenRequestedResource>& req,
                                 TizenResourceList* ret) = 0;

  /* Releases previously allocated resources. If a few resources was allocated
   * at once with AllocateResources(), they not necessarily must be deallocated
   * together.
   * |ids| is a list of resources to be released. Use values of
   *       TizenAllocatedResource::id.
   *       Maximum length is kMaxResourceRequestSize.
   *
   * Return value:
   * false    - no resources were released.
   * true     - all requested resource are released.
   *
   * Note: this function can be called from any thread. However classes
   *       that implements this interface can pose additional restrictions
   *       on this rule. */
  virtual bool ReleaseResources(ResourceClient* client,
                                const std::vector<int>& ids) = 0;

  /* Use to transform video decoder description |desc| into
   * TizenRequestedResource returned trough |ret| which can be used to allocate
   * a video decoder resource.
   * Default resource state is RESOURCE_STATE_EXCLUSIVE.
   * |sub| can be set to true for secondary resource or false for main
   *       resource.
   *
   * Return value:
   * false    - invalid values in |desc|
   * true     - success
   */
  static bool PrepareVideoDecoderRequest(
      const TizenVideoDecoderDescription* desc,
      TizenRequestedResource* ret,
      bool sub);

  /* Use to prepare request for allocation of scaler resource.
   * |sub| can be set to true for secondary resource or false for main
   *       resource.
   * Default resource state is RESOURCE_STATE_EXCLUSIVE. */
  static void PrepareScalerRequest(TizenRequestedResource* ret, bool sub);

  /* Converts TizenResourceList |list| returned from allocation to a vector of
   * resource id's |ids|. Can be used as helper for ReleaseResources(). */
  static std::vector<int> ResourceListToIds(const TizenResourceList* list);

  static std::string IdsToString(const std::vector<int> ids);

  /* Tells how many resources can be processed by one allocation/deallocation
   * request.
   * Should be kept in sync with RM_REQUEST_RESOURCE_MAX from rm_type.h. */
  static const unsigned kMaxResourceRequestSize;

 protected:
  virtual ~TizenResourceManager() {}
};

}  // namespace content

#endif  // CONTENT_COMMON_TIZEN_RESOURCE_MANAGER_H_
