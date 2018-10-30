/* Copyright (c) 2016 Samsung Electronics. All rights reserved.
 */

/* From samsung/ppb_media_data_source_samsung.idl,
 *   modified Mon May 29 08:30:02 2017.
 */

#ifndef PPAPI_C_SAMSUNG_PPB_MEDIA_DATA_SOURCE_SAMSUNG_H_
#define PPAPI_C_SAMSUNG_PPB_MEDIA_DATA_SOURCE_SAMSUNG_H_

#include "ppapi/c/pp_bool.h"
#include "ppapi/c/pp_completion_callback.h"
#include "ppapi/c/pp_instance.h"
#include "ppapi/c/pp_macros.h"
#include "ppapi/c/pp_resource.h"
#include "ppapi/c/pp_size.h"
#include "ppapi/c/pp_stdint.h"
#include "ppapi/c/pp_time.h"
#include "ppapi/c/pp_var.h"
#include "ppapi/c/samsung/pp_media_codecs_samsung.h"
#include "ppapi/c/samsung/pp_media_common_samsung.h"
#include "ppapi/c/samsung/pp_media_player_samsung.h"
#include "ppapi/c/samsung/ppp_media_data_source_samsung.h"

#define PPB_MEDIADATASOURCE_SAMSUNG_INTERFACE_1_0 \
    "PPB_MediaDataSource_Samsung;1.0"
#define PPB_MEDIADATASOURCE_SAMSUNG_INTERFACE \
    PPB_MEDIADATASOURCE_SAMSUNG_INTERFACE_1_0

#define PPB_URLDATASOURCE_SAMSUNG_INTERFACE_1_0 "PPB_URLDataSource_Samsung;1.0"
#define PPB_URLDATASOURCE_SAMSUNG_INTERFACE \
    PPB_URLDATASOURCE_SAMSUNG_INTERFACE_1_0

#define PPB_ESDATASOURCE_SAMSUNG_INTERFACE_1_0 "PPB_ESDataSource_Samsung;1.0"
#define PPB_ESDATASOURCE_SAMSUNG_INTERFACE \
    PPB_ESDATASOURCE_SAMSUNG_INTERFACE_1_0

#define PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0 \
    "PPB_ElementaryStream_Samsung;1.0"
#define PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_1 \
    "PPB_ElementaryStream_Samsung;1.1"
#define PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE \
    PPB_ELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_1

#define PPB_AUDIOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0 \
    "PPB_AudioElementaryStream_Samsung;1.0"
#define PPB_AUDIOELEMENTARYSTREAM_SAMSUNG_INTERFACE \
    PPB_AUDIOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0

#define PPB_VIDEOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0 \
    "PPB_VideoElementaryStream_Samsung;1.0"
#define PPB_VIDEOELEMENTARYSTREAM_SAMSUNG_INTERFACE \
    PPB_VIDEOELEMENTARYSTREAM_SAMSUNG_INTERFACE_1_0

/**
 * @file
 * This file defines the Media Data Sources interfaces, which provide
 * abilities to feed player with media clip data.
 *
 * Part of Pepper Media Player interfaces (Samsung's extension).
 * See See comments in ppb_media_player_samsung for general overview.
 *
 * Data Sources:
 * - <code>PPB_MediaDataSource_Samusng</code> is an abstract interface
 *   representing data source.
 * - <code>PPB_URLDataSource_Samusng</code> is data source which opens
 *   stream given by URL. It is derived from
 *   <code>PPB_DataSource_Samusng</code>.
 * - <code>PPB_ESDataSource_Samusng</code> is data source allowing to provide
 *   Elementary Streams from Pepper/PNaCl module. Using this player the module
 *   can create buffers accepting Elementary Stream packets.
 *   Those buffers are derived from <code>PPB_ElementaryStream_Samusng</code>
 *   interface.
 *
 *   Relationship can be represented as follows:
 *   PPB_MediaDataSource_Samusng [base]
 *      |
 *      |- PPB_URLDataSource_Samusng [derived]
 *      |- PPB_ESDataSource_Samusng  [derived] consists of set of
 *                                             PPB_ElementaryStream_Samusng
 *
 * - <code>PPB_ElementaryStream_Samusng</code> is basic interface providing
 *   methods to submit Elementary Stream packets to the player. Those packets
 *   might be optionally encrypted. Inheriting interfaces must provide methods
 *   necessary to initialize decoder handling elementary stream of given type
 *   (audio or video).
 * - <code>PPB_AudioElementaryStream_Samsung</code> is interface allowing
 *   application to provide packets comprising audio Elementary Stream.
 * - <code>PPB_VideoElementaryStream_Samsung</code> is interface allowing
 *   application to provide packets comprising video Elementary Stream.
 *
 *   Relationship can be represented as follows:
 *   PPB_ElementaryStream_Samsung [base]
 *      |
 *      |- PPB_AudioElementaryStream_Samsung [derived]
 *      |- PPB_VideoElementaryStream_Samsung [derived]
 */


/**
 * @addtogroup Interfaces
 * @{
 */
/**
 * Interface representing abstract Data Source.
 */
struct PPB_MediaDataSource_Samsung_1_0 {
  /**
   * Determines if the given resource is a media data source.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_MediaDataSource_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsMediaDataSource)(PP_Resource resource);
};

typedef struct PPB_MediaDataSource_Samsung_1_0 PPB_MediaDataSource_Samsung;

/**
 * Data Source handling media passed as URLs in Create method,
 * derives from PPB_MediaDataSource_Samsung.
 *
 * After creation data source can be attached to the player.
 * When attaching to the player passed URL is validated and buffering starts.
 */
struct PPB_URLDataSource_Samsung_1_0 {
  /**
   * Creates a new URL data source resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying the instance
   * with the URL data source.
   * @param[in] url An URL identifying media clip fetched by data source.
   *
   * @return A <code>PP_Resource</code> corresponding to a URL data source if
   * successful or 0 otherwise.
   */
  PP_Resource (*Create)(PP_Instance instance, const char* url);
  /**
   * Determines if the given resource is an URL data source.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_URLDataSource_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsURLDataSource)(PP_Resource resource);
  /**
   * Gets the value for specific Feature in the HTTP, MMS & Streaming Engine
   * (Smooth Streaming, HLS, DASH, DivX Plus Streaming, Widevine).
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   * @param[in] type A <code>PP_StreamingProperty</code> to get
   * @param[out] value Value of the property, see description of
   * <code>PP_StreamingProperty</code> what is requested format.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*GetStreamingProperty)(PP_Resource resource,
                                  PP_StreamingProperty type,
                                  struct PP_Var* value,
                                  struct PP_CompletionCallback callback);
  /**
   * Sets the value for specific Feature in the HTTP, MMS & Streaming Engine
   * (Smooth Streaming, HLS, DASH, DivX Plus Streaming, Widevine).
   *
   * When special setting is required in Streaming Engine for the
   * Start itrate setting or specific Content Protection (CP),
   * the CUSTOM_MESSAGE Property can be set.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   * @param[in] type A <code>PP_StreamingProperty</code> to set
   * @param[in] value New value of the property, see description of
   * <code>PP_StreamingProperty</code> what is requested format.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*SetStreamingProperty)(PP_Resource resource,
                                  PP_StreamingProperty type,
                                  struct PP_Var value,
                                  struct PP_CompletionCallback callback);
};

typedef struct PPB_URLDataSource_Samsung_1_0 PPB_URLDataSource_Samsung;

/**
 * Data source handling appends of Elementary Streams,
 * derives from PPB_MediaDataSource_Samsung.
 *
 * It is a container for Elementary Streams (audio/video/...) and there can
 * be at most one stream of given type (see
 * <code>PP_ElementaryStream_Type_Samsung</code>).
 *
 * Basic usage (playback of clip containing audio and video):
 *  1. Create ESDataSource resource using <code>Create<code>.
 *  2. Add audio stream using <code>AddStream<code> with
 *     <code>PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_AUDIO<code> type.
 *  3. Configure audio stream, by setting codec, sampling rate, channels and
 *     other necessary information.
 *  4. Call <code>PPB_ElementaryStream_Samsung.InitializeDone</code> to confirm
 *     the configuration
 *  5. Add video stream using <code>AddStream<code> with
 *     <code>PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_VIDEO<code> type.
 *  6. Configure video stream, by setting codec, frame rate, resolution and
 *     other necessary information.
 *  7. Call <code>PPB_ElementaryStream_Samsung.InitializeDone</code> to confirm
 *     the configuration.
 *  8. Attach data source to the player by calling
 *     <code>PPB_MediaPlayer_Samsung.AttachMediaSource</code>.
 *  9. Download and append Elementary Stream audio and video packets
 *     by calling <code>PPB_ElementaryStream_Samsung.AppendPacket</code>
 * 10. Signalize end of stream (clip) by calling <code>SetEndOfStream</code>
 * 11. Detach data source from the player by calling
 *     <code>PPB_MediaPlayer_Samsung.AttachMediaSource</code> with
 *     <code>NULL</code> resource.
 */
struct PPB_ESDataSource_Samsung_1_0 {
  /**
   * Creates a new ES data source resource.
   *
   * @param[in] instance A <code>PP_Instance</code> identifying the instance
   * with the ES data source.
   *
   * @return A <code>PP_Resource</code> corresponding to a media player if
   * successful or 0 otherwise.
   */
  PP_Resource (*Create)(PP_Instance instance);
  /**
   * Determines if the given resource is a ES Data Source.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_ESDataSource_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsESDataSource)(PP_Resource resource);
  /**
   * Factory method which adds stream of given type to the data source.
   *
   * This data source can handle at most one buffer of given type, so calling
   * multiple times this method with the same buffer type will return the
   * same resource as all previous calls. Specified listener will be ignored
   * if buffer is already created.
   *
   * Listener methods will be called in the same thread as was this method
   * invoked.
   *
   * @param[in] data_source A <code>PP_Resource</code> identifying the
   * ES data source to which add new stream.
   * @param[in] stream_type A <code>PP_ElementaryStream_Type_Samsung</code>
   * identifying the stream type which will be added.
   * @param[in] listener A <code>PPP_ElementaryStreamListener_Samsung</code>
   * listener which is required to notify application about data related events.
   * Cannot be NULL.
   * @param[in] user_data A pointer to user data which will be passed
   * to the listeners during method invocation (optional).
   * @param[out] stream A <code>PP_Resource</code> identifying the
   * added stream of requested type.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*AddStream)(
      PP_Resource data_source,
      PP_ElementaryStream_Type_Samsung stream_type,
      const struct PPP_ElementaryStreamListener_Samsung_1_0* listener,
      void* user_data,
      PP_Resource* stream,
      struct PP_CompletionCallback callback);
  /**
   * Sets duration of the whole media stream/container/clip.
   *
   * @param[in] data_source A <code>PP_Resource</code> identifying the
   * ES data source to which add new stream.
   * @param[in] duration A duration of played media.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*SetDuration)(PP_Resource data_source,
                         PP_TimeDelta duration,
                         struct PP_CompletionCallback callback);
  /**
   * Signalizes end of the whole stream/container/clip.
   *
   * @param[in] data_source A <code>PP_Resource</code> identifying the
   * ES data source which has ended.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*SetEndOfStream)(PP_Resource data_source,
                            struct PP_CompletionCallback callback);
};

typedef struct PPB_ESDataSource_Samsung_1_0 PPB_ESDataSource_Samsung;

/**
 * Interface representing common functionalities of elementary streams.
 *
 * Basic usage:
 * 1. Crate stream by calling <code>PPB_ESDataSource_Samsung.AddStream</code>
 * 2. Initialize buffer specific information (audio/video config)
 * 3. Call <code>InitializeDone</code>
 * 5. Attach Data Source to the player
 * 6. Appends Elementary Stream packets by calling <code>AppendPacket</code>
 * 7. Signalize end of stream (clip) by calling
 *    <code>PPB_ESDataSource_Samsung.SetEndOfStream</code>
 */
struct PPB_ElementaryStream_Samsung_1_1 {
  /**
   * Determines if the given resource is a media player.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_MediaPlayer_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsElementaryStream)(PP_Resource resource);
  /**
   * Retrieves stream type represented by this resource.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_ElementaryStream_Type_Samsung</code> represented by this
   * resource or <code>PP_ELEMENTARYSTREAM_TYPE_SAMSUNG_UNKNOWN</code> when
   * this resource doesn't represent any stream type.
   */
  PP_ElementaryStream_Type_Samsung (*GetStreamType)(PP_Resource resource);
  /**
   * Call this method to confirm new/updated buffer config. This method will
   * return PP_OK if set buffer config is valid or one of the error codes from
   * <code>pp_errors.h</code> otherwise.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the
   * elementary stream.
   * @param[in] mode A parameter indicating how much information regarding
   * elementary stream configuration was extracted by the plugin (demuxer).
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   * Returns <code>PP_ERROR_BADARGUMENT</code> when stream configuration
   * is invalid.
   * Returns <code>PP_ERROR_NOTSUPPORTED</code> when given initialization
   * mode is not supported by the browser.
   */
  int32_t (*InitializeDone)(PP_Resource stream,
                            PP_StreamInitializationMode mode,
                            struct PP_CompletionCallback callback);
  /**
   * Appends Elementary Stream packet.
   *
   * Before appending any packet to the buffer, it must be properly configured
   * (see <code>InitializeDone</code>).
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the
   * elementary stream.
   * @param[in] packet A <code>PP_ESPacket</code> containing Elementary Stream
   * packet data and metadata.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   * Returns PP_ERROR_FAILED if InitializeDone() has not successfully completed.
   * Returns PP_ERROR_NOQUOTA if internal buffer is full.
   */
  int32_t (*AppendPacket)(PP_Resource stream,
                          const struct PP_ESPacket* packet,
                          struct PP_CompletionCallback callback);
  /**
   * Appends Elementary Stream encrypted packet.
   *
   * Before appending any packet to the buffer, it must be properly configured
   * (see <code>InitializeDone</code>).
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the
   * elementary stream.
   * @param[in] packet A <code>PP_ESPacket</code> containing Elementary Stream
   * packet data and metadata.
   * @param[in] encryption_info A <code>PP_ESPacketEncryptionInfo</code>
   * containing packet encryption description.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   * Returns PP_ERROR_FAILED if InitializeDone() has not successfully completed.
   * Returns PP_ERROR_NOQUOTA if internal buffer is full.
   */
  int32_t (*AppendEncryptedPacket)(
      PP_Resource stream,
      const struct PP_ESPacket* packet,
      const struct PP_ESPacketEncryptionInfo* encryption_info,
      struct PP_CompletionCallback callback);
  /**
   * Appends Elementary Stream packet.
   *
   * Before appending any packet to the buffer, it must be properly configured
   * (see <code>InitializeDone</code>).
   *
   * Note:
   * After successful append ownership of the TrustZone memory is transfered
   * from the module to the browser and handle must not be released.
   * But in case of any error (i.e. return code/callback call with status
   * other than PP_OK) the module is responsible for releasing memory
   * reference.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the
   * elementary stream.
   * @param[in] packet A <code>PP_ESPacket</code> containing Elementary Stream
   * metadata. Note: when using this method of appending packets
   * <code>size</code> and <code>buffer</code> fields of
   * <code>PP_ESPacket</code> struct must be set to 0 and NULL. If they are
   * not set to zero and NULL PP_ERROR_BADARGUMENT will be returned.
   * @param[in] handle A handle to TrustZone memory containing packet data
   * which will be appended.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   * Returns PP_ERROR_FAILED if InitializeDone() has not successfully completed.
   * Returns PP_ERROR_NOQUOTA if internal buffer is full.
   * Returns PP_ERROR_BADARGUMENT if either <code>size</code> or
   * <code>buffer</code> are not set to 0.
   */
  int32_t (*AppendTrustZonePacket)(PP_Resource stream,
                                   const struct PP_ESPacket* packet,
                                   const struct PP_TrustZoneReference* handle,
                                   struct PP_CompletionCallback callback);
  /**
   * Flushes all appended, but not decoded or rendered packets to this buffer.
   * This method is usually called during seek operations.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the
   * elementary stream.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   * Returns PP_ERROR_FAILED if InitializeDone() has not successfully completed.
   */
  int32_t (*Flush)(PP_Resource stream, struct PP_CompletionCallback callback);
  /**
   * Found DRM system initialization metadata. |type| describes type
   * of the initialization data |init_data| associated with the stream.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the
   * elementary stream.
   * @param[in] type A string describing type of an |init_data|.
   *                 Examples:
   *                 - "cenc:pssh" - |init_data| will contain PSSH box as
   *                   described by Common Encryption specifiacation
   *                 - "mspr:pro" - |init_data| will contain Microsoft
   *                   PlayReady Object Header (PRO).
   * @param[in] init_data_size A size of DRM specific |init_data| buffer
   * @param[in] init_data A buffer containing DRM system initialization data.
   * @param[in] callback A <code>PP_CompletionCallback</code> to be called upon
   * completion.
   *
   * @return PP_OK on success, otherwise an error code from
   * <code>pp_errors.h</code>.
   */
  int32_t (*SetDRMInitData)(PP_Resource stream,
                            const char* type,
                            uint32_t init_data_size,
                            const void* init_data,
                            struct PP_CompletionCallback callback);
};

typedef struct PPB_ElementaryStream_Samsung_1_1 PPB_ElementaryStream_Samsung;

struct PPB_ElementaryStream_Samsung_1_0 {
  PP_Bool (*IsElementaryStream)(PP_Resource resource);
  PP_ElementaryStream_Type_Samsung (*GetStreamType)(PP_Resource resource);
  int32_t (*InitializeDone)(PP_Resource stream,
                            struct PP_CompletionCallback callback);
  int32_t (*AppendPacket)(PP_Resource stream,
                          const struct PP_ESPacket* packet,
                          struct PP_CompletionCallback callback);
  int32_t (*AppendEncryptedPacket)(
      PP_Resource stream,
      const struct PP_ESPacket* packet,
      const struct PP_ESPacketEncryptionInfo* encryption_info,
      struct PP_CompletionCallback callback);
  int32_t (*Flush)(PP_Resource stream, struct PP_CompletionCallback callback);
  int32_t (*SetDRMInitData)(PP_Resource stream,
                            const char* type,
                            uint32_t init_data_size,
                            const void* init_data,
                            struct PP_CompletionCallback callback);
};

/**
 * Interface representing an audio elementary stream and containing methods
 * to set audio codec specific configuration.
 *
 * All pending configuration changes/initialization must be confirmed
 * by call to <code>PPB_ElementaryStream_Samsung.InitializeDone</code>.
 *
 * All getters return last set configuration, which might be not confirmed yet.
 */
struct PPB_AudioElementaryStream_Samsung_1_0 {
  /**
   * Determines if the given resource is an audio elementary stream.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_AudioElementaryStream_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsAudioElementaryStream)(PP_Resource resource);
  /**
   * Retrieves current audio codec type.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   *
   * @return Current audio codec type.
   */
  PP_AudioCodec_Type_Samsung (*GetAudioCodecType)(PP_Resource stream);
  /**
   * Sets new audio codec type.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   * @param[in] audio_codec New audio codec type.
   */
  void (*SetAudioCodecType)(PP_Resource stream,
                            PP_AudioCodec_Type_Samsung audio_codec);
  /**
   * Retrieves current audio codec profile.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   *
   * @return Current audio codec profile.
   */
  PP_AudioCodec_Profile_Samsung (*GetAudioCodecProfile)(PP_Resource stream);
  /**
   * Sets new audio codec profile.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   * @param[in] profile New audio codec profile.
   */
  void (*SetAudioCodecProfile)(PP_Resource stream,
                               PP_AudioCodec_Profile_Samsung profile);
  /**
   * Retrieves current audio sample format.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   *
   * @return Current audio sample format.
   */
  PP_SampleFormat_Samsung (*GetSampleFormat)(PP_Resource stream);
  /**
   * Sets new audio sample format.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   * @param[in] sample_format New audio sample format.
   */
  void (*SetSampleFormat)(PP_Resource stream,
                          PP_SampleFormat_Samsung sample_format);
  /**
   * Retrieves current audio channel layout.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   *
   * @return current audio channel layout.
   */
  PP_ChannelLayout_Samsung (*GetChannelLayout)(PP_Resource stream);
  /**
   * Sets new audio channel layout.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   * @param[in] audio_codec New audio channel layout.
   */
  void (*SetChannelLayout)(PP_Resource stream,
                           PP_ChannelLayout_Samsung channel_layout);
  /**
   * Retrieves how many bits are used to represent audio sample
   * per each channel.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   *
   * @return How many bits are used to represent audio sample per each channel.
   */
  int32_t (*GetBitsPerChannel)(PP_Resource stream);
  /**
   * Sets how many bits are used to represent audio sample per each channel.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   * @param[in] bits_per_channel Value representing how many bits are used
   * to represent audio sample per each channel.
   */
  void (*SetBitsPerChannel)(PP_Resource stream, int32_t bits_per_channel);
  /**
   * Retrieves how many audio samples were recorded per second (sample rate).
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   *
   * @return Current audio samples per second.
   */
  int32_t (*GetSamplesPerSecond)(PP_Resource stream);
  /**
   * Sets how many audio samples were recorded per second (sample rate).
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   * @param[in] samples_per_second Value representing audio samples per second.
   */
  void (*SetSamplesPerSecond)(PP_Resource stream, int32_t samples_per_second);
  /**
   * Sets audio codec specific extra data. Those data are needed by audio codec
   * to initialize properly audio decoding.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the audio
   * elementary stream.
   * @param[in] extra_data_size Size in bytes of |extra_data| buffer.
   * @param[in] extra_data A pointer to the buffer containing audio codec
   * specific extra data.
   */
  void (*SetCodecExtraData)(PP_Resource stream,
                            uint32_t extra_data_size,
                            const void* extra_data);
};

typedef struct PPB_AudioElementaryStream_Samsung_1_0
    PPB_AudioElementaryStream_Samsung;

/**
 * Interface representing an video elementary stream and containing methods
 * to set video codec specific configuration.
 *
 * All pending configuration changes/initialization must be confirmed
 * by call to <code>PPB_ElementaryStream_Samsung.InitializeDone</code>.
 *
 * All getters return last set configuration, which might be not confirmed yet.
 */
struct PPB_VideoElementaryStream_Samsung_1_0 {
  /**
   * Determines if the given resource is an video elementary stream.
   *
   * @param[in] resource A <code>PP_Resource</code> identifying a resource.
   *
   * @return <code>PP_TRUE</code> if the resource is a
   * <code>PPB_VideoElementaryStream_Samsung</code>, <code>PP_FALSE</code>
   * if the resource is invalid or some other type.
   */
  PP_Bool (*IsVideoElementaryStream)(PP_Resource resource);
  /**
   * Retrieves current video codec type.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   *
   * @return Current video codec type.
   */
  PP_VideoCodec_Type_Samsung (*GetVideoCodecType)(PP_Resource stream);
  /**
   * Sets new video codec type.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[in] audio_codec New video codec type.
   */
  void (*SetVideoCodecType)(PP_Resource stream,
                            PP_VideoCodec_Type_Samsung video_codec);
  /**
   * Retrieves current video codec profile.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   *
   * @return Current audio codec profile.
   */
  PP_VideoCodec_Profile_Samsung (*GetVideoCodecProfile)(PP_Resource stream);
  /**
   * Sets new video codec profile.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[in] profile New video codec profile.
   */
  void (*SetVideoCodecProfile)(PP_Resource stream,
                               PP_VideoCodec_Profile_Samsung profile);
  /**
   * Retrieves current video frame format.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   *
   * @return Current video frame format.
   */
  PP_VideoFrame_Format_Samsung (*GetVideoFrameFormat)(PP_Resource stream);
  /**
   * Sets new video frame format.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[in] frame_format New video frame format.
   */
  void (*SetVideoFrameFormat)(PP_Resource stream,
                              PP_VideoFrame_Format_Samsung frame_format);
  /**
   * Retrieves current video frame size in pixels.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[out] size Current video frame size in pixels.
   */
  void (*GetVideoFrameSize)(PP_Resource stream, struct PP_Size* size);
  /**
   * Sets new video frame size in pixels.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[in] size New video frame size in pixels.
   */
  void (*SetVideoFrameSize)(PP_Resource stream, const struct PP_Size* size);
  /**
   * Retrieves current video frame rate as rational number represented by
   * fraction |numerator| / |denominator|.
   *
   * Both |numerator| and |denominator| must be non-null, otherwise no
   * information is retrieved.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[out] numerator A dividend of the quotient.
   * @param[out] denominator A divisor of the quotient.
   */
  void (*GetFrameRate)(PP_Resource stream,
                       uint32_t* numerator,
                       uint32_t* denominator);
  /**
   * Sets new video frame rate as rational number represented by
   * fraction |numerator| / |denominator|.
   *
   * Both |numerator| and |denominator| must be non-null, otherwise no
   * information is set. Additionally |denominator| must be positive (!= 0).
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[in] numerator A dividend of the quotient.
   * @param[in] denominator A divisor of the quotient.
   */
  void (*SetFrameRate)(PP_Resource stream,
                       uint32_t numerator,
                       uint32_t denominator);
  /**
   * Sets video codec specific extra data. Those data are needed by video codec
   * to initialize properly video decoding.
   *
   * @param[in] stream A <code>PP_Resource</code> identifying the video
   * elementary stream.
   * @param[in] extra_data_size Size in bytes of |extra_data| buffer.
   * @param[in] extra_data A pointer to the buffer containing video codec
   * specific extra data.
   */
  void (*SetCodecExtraData)(PP_Resource stream,
                            uint32_t extra_data_size,
                            const void* extra_data);
};

typedef struct PPB_VideoElementaryStream_Samsung_1_0
    PPB_VideoElementaryStream_Samsung;
/**
 * @}
 */

#endif  /* PPAPI_C_SAMSUNG_PPB_MEDIA_DATA_SOURCE_SAMSUNG_H_ */

