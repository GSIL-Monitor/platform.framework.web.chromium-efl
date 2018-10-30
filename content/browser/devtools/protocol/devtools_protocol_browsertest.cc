// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/sys_info.h"
#include "base/values.h"
#include "build/build_config.h"
#include "content/browser/devtools/protocol/devtools_download_manager_delegate.h"
#include "content/browser/download/download_file_factory.h"
#include "content/browser/download/download_file_impl.h"
#include "content/browser/download/download_manager_impl.h"
#include "content/browser/download/download_task_runner.h"
#include "content/browser/frame_host/interstitial_page_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_base.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/devtools_agent_host.h"
#include "content/public/browser/download_manager.h"
#include "content/public/browser/interstitial_page_delegate.h"
#include "content/public/browser/javascript_dialog_manager.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/security_style_explanations.h"
#include "content/public/browser/ssl_status.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "content/public/test/browser_test_utils.h"
#include "content/public/test/content_browser_test.h"
#include "content/public/test/content_browser_test_utils.h"
#include "content/public/test/download_test_observer.h"
#include "content/public/test/test_navigation_observer.h"
#include "content/shell/browser/shell.h"
#include "content/shell/browser/shell_browser_context.h"
#include "content/shell/browser/shell_content_browser_client.h"
#include "content/shell/browser/shell_download_manager_delegate.h"
#include "content/test/content_browser_test_utils_internal.h"
#include "net/dns/mock_host_resolver.h"
#include "net/test/cert_test_util.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "net/test/test_data_directory.h"
#include "net/test/url_request/url_request_mock_http_job.h"
#include "net/test/url_request/url_request_slow_download_job.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkColor.h"
#include "ui/base/layout.h"
#include "ui/compositor/compositor_switches.h"
#include "ui/gfx/codec/jpeg_codec.h"
#include "ui/gfx/codec/png_codec.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/skia_util.h"
#include "ui/snapshot/snapshot.h"

#define EXPECT_SIZE_EQ(expected, actual)               \
  do {                                                 \
    EXPECT_EQ((expected).width(), (actual).width());   \
    EXPECT_EQ((expected).height(), (actual).height()); \
  } while (false)

using testing::ElementsAre;

namespace content {

namespace {

const char kIdParam[] = "id";
const char kMethodParam[] = "method";
const char kParamsParam[] = "params";

// If |params| contains an explanation with a non-empty certificate list,
// returns true and points |certificate| to the certificate list of the first
// explanation that contains a nonempty certificate list. Otherwise returns
// false. |params| is expected to be the parameters of a securityStateChanged
// notification.
bool GetCertificateFromNotificationParams(base::DictionaryValue* params,
                                          const base::ListValue** certificate) {
  const base::ListValue* explanations;
  if (!params->GetList("explanations", &explanations)) {
    return false;
  }
  for (const auto& explanation : *explanations) {
    const base::DictionaryValue* explanation_dict;
    if (explanation.GetAsDictionary(&explanation_dict) &&
        explanation_dict->GetList("certificate", certificate) &&
        (*certificate)->GetSize() > 0u) {
      return true;
    }
  }
  return false;
}

bool SecurityStateChangedHasCertificateExplanation(
    base::DictionaryValue* params) {
  const base::ListValue* unused;
  return GetCertificateFromNotificationParams(params, &unused);
}

class TestJavaScriptDialogManager : public JavaScriptDialogManager,
                                    public WebContentsDelegate {
 public:
  TestJavaScriptDialogManager() {}
  ~TestJavaScriptDialogManager() override {}

  void Handle() {
    if (!callback_.is_null()) {
      std::move(callback_).Run(true, base::string16());
    } else {
      handle_ = true;
    }
  }

  // WebContentsDelegate
  JavaScriptDialogManager* GetJavaScriptDialogManager(
      WebContents* source) override {
    return this;
  }

  // JavaScriptDialogManager
  void RunJavaScriptDialog(WebContents* web_contents,
                           const GURL& alerting_frame_url,
                           JavaScriptDialogType dialog_type,
                           const base::string16& message_text,
                           const base::string16& default_prompt_text,
                           DialogClosedCallback callback,
                           bool* did_suppress_message) override {
    if (handle_) {
      handle_ = false;
      std::move(callback).Run(true, base::string16());
    } else {
      callback_ = std::move(callback);
    }
  };

  void RunBeforeUnloadDialog(WebContents* web_contents,
                             bool is_reload,
                             DialogClosedCallback callback) override {}

  bool HandleJavaScriptDialog(WebContents* web_contents,
                              bool accept,
                              const base::string16* prompt_override) override {
    is_handled_ = true;
    return true;
  }

  void CancelDialogs(WebContents* web_contents,
                     bool reset_state) override {}

  bool is_handled() { return is_handled_; }

 private:
  DialogClosedCallback callback_;
  bool handle_ = false;
  bool is_handled_ = false;
  DISALLOW_COPY_AND_ASSIGN(TestJavaScriptDialogManager);
};

}  // namespace

class DevToolsProtocolTest : public ContentBrowserTest,
                             public DevToolsAgentHostClient,
                             public WebContentsDelegate {
 public:
  typedef base::Callback<bool(base::DictionaryValue*)> NotificationMatcher;

  DevToolsProtocolTest()
      : last_sent_id_(0),
        waiting_for_command_result_id_(0),
        in_dispatch_(false),
        agent_host_can_close_(false) {}

  void SetUpOnMainThread() override {
    host_resolver()->AddRule("*", "127.0.0.1");
  }

 protected:
  // WebContentsDelegate methods:
  bool DidAddMessageToConsole(WebContents* source,
                              int32_t level,
                              const base::string16& message,
                              int32_t line_no,
                              const base::string16& source_id) override {
    console_messages_.push_back(base::UTF16ToUTF8(message));
    return true;
  }

  blink::WebSecurityStyle GetSecurityStyle(
      content::WebContents* web_contents,
      content::SecurityStyleExplanations* security_style_explanations)
      override {
    security_style_explanations->secure_explanations.push_back(
        SecurityStyleExplanation(
            "an explanation", "an explanation description", cert_,
            blink::WebMixedContentContextType::kNotMixedContent));
    return blink::kWebSecurityStyleNeutral;
  }

  base::DictionaryValue* SendCommand(
      const std::string& method,
      std::unique_ptr<base::DictionaryValue> params) {
    return SendCommand(method, std::move(params), true);
  }

  base::DictionaryValue* SendCommand(
      const std::string& method,
      std::unique_ptr<base::DictionaryValue> params,
      bool wait) {
    in_dispatch_ = true;
    base::DictionaryValue command;
    command.SetInteger(kIdParam, ++last_sent_id_);
    command.SetString(kMethodParam, method);
    if (params)
      command.Set(kParamsParam, std::move(params));

    std::string json_command;
    base::JSONWriter::Write(command, &json_command);
    agent_host_->DispatchProtocolMessage(this, json_command);
    // Some messages are dispatched synchronously.
    // Only run loop if we are not finished yet.
    if (in_dispatch_ && wait) {
      WaitForResponse();
      in_dispatch_ = false;
      return result_.get();
    }
    in_dispatch_ = false;
    return nullptr;
  }

  void WaitForResponse() {
    waiting_for_command_result_id_ = last_sent_id_;
    base::RunLoop().Run();
  }

  bool HasValue(const std::string& path) {
    base::Value* value = 0;
    return result_->Get(path, &value);
  }

  bool HasListItem(const std::string& path_to_list,
                   const std::string& name,
                   const std::string& value) {
    base::ListValue* list;
    if (!result_->GetList(path_to_list, &list))
      return false;

    for (size_t i = 0; i != list->GetSize(); i++) {
      base::DictionaryValue* item;
      if (!list->GetDictionary(i, &item))
        return false;
      std::string id;
      if (!item->GetString(name, &id))
        return false;
      if (id == value)
        return true;
    }
    return false;
  }

  void Attach() {
    agent_host_ = DevToolsAgentHost::GetOrCreateFor(shell()->web_contents());
    agent_host_->AttachClient(this);
    shell()->web_contents()->SetDelegate(this);
  }

  void TearDownOnMainThread() override {
    if (agent_host_) {
      agent_host_->DetachClient(this);
      agent_host_ = nullptr;
    }
  }

  std::unique_ptr<base::DictionaryValue> WaitForNotification(
      const std::string& notification) {
    return WaitForNotification(notification, false);
  }

  std::unique_ptr<base::DictionaryValue> WaitForNotification(
      const std::string& notification,
      bool allow_existing) {
    if (allow_existing) {
      for (size_t i = 0; i < notifications_.size(); i++) {
        if (notifications_[i] == notification) {
          std::unique_ptr<base::DictionaryValue> result =
              std::move(notification_params_[i]);
          notifications_.erase(notifications_.begin() + i);
          notification_params_.erase(notification_params_.begin() + i);
          return result;
        }
      }
    }

    waiting_for_notification_ = notification;
    RunMessageLoop();
    return std::move(waiting_for_notification_params_);
  }

  // Waits for a notification whose params, when passed to |matcher|, returns
  // true. Existing notifications are allowed.
  std::unique_ptr<base::DictionaryValue> WaitForMatchingNotification(
      const std::string& notification,
      const NotificationMatcher& matcher) {
    for (size_t i = 0; i < notifications_.size(); i++) {
      if (notifications_[i] == notification &&
          matcher.Run(notification_params_[i].get())) {
        std::unique_ptr<base::DictionaryValue> result =
            std::move(notification_params_[i]);
        notifications_.erase(notifications_.begin() + i);
        notification_params_.erase(notification_params_.begin() + i);
        return result;
      }
    }

    waiting_for_notification_ = notification;
    waiting_for_notification_matcher_ = matcher;
    RunMessageLoop();
    return std::move(waiting_for_notification_params_);
  }

  void ClearNotifications() {
    notifications_.clear();
    notification_params_.clear();
  }

  struct ExpectedNavigation {
    std::string url;
    bool is_redirect;
    bool abort;
  };

  std::string RemovePort(const GURL& url) {
    GURL::Replacements remove_port;
    remove_port.ClearPort();
    return url.ReplaceComponents(remove_port).spec();
  }

  // Waits for the expected navigations to occur in any order. If an expected
  // navigation occurs, Network.continueInterceptedRequest is called with the
  // specified navigation_response to either allow it to proceed or to cancel
  // it.
  void ProcessNavigationsAnyOrder(
      std::vector<ExpectedNavigation> expected_navigations) {
    std::unique_ptr<base::DictionaryValue> params;
    while (!expected_navigations.empty()) {
      std::unique_ptr<base::DictionaryValue> params =
          WaitForNotification("Network.requestIntercepted");

      std::string interception_id;
      ASSERT_TRUE(params->GetString("interceptionId", &interception_id));
      bool is_redirect = params->HasKey("redirectUrl");
      bool is_navigation;
      ASSERT_TRUE(params->GetBoolean("isNavigationRequest", &is_navigation));
      std::string resource_type;
      ASSERT_TRUE(params->GetString("resourceType", &resource_type));
      std::string url;
      ASSERT_TRUE(params->GetString("request.url", &url));
      if (is_redirect)
        ASSERT_TRUE(params->GetString("redirectUrl", &url));
      // The url will typically have a random port which we want to remove.
      url = RemovePort(GURL(url));

      if (!is_navigation) {
        params.reset(new base::DictionaryValue());
        params->SetString("interceptionId", interception_id);
        SendCommand("Network.continueInterceptedRequest", std::move(params),
                    false);
        continue;
      }

      bool navigation_was_expected = false;
      for (auto it = expected_navigations.begin();
           it != expected_navigations.end(); it++) {
        if (url != it->url || is_redirect != it->is_redirect)
          continue;

        params.reset(new base::DictionaryValue());
        params->SetString("interceptionId", interception_id);
        if (it->abort)
          params->SetString("errorReason", "Aborted");
        SendCommand("Network.continueInterceptedRequest", std::move(params),
                    false);

        navigation_was_expected = true;
        expected_navigations.erase(it);
        break;
      }
      EXPECT_TRUE(navigation_was_expected)
          << "url = " << url << "is_redirect = " << is_redirect;
    }
  }

  std::vector<std::string> GetAllFrameUrls() {
    std::vector<std::string> urls;
    for (RenderFrameHost* render_frame_host :
         shell()->web_contents()->GetAllFrames()) {
      urls.push_back(RemovePort(render_frame_host->GetLastCommittedURL()));
    }
    return urls;
  }

  void set_agent_host_can_close() { agent_host_can_close_ = true; }

  void SetSecurityExplanationCert(
      const scoped_refptr<net::X509Certificate>& cert) {
    cert_ = cert;
  }

  std::unique_ptr<base::DictionaryValue> result_;
  scoped_refptr<DevToolsAgentHost> agent_host_;
  int last_sent_id_;
  std::vector<int> result_ids_;
  std::vector<std::string> notifications_;
  std::vector<std::string> console_messages_;
  std::vector<std::unique_ptr<base::DictionaryValue>> notification_params_;

 private:
  void DispatchProtocolMessage(DevToolsAgentHost* agent_host,
                               const std::string& message) override {
    std::unique_ptr<base::DictionaryValue> root(
        static_cast<base::DictionaryValue*>(
            base::JSONReader::Read(message).release()));
    int id;
    if (root->GetInteger("id", &id)) {
      result_ids_.push_back(id);
      base::DictionaryValue* result;
      ASSERT_TRUE(root->GetDictionary("result", &result));
      result_.reset(result->DeepCopy());
      in_dispatch_ = false;
      if (id && id == waiting_for_command_result_id_) {
        waiting_for_command_result_id_ = 0;
        base::RunLoop::QuitCurrentDeprecated();
      }
    } else {
      std::string notification;
      EXPECT_TRUE(root->GetString("method", &notification));
      notifications_.push_back(notification);
      base::DictionaryValue* params;
      if (root->GetDictionary("params", &params)) {
        notification_params_.push_back(params->CreateDeepCopy());
      } else {
        notification_params_.push_back(
            base::WrapUnique(new base::DictionaryValue()));
      }
      if (waiting_for_notification_ == notification &&
          (waiting_for_notification_matcher_.is_null() ||
           waiting_for_notification_matcher_.Run(
               notification_params_[notification_params_.size() - 1].get()))) {
        waiting_for_notification_ = std::string();
        waiting_for_notification_matcher_ = NotificationMatcher();
        waiting_for_notification_params_ = base::WrapUnique(
            notification_params_[notification_params_.size() - 1]->DeepCopy());
        base::RunLoop::QuitCurrentDeprecated();
      }
    }
  }

  void AgentHostClosed(DevToolsAgentHost* agent_host, bool replaced) override {
    if (!agent_host_can_close_)
      NOTREACHED();
  }

  std::string waiting_for_notification_;
  NotificationMatcher waiting_for_notification_matcher_;
  std::unique_ptr<base::DictionaryValue> waiting_for_notification_params_;
  int waiting_for_command_result_id_;
  bool in_dispatch_;
  bool agent_host_can_close_;
  scoped_refptr<net::X509Certificate> cert_;
};

class TestInterstitialDelegate : public InterstitialPageDelegate {
 private:
  // InterstitialPageDelegate:
  std::string GetHTMLContents() override { return "<p>Interstitial</p>"; }
};

class SyntheticKeyEventTest : public DevToolsProtocolTest {
 protected:
  void SendKeyEvent(const std::string& type,
                    int modifier,
                    int windowsKeyCode,
                    int nativeKeyCode,
                    const std::string& key,
                    bool wait) {
    std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
    params->SetString("type", type);
    params->SetInteger("modifiers", modifier);
    params->SetInteger("windowsVirtualKeyCode", windowsKeyCode);
    params->SetInteger("nativeVirtualKeyCode", nativeKeyCode);
    params->SetString("key", key);
    SendCommand("Input.dispatchKeyEvent", std::move(params), wait);
  }
};

class SyntheticMouseEventTest : public DevToolsProtocolTest {
 protected:
  void SendMouseEvent(const std::string& type, int x, int y, bool wait) {
    std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
    params->SetString("type", type);
    params->SetInteger("x", x);
    params->SetInteger("y", y);
    SendCommand("Input.dispatchMouseEvent", std::move(params), wait);
  }
};

IN_PROC_BROWSER_TEST_F(SyntheticKeyEventTest, KeyEventSynthesizeKey) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  ASSERT_TRUE(content::ExecuteScript(
      shell()->web_contents()->GetRenderViewHost(),
      "function handleKeyEvent(event) {"
        "domAutomationController.send(event.key);"
      "}"
      "document.body.addEventListener('keydown', handleKeyEvent);"
      "document.body.addEventListener('keyup', handleKeyEvent);"));

  DOMMessageQueue dom_message_queue;

  // Send enter (keycode 13).
  SendKeyEvent("rawKeyDown", 0, 13, 13, "Enter", true);
  SendKeyEvent("keyUp", 0, 13, 13, "Enter", true);

  std::string key;
  ASSERT_TRUE(dom_message_queue.WaitForMessage(&key));
  EXPECT_EQ("\"Enter\"", key);
  ASSERT_TRUE(dom_message_queue.WaitForMessage(&key));
  EXPECT_EQ("\"Enter\"", key);

  // Send escape (keycode 27).
  SendKeyEvent("rawKeyDown", 0, 27, 27, "Escape", true);
  SendKeyEvent("keyUp", 0, 27, 27, "Escape", true);

  ASSERT_TRUE(dom_message_queue.WaitForMessage(&key));
  EXPECT_EQ("\"Escape\"", key);
  ASSERT_TRUE(dom_message_queue.WaitForMessage(&key));
  EXPECT_EQ("\"Escape\"", key);
}

IN_PROC_BROWSER_TEST_F(SyntheticKeyEventTest, KeyboardEventAck) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  ASSERT_TRUE(content::ExecuteScript(
      shell()->web_contents()->GetRenderViewHost(),
      "document.body.addEventListener('keydown', () => console.log('x'));"));

  scoped_refptr<InputMsgWatcher> filter = new InputMsgWatcher(
      RenderWidgetHostImpl::From(
          shell()->web_contents()->GetRenderViewHost()->GetWidget()),
      blink::WebInputEvent::kMouseMove);

  SendCommand("Runtime.enable", nullptr);
  SendKeyEvent("rawKeyDown", 0, 13, 13, "Enter", false);

  // We expect that the console log message event arrives *before* the input
  // event ack, and the subsequent command response for Input.dispatchKeyEvent.
  WaitForNotification("Runtime.consoleAPICalled");
  EXPECT_THAT(console_messages_, ElementsAre("x"));
  EXPECT_FALSE(filter->HasReceivedAck());
  EXPECT_EQ(1u, result_ids_.size());

  WaitForResponse();
  EXPECT_EQ(2u, result_ids_.size());
}

IN_PROC_BROWSER_TEST_F(SyntheticMouseEventTest, MouseEventAck) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  ASSERT_TRUE(content::ExecuteScript(
      shell()->web_contents()->GetRenderViewHost(),
      "document.body.addEventListener('mousemove', () => console.log('x'));"));

  scoped_refptr<InputMsgWatcher> filter = new InputMsgWatcher(
      RenderWidgetHostImpl::From(
          shell()->web_contents()->GetRenderViewHost()->GetWidget()),
      blink::WebInputEvent::kMouseMove);

  SendCommand("Runtime.enable", nullptr);
  SendMouseEvent("mouseMoved", 15, 15, false);

  // We expect that the console log message event arrives *before* the input
  // event ack, and the subsequent command response for
  // Input.dispatchMouseEvent.
  WaitForNotification("Runtime.consoleAPICalled");
  EXPECT_THAT(console_messages_, ElementsAre("x"));
  EXPECT_FALSE(filter->HasReceivedAck());
  EXPECT_EQ(1u, result_ids_.size());

  WaitForResponse();
  EXPECT_EQ(2u, result_ids_.size());
}

namespace {
bool DecodePNG(std::string base64_data, SkBitmap* bitmap) {
  std::string png_data;
  if (!base::Base64Decode(base64_data, &png_data))
    return false;
  return gfx::PNGCodec::Decode(
      reinterpret_cast<unsigned const char*>(png_data.data()), png_data.size(),
      bitmap);
}

std::unique_ptr<SkBitmap> DecodeJPEG(std::string base64_data) {
  std::string jpeg_data;
  if (!base::Base64Decode(base64_data, &jpeg_data))
    return nullptr;
  return gfx::JPEGCodec::Decode(
      reinterpret_cast<unsigned const char*>(jpeg_data.data()),
      jpeg_data.size());
}

bool ColorsMatchWithinLimit(SkColor color1,
                            SkColor color2,
                            int32_t error_limit) {
  auto a_distance = std::abs(static_cast<int32_t>(SkColorGetA(color1)) -
                             static_cast<int32_t>(SkColorGetA(color2)));
  auto r_distance = std::abs(static_cast<int32_t>(SkColorGetR(color1)) -
                             static_cast<int32_t>(SkColorGetR(color2)));
  auto g_distance = std::abs(static_cast<int32_t>(SkColorGetG(color1)) -
                             static_cast<int32_t>(SkColorGetG(color2)));
  auto b_distance = std::abs(static_cast<int32_t>(SkColorGetB(color1)) -
                             static_cast<int32_t>(SkColorGetB(color2)));

  return a_distance * a_distance + r_distance * r_distance +
             g_distance * g_distance + b_distance * b_distance <=
         error_limit * error_limit;
}

// Adapted from cc::ExactPixelComparator.
bool MatchesBitmap(const SkBitmap& expected_bmp,
                   const SkBitmap& actual_bmp,
                   const gfx::Rect& matching_mask,
                   float device_scale_factor,
                   int error_limit) {
  // Number of pixels with an error
  int error_pixels_count = 0;

  gfx::Rect error_bounding_rect = gfx::Rect();

  // Scale expectations along with the mask.
  device_scale_factor = device_scale_factor ? device_scale_factor : 1;

  // Check that bitmaps have identical dimensions.
  EXPECT_EQ(expected_bmp.width() * device_scale_factor, actual_bmp.width());
  EXPECT_EQ(expected_bmp.height() * device_scale_factor, actual_bmp.height());
  if (expected_bmp.width() * device_scale_factor != actual_bmp.width() ||
      expected_bmp.height() * device_scale_factor != actual_bmp.height()) {
    return false;
  }

  DCHECK(gfx::SkIRectToRect(actual_bmp.bounds()).Contains(matching_mask));

  for (int x = matching_mask.x(); x < matching_mask.right(); ++x) {
    for (int y = matching_mask.y(); y < matching_mask.bottom(); ++y) {
      SkColor actual_color =
          actual_bmp.getColor(x * device_scale_factor, y * device_scale_factor);
      SkColor expected_color = expected_bmp.getColor(x, y);
      if (!ColorsMatchWithinLimit(actual_color, expected_color, error_limit)) {
        if (error_pixels_count < 10) {
          LOG(ERROR) << "Pixel (" << x << "," << y << "): expected "
                     << expected_color << " actual " << actual_color;
        }
        error_pixels_count++;
        error_bounding_rect.Union(gfx::Rect(x, y, 1, 1));
      }
    }
  }

  if (error_pixels_count != 0) {
    LOG(ERROR) << "Number of pixel with an error: " << error_pixels_count;
    LOG(ERROR) << "Error Bounding Box : " << error_bounding_rect.ToString();
    return false;
  }

  return true;
}
}  // namespace

class CaptureScreenshotTest : public DevToolsProtocolTest {
 protected:
  enum ScreenshotEncoding { ENCODING_PNG, ENCODING_JPEG };
  void CaptureScreenshotAndCompareTo(const SkBitmap& expected_bitmap,
                                     ScreenshotEncoding encoding,
                                     bool from_surface,
                                     float device_scale_factor = 0,
                                     const gfx::RectF& clip = gfx::RectF(),
                                     float clip_scale = 0) {
    std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
    params->SetString("format", encoding == ENCODING_PNG ? "png" : "jpeg");
    params->SetInteger("quality", 100);
    params->SetBoolean("fromSurface", from_surface);
    if (clip_scale) {
      std::unique_ptr<base::DictionaryValue> clip_value(
          new base::DictionaryValue());
      clip_value->SetDouble("x", clip.x());
      clip_value->SetDouble("y", clip.y());
      clip_value->SetDouble("width", clip.width());
      clip_value->SetDouble("height", clip.height());
      clip_value->SetDouble("scale", clip_scale);
      params->Set("clip", std::move(clip_value));
    }
    SendCommand("Page.captureScreenshot", std::move(params));

    std::string base64;
    EXPECT_TRUE(result_->GetString("data", &base64));
    std::unique_ptr<SkBitmap> result_bitmap;
    int error_limit = 0;
    if (encoding == ENCODING_PNG) {
      result_bitmap.reset(new SkBitmap());
      EXPECT_TRUE(DecodePNG(base64, result_bitmap.get()));
    } else {
      result_bitmap = DecodeJPEG(base64);
      // Even with quality 100, jpeg isn't lossless. So, we allow some skew in
      // pixel values. Not that this assumes that there is no skew in pixel
      // positions, so will only work reliably if all pixels have equal values.
      error_limit = 3;
    }
    EXPECT_TRUE(result_bitmap);

    gfx::Rect matching_mask(gfx::SkIRectToRect(expected_bitmap.bounds()));
#if defined(OS_MACOSX)
    // Mask out the corners, which may be drawn differently on Mac because of
    // rounded corners.
    matching_mask.Inset(4, 4, 4, 4);
#endif
    EXPECT_TRUE(MatchesBitmap(expected_bitmap, *result_bitmap, matching_mask,
                              device_scale_factor, error_limit));
  }

  // Takes a screenshot of a colored box that is positioned inside the frame.
  void PlaceAndCaptureBox(const gfx::Size& frame_size,
                          const gfx::Size& box_size,
                          float screenshot_scale,
                          float device_scale_factor) {
    static const int kBoxOffsetHeight = 100;
    const gfx::Size scaled_box_size =
        ScaleToFlooredSize(box_size, screenshot_scale);
    std::unique_ptr<base::DictionaryValue> params;

    VLOG(1) << "Testing screenshot of box with size " << box_size.width() << "x"
            << box_size.height() << "px at scale " << screenshot_scale
            << " ...";

    // Draw a blue box of provided size in the horizontal center of the page.
    EXPECT_TRUE(content::ExecuteScript(
        shell()->web_contents()->GetRenderViewHost(),
        base::StringPrintf(
            "var style = document.body.style;                             "
            "style.overflow = 'hidden';                                   "
            "style.minHeight = '%dpx';                                    "
            "style.backgroundImage = 'linear-gradient(#0000ff, #0000ff)'; "
            "style.backgroundSize = '%dpx %dpx';                          "
            "style.backgroundPosition = '50%% %dpx';                      "
            "style.backgroundRepeat = 'no-repeat';                        ",
            box_size.height() + kBoxOffsetHeight, box_size.width(),
            box_size.height(), kBoxOffsetHeight)));

    // Force frame size: The offset of the blue box within the frame shouldn't
    // change during screenshotting. This verifies that the page doesn't observe
    // a change in frame size as a side effect of screenshotting.

    params.reset(new base::DictionaryValue());
    params->SetInteger("width", frame_size.width());
    params->SetInteger("height", frame_size.height());
    params->SetDouble("deviceScaleFactor", device_scale_factor);
    params->SetBoolean("mobile", false);
    SendCommand("Emulation.setDeviceMetricsOverride", std::move(params));

    // Resize frame to scaled blue box size.
    gfx::RectF clip;
    clip.set_width(box_size.width());
    clip.set_height(box_size.height());
    clip.set_x((frame_size.width() - box_size.width()) / 2.);
    clip.set_y(kBoxOffsetHeight);

    // Capture screenshot and verify that it is indeed blue.
    SkBitmap expected_bitmap;
    expected_bitmap.allocN32Pixels(scaled_box_size.width(),
                                   scaled_box_size.height());
    expected_bitmap.eraseColor(SkColorSetRGB(0x00, 0x00, 0xff));
    CaptureScreenshotAndCompareTo(expected_bitmap, ENCODING_PNG, true,
                                  device_scale_factor, clip, screenshot_scale);

    // Reset for next screenshot.
    SendCommand("Emulation.clearDeviceMetricsOverride", nullptr);
  }

 private:
#if !defined(OS_ANDROID)
  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitch(switches::kEnablePixelOutputInTests);
  }
#endif
};

IN_PROC_BROWSER_TEST_F(CaptureScreenshotTest, CaptureScreenshot) {
  // This test fails consistently on low-end Android devices.
  // See crbug.com/653637.
  // TODO(eseckler): Reenable with error limit if necessary.
  if (base::SysInfo::IsLowEndDevice()) return;

  shell()->LoadURL(
      GURL("data:text/html,<body style='background:#123456'></body>"));
  WaitForLoadStop(shell()->web_contents());
  Attach();
  SkBitmap expected_bitmap;
  // We compare against the actual physical backing size rather than the
  // view size, because the view size is stored adjusted for DPI and only in
  // integer precision.
  gfx::Size view_size = static_cast<RenderWidgetHostViewBase*>(
                            shell()->web_contents()->GetRenderWidgetHostView())
                            ->GetPhysicalBackingSize();
  expected_bitmap.allocN32Pixels(view_size.width(), view_size.height());
  expected_bitmap.eraseColor(SkColorSetRGB(0x12, 0x34, 0x56));
  CaptureScreenshotAndCompareTo(expected_bitmap, ENCODING_PNG, false);
}

IN_PROC_BROWSER_TEST_F(CaptureScreenshotTest, CaptureScreenshotJpeg) {
  // This test fails consistently on low-end Android devices.
  // See crbug.com/653637.
  // TODO(eseckler): Reenable with error limit if necessary.
  if (base::SysInfo::IsLowEndDevice())
    return;

  shell()->LoadURL(
      GURL("data:text/html,<body style='background:#123456'></body>"));
  WaitForLoadStop(shell()->web_contents());
  Attach();
  SkBitmap expected_bitmap;
  // We compare against the actual physical backing size rather than the
  // view size, because the view size is stored adjusted for DPI and only in
  // integer precision.
  gfx::Size view_size = static_cast<RenderWidgetHostViewBase*>(
                            shell()->web_contents()->GetRenderWidgetHostView())
                            ->GetPhysicalBackingSize();
  expected_bitmap.allocN32Pixels(view_size.width(), view_size.height());
  expected_bitmap.eraseColor(SkColorSetRGB(0x12, 0x34, 0x56));
  CaptureScreenshotAndCompareTo(expected_bitmap, ENCODING_JPEG, false);
}

// Setting frame size (through RWHV) is not supported on Android.
#if defined(OS_ANDROID) || defined(OS_LINUX)
#define MAYBE_CaptureScreenshotArea DISABLED_CaptureScreenshotArea
#else
#define MAYBE_CaptureScreenshotArea CaptureScreenshotArea
#endif
IN_PROC_BROWSER_TEST_F(CaptureScreenshotTest,
                       MAYBE_CaptureScreenshotArea) {
  static const gfx::Size kFrameSize(800, 600);

  shell()->LoadURL(GURL("about:blank"));
  Attach();

  // Test capturing a subarea inside the emulated frame at different scales.
  PlaceAndCaptureBox(kFrameSize, gfx::Size(100, 200), 1.0, 1.);
  PlaceAndCaptureBox(kFrameSize, gfx::Size(100, 200), 2.0, 1.);
  PlaceAndCaptureBox(kFrameSize, gfx::Size(100, 200), 0.5, 1.);

  // Ensure that content outside the emulated frame is painted, too.
  PlaceAndCaptureBox(kFrameSize, gfx::Size(10, 8192), 1.0, 1.);

  // Check non-1 device scale factor.
  PlaceAndCaptureBox(kFrameSize, gfx::Size(100, 200), 1.0, 2.);
  // Ensure not emulating device scale factor works.
  PlaceAndCaptureBox(kFrameSize, gfx::Size(100, 200), 1.0, 0.);
}

// Verifies that setDefaultBackgroundColor and captureScreenshot support a
// transparent background, and that setDeviceMetricsOverride doesn't affect it.

// Temporarily disabled while protocol methods are being refactored.
IN_PROC_BROWSER_TEST_F(CaptureScreenshotTest, DISABLED_TransparentScreenshots) {
  if (base::SysInfo::IsLowEndDevice())
    return;

  shell()->LoadURL(
      GURL("data:text/html,<body style='background:transparent'></body>"));
  WaitForLoadStop(shell()->web_contents());
  Attach();

  // Override background to transparent.
  std::unique_ptr<base::DictionaryValue> color(new base::DictionaryValue());
  color->SetInteger("r", 0);
  color->SetInteger("g", 0);
  color->SetInteger("b", 0);
  color->SetDouble("a", 0);
  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->Set("color", std::move(color));
  SendCommand("Emulation.setDefaultBackgroundColorOverride", std::move(params));

  SkBitmap expected_bitmap;
  // We compare against the actual physical backing size rather than the
  // view size, because the view size is stored adjusted for DPI and only in
  // integer precision.
  gfx::Size view_size = static_cast<RenderWidgetHostViewBase*>(
                            shell()->web_contents()->GetRenderWidgetHostView())
                            ->GetPhysicalBackingSize();
  expected_bitmap.allocN32Pixels(view_size.width(), view_size.height());
  expected_bitmap.eraseColor(SK_ColorTRANSPARENT);
  CaptureScreenshotAndCompareTo(expected_bitmap, ENCODING_PNG, true);

  // Check that device emulation does not affect the transparency.
  params.reset(new base::DictionaryValue());
  params->SetInteger("width", view_size.width());
  params->SetInteger("height", view_size.height());
  params->SetDouble("deviceScaleFactor", 0);
  params->SetBoolean("mobile", false);
  params->SetBoolean("fitWindow", false);
  SendCommand("Emulation.setDeviceMetricsOverride", std::move(params));
  CaptureScreenshotAndCompareTo(expected_bitmap, ENCODING_PNG, true);
}

#if defined(OS_ANDROID)
// Disabled, see http://crbug.com/469947.
IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, DISABLED_SynthesizePinchGesture) {
  GURL test_url = GetTestUrl("devtools", "synthetic_gesture_tests.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  Attach();

  int old_width;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(window.innerWidth)", &old_width));

  int old_height;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(window.innerHeight)",
      &old_height));

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetInteger("x", old_width / 2);
  params->SetInteger("y", old_height / 2);
  params->SetDouble("scaleFactor", 2.0);
  SendCommand("Input.synthesizePinchGesture", std::move(params));

  int new_width;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(window.innerWidth)", &new_width));
  ASSERT_DOUBLE_EQ(2.0, static_cast<double>(old_width) / new_width);

  int new_height;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(window.innerHeight)",
      &new_height));
  ASSERT_DOUBLE_EQ(2.0, static_cast<double>(old_height) / new_height);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, DISABLED_SynthesizeScrollGesture) {
  GURL test_url = GetTestUrl("devtools", "synthetic_gesture_tests.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  Attach();

  int scroll_top;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(document.body.scrollTop)",
      &scroll_top));
  ASSERT_EQ(0, scroll_top);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetInteger("x", 0);
  params->SetInteger("y", 0);
  params->SetInteger("xDistance", 0);
  params->SetInteger("yDistance", -100);
  SendCommand("Input.synthesizeScrollGesture", std::move(params));

  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(document.body.scrollTop)",
      &scroll_top));
  ASSERT_EQ(100, scroll_top);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, DISABLED_SynthesizeTapGesture) {
  GURL test_url = GetTestUrl("devtools", "synthetic_gesture_tests.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  Attach();

  int scroll_top;
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(document.body.scrollTop)",
      &scroll_top));
  ASSERT_EQ(0, scroll_top);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetInteger("x", 16);
  params->SetInteger("y", 16);
  params->SetString("gestureSourceType", "touch");
  SendCommand("Input.synthesizeTapGesture", std::move(params));

  // The link that we just tapped should take us to the bottom of the page. The
  // new value of |document.body.scrollTop| will depend on the screen dimensions
  // of the device that we're testing on, but in any case it should be greater
  // than 0.
  ASSERT_TRUE(content::ExecuteScriptAndExtractInt(
      shell(), "domAutomationController.send(document.body.scrollTop)",
      &scroll_top));
  ASSERT_GT(scroll_top, 0);
}
#endif  // defined(OS_ANDROID)

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, NavigationPreservesMessages) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL test_url = embedded_test_server()->GetURL("/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  Attach();
  SendCommand("Page.enable", nullptr, false);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  test_url = GetTestUrl("devtools", "navigation.html");
  params->SetString("url", test_url.spec());
  TestNavigationObserver navigation_observer(shell()->web_contents());
  SendCommand("Page.navigate", std::move(params), true);
  navigation_observer.Wait();

  bool enough_results = result_ids_.size() >= 2u;
  EXPECT_TRUE(enough_results);
  if (enough_results) {
    EXPECT_EQ(1, result_ids_[0]);  // Page.enable
    EXPECT_EQ(2, result_ids_[1]);  // Page.navigate
  }

  enough_results = notifications_.size() >= 1u;
  EXPECT_TRUE(enough_results);
  bool found_frame_notification = false;
  for (const std::string& notification : notifications_) {
    if (notification == "Page.frameStartedLoading")
      found_frame_notification = true;
  }
  EXPECT_TRUE(found_frame_notification);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, CrossSiteNoDetach) {
  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL test_url1 = embedded_test_server()->GetURL(
      "A.com", "/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url1, 1);
  Attach();

  GURL test_url2 = embedded_test_server()->GetURL(
      "B.com", "/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url2, 1);

  EXPECT_EQ(0u, notifications_.size());
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, CrossSiteNavigation) {
  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL test_url1 =
      embedded_test_server()->GetURL("A.com", "/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url1, 1);
  Attach();
  SendCommand("Page.enable", nullptr, false);

  GURL test_url2 =
      embedded_test_server()->GetURL("B.com", "/devtools/navigation.html");
  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("url", test_url2.spec());
  base::DictionaryValue* result =
      SendCommand("Page.navigate", std::move(params));
  std::string frame_id;
  EXPECT_TRUE(result->GetString("frameId", &frame_id));

  params = WaitForNotification("Page.frameStoppedLoading", true);
  std::string stopped_id;
  EXPECT_TRUE(params->GetString("frameId", &stopped_id));

  EXPECT_EQ(stopped_id, frame_id);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, CrossSiteCrash) {
  set_agent_host_can_close();
  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL test_url1 =
      embedded_test_server()->GetURL("A.com", "/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url1, 1);
  Attach();
  CrashTab(shell()->web_contents());

  GURL test_url2 =
      embedded_test_server()->GetURL("B.com", "/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url2, 1);

  // Should not crash at this point.
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, ReconnectPreservesState) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL test_url = embedded_test_server()->GetURL("/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);

  Shell* second = CreateBrowser();
  NavigateToURLBlockUntilNavigationsComplete(second, test_url, 1);

  Attach();
  SendCommand("Runtime.enable", nullptr);

  agent_host_->DisconnectWebContents();
  agent_host_->ConnectWebContents(second->web_contents());
  WaitForNotification("Runtime.executionContextsCleared");
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, CrossSitePauseInBeforeUnload) {
  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  NavigateToURLBlockUntilNavigationsComplete(shell(),
      embedded_test_server()->GetURL("A.com", "/devtools/navigation.html"), 1);
  Attach();
  SendCommand("Debugger.enable", nullptr);

  ASSERT_TRUE(content::ExecuteScript(
      shell(),
      "window.onbeforeunload = function() { debugger; return null; }"));

  shell()->LoadURL(
      embedded_test_server()->GetURL("B.com", "/devtools/navigation.html"));
  WaitForNotification("Debugger.paused");
  TestNavigationObserver observer(shell()->web_contents(), 1);
  SendCommand("Debugger.resume", nullptr);
  observer.Wait();
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, InspectDuringFrameSwap) {
  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  GURL test_url1 =
      embedded_test_server()->GetURL("A.com", "/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url1, 1);

  ShellAddedObserver new_shell_observer;
  EXPECT_TRUE(ExecuteScript(shell(), "window.open('about:blank','foo');"));
  Shell* new_shell = new_shell_observer.GetShell();
  EXPECT_TRUE(new_shell->web_contents()->HasOpener());

  agent_host_ = DevToolsAgentHost::GetOrCreateFor(new_shell->web_contents());
  agent_host_->AttachClient(this);

  GURL test_url2 =
      embedded_test_server()->GetURL("B.com", "/devtools/navigation.html");

  // After this navigation, if the bug exists, the process will crash.
  NavigateToURLBlockUntilNavigationsComplete(new_shell, test_url2, 1);

  // Ensure that the A.com process is still alive by executing a script in the
  // original tab.
  //
  // TODO(alexmos, nasko):  A better way to do this is to navigate the original
  // tab to another site, watch for process exit, and check whether there was a
  // crash. However, currently there's no way to wait for process exit
  // regardless of whether it's a crash or not.  RenderProcessHostWatcher
  // should be fixed to support waiting on both WATCH_FOR_PROCESS_EXIT and
  // WATCH_FOR_HOST_DESTRUCTION, and then used here.
  bool success = false;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(shell(),
                                          "window.domAutomationController.send("
                                          "    !!window.open('', 'foo'));",
                                          &success));
  EXPECT_TRUE(success);

  GURL test_url3 =
      embedded_test_server()->GetURL("A.com", "/devtools/navigation.html");

  // After this navigation, if the bug exists, the process will crash.
  NavigateToURLBlockUntilNavigationsComplete(new_shell, test_url3, 1);

  // Ensure that the A.com process is still alive by executing a script in the
  // original tab.
  success = false;
  EXPECT_TRUE(ExecuteScriptAndExtractBool(shell(),
                                          "window.domAutomationController.send("
                                          "    !!window.open('', 'foo'));",
                                          &success));
  EXPECT_TRUE(success);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, DoubleCrash) {
  set_agent_host_can_close();
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL test_url = embedded_test_server()->GetURL("/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  SendCommand("ServiceWorker.enable", nullptr);
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  CrashTab(shell()->web_contents());
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  CrashTab(shell()->web_contents());
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  // Should not crash at this point.
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, ReloadBlankPage) {
  Shell* window =  Shell::CreateNewWindow(
      shell()->web_contents()->GetBrowserContext(),
      GURL("javascript:x=1"),
      nullptr,
      gfx::Size());
  WaitForLoadStop(window->web_contents());
  Attach();
  SendCommand("Page.reload", nullptr, false);
  // Should not crash at this point.
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, EvaluateInBlankPage) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("expression", "window");
  SendCommand("Runtime.evaluate", std::move(params), true);
  EXPECT_FALSE(result_->HasKey("exceptionDetails"));
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest,
    EvaluateInBlankPageAfterNavigation) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL test_url = embedded_test_server()->GetURL("/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  Attach();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("expression", "window");
  SendCommand("Runtime.evaluate", std::move(params), true);
  EXPECT_FALSE(result_->HasKey("exceptionDetails"));
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, JavaScriptDialogNotifications) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  TestJavaScriptDialogManager dialog_manager;
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(shell()->web_contents());
  wc->SetDelegate(&dialog_manager);
  SendCommand("Page.enable", nullptr, true);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("expression", "prompt('hello?', 'default')");
  SendCommand("Runtime.evaluate", std::move(params), false);

  params = WaitForNotification("Page.javascriptDialogOpening");
  std::string url;
  EXPECT_TRUE(params->GetString("url", &url));
  EXPECT_EQ("about:blank", url);
  std::string message;
  EXPECT_TRUE(params->GetString("message", &message));
  EXPECT_EQ("hello?", message);
  std::string type;
  EXPECT_TRUE(params->GetString("type", &type));
  EXPECT_EQ("prompt", type);
  std::string default_prompt;
  EXPECT_TRUE(params->GetString("defaultPrompt", &default_prompt));
  EXPECT_EQ("default", default_prompt);

  params.reset(new base::DictionaryValue());
  params->SetBoolean("accept", true);
  params->SetString("promptText", "hi!");
  SendCommand("Page.handleJavaScriptDialog", std::move(params), false);

  params = WaitForNotification("Page.javascriptDialogClosed", true);
  bool result = false;
  EXPECT_TRUE(params->GetBoolean("result", &result));
  EXPECT_TRUE(result);

  EXPECT_TRUE(dialog_manager.is_handled());

  std::string input;
  EXPECT_TRUE(params->GetString("userInput", &input));
  EXPECT_EQ("hi!", input);
  wc->SetDelegate(nullptr);
  wc->SetJavaScriptDialogManagerForTesting(nullptr);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, JavaScriptDialogInterop) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  TestJavaScriptDialogManager dialog_manager;
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(shell()->web_contents());
  wc->SetDelegate(&dialog_manager);
  SendCommand("Page.enable", nullptr, true);
  SendCommand("Runtime.enable", nullptr, true);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("expression", "alert('42')");
  SendCommand("Runtime.evaluate", std::move(params), false);
  WaitForNotification("Page.javascriptDialogOpening");

  dialog_manager.Handle();
  WaitForNotification("Page.javascriptDialogClosed", true);
  wc->SetDelegate(nullptr);
  wc->SetJavaScriptDialogManagerForTesting(nullptr);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, PageDisableWithOpenedDialog) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  TestJavaScriptDialogManager dialog_manager;
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(shell()->web_contents());
  wc->SetDelegate(&dialog_manager);

  SendCommand("Page.enable", nullptr, true);
  SendCommand("Runtime.enable", nullptr, true);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("expression", "alert('42')");
  SendCommand("Runtime.evaluate", std::move(params), false);
  WaitForNotification("Page.javascriptDialogOpening");
  EXPECT_TRUE(wc->IsJavaScriptDialogShowing());

  EXPECT_FALSE(dialog_manager.is_handled());
  SendCommand("Page.disable", nullptr, false);
  EXPECT_TRUE(wc->IsJavaScriptDialogShowing());
  EXPECT_FALSE(dialog_manager.is_handled());
  dialog_manager.Handle();
  EXPECT_FALSE(wc->IsJavaScriptDialogShowing());

  params.reset(new base::DictionaryValue());
  params->SetString("expression", "42");
  SendCommand("Runtime.evaluate", std::move(params), true);

  wc->SetDelegate(nullptr);
  wc->SetJavaScriptDialogManagerForTesting(nullptr);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, PageDisableWithNoDialogManager) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(shell()->web_contents());
  wc->SetDelegate(nullptr);

  SendCommand("Page.enable", nullptr, true);
  SendCommand("Runtime.enable", nullptr, true);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("expression", "alert('42');");
  SendCommand("Runtime.evaluate", std::move(params), false);
  WaitForNotification("Page.javascriptDialogOpening");
  EXPECT_TRUE(wc->IsJavaScriptDialogShowing());

  SendCommand("Page.disable", nullptr, true);
  EXPECT_FALSE(wc->IsJavaScriptDialogShowing());
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, BeforeUnloadDialog) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  TestJavaScriptDialogManager dialog_manager;

  WebContentsImpl* wc = static_cast<WebContentsImpl*>(shell()->web_contents());
  wc->SetDelegate(&dialog_manager);
  SendCommand("Runtime.enable", nullptr, true);

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());

  params.reset(new base::DictionaryValue());
  params->SetString("expression",
                    "window.onbeforeunload=()=>{return 'prompt';}");
  params->SetBoolean("userGesture", true);
  SendCommand("Runtime.evaluate", std::move(params), true);

  SendCommand("Page.enable", nullptr, true);
  SendCommand("Page.reload", nullptr, false);

  params = WaitForNotification("Page.javascriptDialogOpening", true);

  std::string url;
  EXPECT_TRUE(params->GetString("url", &url));
  EXPECT_EQ("about:blank", url);
  std::string type;
  EXPECT_TRUE(params->GetString("type", &type));
  EXPECT_EQ("beforeunload", type);

  params.reset(new base::DictionaryValue());
  params->SetBoolean("accept", true);
  SendCommand("Page.handleJavaScriptDialog", std::move(params), false);
  WaitForNotification("Page.javascriptDialogClosed", true);
  wc->SetDelegate(nullptr);
  wc->SetJavaScriptDialogManagerForTesting(nullptr);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, BrowserCreateAndCloseTarget) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  EXPECT_EQ(1u, shell()->windows().size());
  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("url", "about:blank");
  SendCommand("Target.createTarget", std::move(params), true);
  std::string target_id;
  EXPECT_TRUE(result_->GetString("targetId", &target_id));
  EXPECT_EQ(2u, shell()->windows().size());

  // TODO(eseckler): Since the RenderView is closed asynchronously, we currently
  // don't verify that the command actually closes the shell.
  bool success;
  params.reset(new base::DictionaryValue());
  params->SetString("targetId", target_id);
  SendCommand("Target.closeTarget", std::move(params), true);
  EXPECT_TRUE(result_->GetBoolean("success", &success));
  EXPECT_TRUE(success);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, BrowserGetTargets) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  SendCommand("Target.getTargets", nullptr, true);
  base::ListValue* target_infos;
  EXPECT_TRUE(result_->GetList("targetInfos", &target_infos));
  EXPECT_EQ(1u, target_infos->GetSize());
  base::DictionaryValue* target_info;
  EXPECT_TRUE(target_infos->GetDictionary(0u, &target_info));
  std::string target_id, type, title, url;
  EXPECT_TRUE(target_info->GetString("targetId", &target_id));
  EXPECT_TRUE(target_info->GetString("type", &type));
  EXPECT_TRUE(target_info->GetString("title", &title));
  EXPECT_TRUE(target_info->GetString("url", &url));
  EXPECT_EQ("page", type);
  EXPECT_EQ("about:blank", title);
  EXPECT_EQ("about:blank", url);
}

namespace {
class NavigationFinishedObserver : public content::WebContentsObserver {
 public:
  explicit NavigationFinishedObserver(WebContents* web_contents)
      : WebContentsObserver(web_contents),
        num_finished_(0),
        num_to_wait_for_(0) {}

  ~NavigationFinishedObserver() override {}

  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override {
    if (navigation_handle->WasServerRedirect())
      return;

    num_finished_++;
    if (num_finished_ >= num_to_wait_for_ && num_to_wait_for_ != 0) {
      base::RunLoop::QuitCurrentDeprecated();
    }
  }

  void WaitForNavigationsToFinish(int num_to_wait_for) {
    if (num_finished_ < num_to_wait_for) {
      num_to_wait_for_ = num_to_wait_for;
      RunMessageLoop();
    }
    num_to_wait_for_ = 0;
  }

 private:
  int num_finished_;
  int num_to_wait_for_;
};

class LoadFinishedObserver : public content::WebContentsObserver {
 public:
  explicit LoadFinishedObserver(WebContents* web_contents)
      : WebContentsObserver(web_contents), num_finished_(0) {}

  ~LoadFinishedObserver() override {}

  void DidStopLoading() override {
    num_finished_++;
    if (run_loop_.running())
      run_loop_.Quit();
  }

  void WaitForLoadToFinish() {
    if (num_finished_ == 0)
      run_loop_.Run();
  }

 private:
  int num_finished_;
  base::RunLoop run_loop_;
};

}  // namespace

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, PageStopLoading) {
  ASSERT_TRUE(embedded_test_server()->Start());

  // Navigate to about:blank first so we can make sure there is a target page we
  // can attach to, and have Network.setRequestInterceptionEnabled complete
  // before we start the navigations we're interested in.
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetBoolean("enabled", true);
  SendCommand("Network.setRequestInterceptionEnabled", std::move(params), true);

  LoadFinishedObserver load_finished_observer(shell()->web_contents());

  // The page will try to navigate twice, however since
  // Network.setRequestInterceptionEnabled is true,
  // it'll wait for confirmation before committing to the navigation.
  GURL test_url = embedded_test_server()->GetURL(
      "/devtools/control_navigations/meta_tag.html");
  shell()->LoadURL(test_url);

  // Stop all navigations.
  SendCommand("Page.stopLoading", nullptr);

  // Wait for the initial navigation to finish.
  load_finished_observer.WaitForLoadToFinish();
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, ControlNavigationsMainFrame) {
  ASSERT_TRUE(embedded_test_server()->Start());

  // Navigate to about:blank first so we can make sure there is a target page we
  // can attach to, and have Network.setRequestInterceptionEnabled complete
  // before we start the navigations we're interested in.
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetBoolean("enabled", true);
  SendCommand("Network.setRequestInterceptionEnabled", std::move(params), true);

  NavigationFinishedObserver navigation_finished_observer(
      shell()->web_contents());

  GURL test_url = embedded_test_server()->GetURL(
      "/devtools/control_navigations/meta_tag.html");
  shell()->LoadURL(test_url);

  std::vector<ExpectedNavigation> expected_navigations = {
      {"http://127.0.0.1/devtools/control_navigations/meta_tag.html",
       false /* expected_is_redirect */, false /* abort */},
      {"http://127.0.0.1/devtools/navigation.html",
       false /* expected_is_redirect */, true /* abort */}};

  ProcessNavigationsAnyOrder(std::move(expected_navigations));

  // Wait for the initial navigation and the cancelled meta refresh navigation
  // to finish.
  navigation_finished_observer.WaitForNavigationsToFinish(2);

  // Check main frame has the expected url.
  EXPECT_EQ(
      "http://127.0.0.1/devtools/control_navigations/meta_tag.html",
      RemovePort(
          shell()->web_contents()->GetMainFrame()->GetLastCommittedURL()));
}

class IsolatedDevToolsProtocolTest : public DevToolsProtocolTest {
 public:
  ~IsolatedDevToolsProtocolTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    IsolateAllSitesForTesting(command_line);
  }
};

IN_PROC_BROWSER_TEST_F(IsolatedDevToolsProtocolTest,
                       ControlNavigationsChildFrames) {
  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());

  // Navigate to about:blank first so we can make sure there is a target page we
  // can attach to, and have Network.setRequestInterceptionEnabled complete
  // before we start the navigations we're interested in.
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetBoolean("enabled", true);
  SendCommand("Network.setRequestInterceptionEnabled", std::move(params), true);

  NavigationFinishedObserver navigation_finished_observer(
      shell()->web_contents());

  GURL test_url = embedded_test_server()->GetURL(
      "/devtools/control_navigations/iframe_navigation.html");
  shell()->LoadURL(test_url);

  // Allow main frame navigation, and all iframe navigations to http://a.com
  // Allow initial iframe navigation to http://b.com but dissallow it to
  // navigate to /devtools/navigation.html.
  std::vector<ExpectedNavigation> expected_navigations = {
      {"http://127.0.0.1/devtools/control_navigations/"
       "iframe_navigation.html",
       false /* expected_is_redirect */, false /* abort */},
      {"http://127.0.0.1/cross-site/a.com/devtools/control_navigations/"
       "meta_tag.html",
       false /* expected_is_redirect */, false /* abort */},
      {"http://127.0.0.1/cross-site/b.com/devtools/control_navigations/"
       "meta_tag.html",
       false /* expected_is_redirect */, false /* abort */},
      {"http://a.com/devtools/control_navigations/meta_tag.html",
       true /* expected_is_redirect */, false /* abort */},
      {"http://b.com/devtools/control_navigations/meta_tag.html",
       true /* expected_is_redirect */, false /* abort */},
      {"http://a.com/devtools/navigation.html",
       false /* expected_is_redirect */, false /* abort */},
      {"http://b.com/devtools/navigation.html",
       false /* expected_is_redirect */, true /* abort */}};

  ProcessNavigationsAnyOrder(std::move(expected_navigations));

  // Wait for each frame's navigation to finish, ignoring redirects.
  navigation_finished_observer.WaitForNavigationsToFinish(3);

  // Make sure each frame has the expected url.
  EXPECT_THAT(
      GetAllFrameUrls(),
      ElementsAre("http://127.0.0.1/devtools/control_navigations/"
                  "iframe_navigation.html",
                  "http://a.com/devtools/navigation.html",
                  "http://b.com/devtools/control_navigations/meta_tag.html"));
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, VirtualTimeTest) {
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params->SetString("policy", "pause");
  SendCommand("Emulation.setVirtualTimePolicy", std::move(params), true);

  params.reset(new base::DictionaryValue());
  params->SetString("expression",
                    "setTimeout(function(){console.log('before')}, 999);"
                    "setTimeout(function(){console.log('at')}, 1000);"
                    "setTimeout(function(){console.log('after')}, 1001);");
  SendCommand("Runtime.evaluate", std::move(params), true);

  // Let virtual time advance for one second.
  params.reset(new base::DictionaryValue());
  params->SetString("policy", "advance");
  params->SetInteger("budget", 1000);
  SendCommand("Emulation.setVirtualTimePolicy", std::move(params), true);

  WaitForNotification("Emulation.virtualTimeBudgetExpired");

  params.reset(new base::DictionaryValue());
  params->SetString("expression", "console.log('done')");
  SendCommand("Runtime.evaluate", std::move(params), true);

  // The third timer should not fire.
  EXPECT_THAT(console_messages_, ElementsAre("before", "at", "done"));

  // Let virtual time advance for another second, which should make the third
  // timer fire.
  params.reset(new base::DictionaryValue());
  params->SetString("policy", "advance");
  params->SetInteger("budget", 1000);
  SendCommand("Emulation.setVirtualTimePolicy", std::move(params), true);

  WaitForNotification("Emulation.virtualTimeBudgetExpired");

  EXPECT_THAT(console_messages_, ElementsAre("before", "at", "done", "after"));
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, CertificateError) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_EXPIRED);
  https_server.ServeFilesFromSourceDirectory("content/test/data");
  ASSERT_TRUE(https_server.Start());
  GURL test_url = https_server.GetURL("/devtools/navigation.html");
  std::unique_ptr<base::DictionaryValue> params;
  std::unique_ptr<base::DictionaryValue> command_params;
  int eventId;

  shell()->LoadURL(GURL("about:blank"));
  WaitForLoadStop(shell()->web_contents());

  Attach();
  SendCommand("Network.enable", nullptr, true);
  SendCommand("Security.enable", nullptr, false);
  command_params.reset(new base::DictionaryValue());
  command_params->SetBoolean("override", true);
  SendCommand("Security.setOverrideCertificateErrors",
              std::move(command_params), true);

  // Test cancel.
  SendCommand("Network.clearBrowserCache", nullptr, true);
  SendCommand("Network.clearBrowserCookies", nullptr, true);
  TestNavigationObserver cancel_observer(shell()->web_contents(), 1);
  shell()->LoadURL(test_url);
  params = WaitForNotification("Security.certificateError", false);
  EXPECT_TRUE(shell()->web_contents()->GetController().GetPendingEntry());
  EXPECT_EQ(
      test_url,
      shell()->web_contents()->GetController().GetPendingEntry()->GetURL());
  EXPECT_TRUE(params->GetInteger("eventId", &eventId));
  command_params.reset(new base::DictionaryValue());
  command_params->SetInteger("eventId", eventId);
  command_params->SetString("action", "cancel");
  SendCommand("Security.handleCertificateError", std::move(command_params),
              false);
  cancel_observer.Wait();
  EXPECT_FALSE(shell()->web_contents()->GetController().GetPendingEntry());
  EXPECT_EQ(GURL("about:blank"), shell()
                                     ->web_contents()
                                     ->GetController()
                                     .GetLastCommittedEntry()
                                     ->GetURL());

  // Test continue.
  SendCommand("Network.clearBrowserCache", nullptr, true);
  SendCommand("Network.clearBrowserCookies", nullptr, true);
  TestNavigationObserver continue_observer(shell()->web_contents(), 1);
  shell()->LoadURL(test_url);
  params = WaitForNotification("Security.certificateError", false);
  EXPECT_TRUE(params->GetInteger("eventId", &eventId));
  command_params.reset(new base::DictionaryValue());
  command_params->SetInteger("eventId", eventId);
  command_params->SetString("action", "continue");
  SendCommand("Security.handleCertificateError", std::move(command_params),
              false);
  WaitForNotification("Network.loadingFinished", true);
  continue_observer.Wait();
  EXPECT_EQ(test_url, shell()
                          ->web_contents()
                          ->GetController()
                          .GetLastCommittedEntry()
                          ->GetURL());
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, SubresourceWithCertificateError) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.SetSSLConfig(net::EmbeddedTestServer::CERT_EXPIRED);
  https_server.ServeFilesFromSourceDirectory("content/test/data/devtools");
  ASSERT_TRUE(https_server.Start());
  GURL test_url = https_server.GetURL("/image.html");
  std::unique_ptr<base::DictionaryValue> params;
  std::unique_ptr<base::DictionaryValue> command_params;
  int eventId;

  shell()->LoadURL(GURL("about:blank"));
  WaitForLoadStop(shell()->web_contents());

  Attach();
  SendCommand("Security.enable", nullptr, false);
  command_params.reset(new base::DictionaryValue());
  command_params->SetBoolean("override", true);
  SendCommand("Security.setOverrideCertificateErrors",
              std::move(command_params), true);

  TestNavigationObserver observer(shell()->web_contents(), 1);
  shell()->LoadURL(test_url);

  // Expect certificateError event for main frame.
  params = WaitForNotification("Security.certificateError", false);
  EXPECT_TRUE(params->GetInteger("eventId", &eventId));
  command_params.reset(new base::DictionaryValue());
  command_params->SetInteger("eventId", eventId);
  command_params->SetString("action", "continue");
  SendCommand("Security.handleCertificateError", std::move(command_params),
              false);

  // Expect certificateError event for image.
  params = WaitForNotification("Security.certificateError", false);
  EXPECT_TRUE(params->GetInteger("eventId", &eventId));
  command_params.reset(new base::DictionaryValue());
  command_params->SetInteger("eventId", eventId);
  command_params->SetString("action", "continue");
  SendCommand("Security.handleCertificateError", std::move(command_params),
              false);

  observer.Wait();
  EXPECT_EQ(test_url, shell()
                          ->web_contents()
                          ->GetController()
                          .GetLastCommittedEntry()
                          ->GetURL());
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, TargetDiscovery) {
  std::string temp;
  std::set<std::string> ids;
  std::unique_ptr<base::DictionaryValue> command_params;
  std::unique_ptr<base::DictionaryValue> params;
  bool is_attached;

  ASSERT_TRUE(embedded_test_server()->Start());
  GURL first_url = embedded_test_server()->GetURL("/devtools/navigation.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), first_url, 1);

  GURL second_url = embedded_test_server()->GetURL("/devtools/navigation.html");
  Shell* second = CreateBrowser();
  NavigateToURLBlockUntilNavigationsComplete(second, second_url, 1);

  Attach();
  int attached_count = 0;
  command_params.reset(new base::DictionaryValue());
  command_params->SetBoolean("discover", true);
  SendCommand("Target.setDiscoverTargets", std::move(command_params), true);
  params = WaitForNotification("Target.targetCreated", true);
  EXPECT_TRUE(params->GetString("targetInfo.type", &temp));
  EXPECT_EQ("page", temp);
  EXPECT_TRUE(params->GetBoolean("targetInfo.attached", &is_attached));
  attached_count += is_attached ? 1 : 0;
  EXPECT_TRUE(params->GetString("targetInfo.targetId", &temp));
  EXPECT_TRUE(ids.find(temp) == ids.end());
  ids.insert(temp);
  params = WaitForNotification("Target.targetCreated", true);
  EXPECT_TRUE(params->GetString("targetInfo.type", &temp));
  EXPECT_EQ("page", temp);
  EXPECT_TRUE(params->GetBoolean("targetInfo.attached", &is_attached));
  attached_count += is_attached ? 1 : 0;
  EXPECT_TRUE(params->GetString("targetInfo.targetId", &temp));
  EXPECT_TRUE(ids.find(temp) == ids.end());
  ids.insert(temp);
  EXPECT_TRUE(notifications_.empty());
  EXPECT_EQ(1, attached_count);

  GURL third_url = embedded_test_server()->GetURL("/devtools/navigation.html");
  Shell* third = CreateBrowser();
  NavigateToURLBlockUntilNavigationsComplete(third, third_url, 1);

  params = WaitForNotification("Target.targetCreated", true);
  EXPECT_TRUE(params->GetString("targetInfo.type", &temp));
  EXPECT_EQ("page", temp);
  EXPECT_TRUE(params->GetString("targetInfo.targetId", &temp));
  EXPECT_TRUE(ids.find(temp) == ids.end());
  EXPECT_TRUE(params->GetBoolean("targetInfo.attached", &is_attached));
  EXPECT_FALSE(is_attached);
  std::string attached_id = temp;
  ids.insert(temp);

  params = WaitForNotification("Target.targetInfoChanged", true);
  EXPECT_TRUE(params->GetString("targetInfo.url", &temp));
  EXPECT_EQ("about:blank", temp);

  params = WaitForNotification("Target.targetInfoChanged", true);
  EXPECT_TRUE(params->GetString("targetInfo.url", &temp));
  EXPECT_EQ(third_url, temp);
  EXPECT_TRUE(notifications_.empty());

  second->Close();
  second = nullptr;
  params = WaitForNotification("Target.targetDestroyed", true);
  EXPECT_TRUE(params->GetString("targetId", &temp));
  EXPECT_TRUE(ids.find(temp) != ids.end());
  ids.erase(temp);
  EXPECT_TRUE(notifications_.empty());

  command_params.reset(new base::DictionaryValue());
  command_params->SetString("targetId", attached_id);
  SendCommand("Target.attachToTarget", std::move(command_params), true);
  params = WaitForNotification("Target.targetInfoChanged", true);
  EXPECT_TRUE(params->GetString("targetInfo.targetId", &temp));
  EXPECT_EQ(attached_id, temp);
  EXPECT_TRUE(params->GetBoolean("targetInfo.attached", &is_attached));
  EXPECT_TRUE(is_attached);
  params = WaitForNotification("Target.attachedToTarget", true);
  std::string session_id;
  EXPECT_TRUE(params->GetString("sessionId", &session_id));
  EXPECT_TRUE(params->GetString("targetInfo.targetId", &temp));
  EXPECT_EQ(attached_id, temp);
  EXPECT_TRUE(notifications_.empty());

  WebContents::CreateParams create_params(
      ShellContentBrowserClient::Get()->browser_context(), NULL);
  std::unique_ptr<content::WebContents> web_contents(
      content::WebContents::Create(create_params));
  EXPECT_TRUE(notifications_.empty());

  web_contents->SetDelegate(this);
  params = WaitForNotification("Target.targetCreated", true);
  EXPECT_TRUE(params->GetString("targetInfo.type", &temp));
  EXPECT_EQ("page", temp);
  EXPECT_TRUE(notifications_.empty());

  command_params.reset(new base::DictionaryValue());
  command_params->SetBoolean("discover", false);
  SendCommand("Target.setDiscoverTargets", std::move(command_params), true);
  EXPECT_TRUE(notifications_.empty());

  command_params.reset(new base::DictionaryValue());
  command_params->SetString("sessionId", session_id);
  SendCommand("Target.detachFromTarget", std::move(command_params), true);
  params = WaitForNotification("Target.detachedFromTarget", true);
  EXPECT_TRUE(params->GetString("sessionId", &temp));
  EXPECT_EQ(session_id, temp);
  EXPECT_TRUE(params->GetString("targetId", &temp));
  EXPECT_EQ(attached_id, temp);
  EXPECT_TRUE(notifications_.empty());
}

// Tests that an interstitialShown event is sent when an interstitial is showing
// on attach.
IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, InterstitialShownOnAttach) {
  TestInterstitialDelegate* delegate = new TestInterstitialDelegate;
  WebContentsImpl* web_contents =
      static_cast<WebContentsImpl*>(shell()->web_contents());
  GURL interstitial_url("https://example.test");
  InterstitialPageImpl* interstitial = new InterstitialPageImpl(
      web_contents, static_cast<RenderWidgetHostDelegate*>(web_contents), true,
      interstitial_url, delegate);
  interstitial->Show();
  WaitForInterstitialAttach(web_contents);
  Attach();
  SendCommand("Page.enable", nullptr, false);
  WaitForNotification("Page.interstitialShown", true);
}

class SitePerProcessDevToolsProtocolTest : public DevToolsProtocolTest {
 public:
  void SetUpCommandLine(base::CommandLine* command_line) override {
    DevToolsProtocolTest::SetUpCommandLine(command_line);
    IsolateAllSitesForTesting(command_line);
  };

  void SetUpOnMainThread() override {
    DevToolsProtocolTest::SetUpOnMainThread();
    content::SetupCrossSiteRedirector(embedded_test_server());
    ASSERT_TRUE(embedded_test_server()->Start());
  }
};

IN_PROC_BROWSER_TEST_F(SitePerProcessDevToolsProtocolTest, TargetNoDiscovery) {
  std::string temp;
  std::string target_id;
  std::unique_ptr<base::DictionaryValue> command_params;
  std::unique_ptr<base::DictionaryValue> params;

  GURL main_url(embedded_test_server()->GetURL("/site_per_process_main.html"));
  NavigateToURLBlockUntilNavigationsComplete(shell(), main_url, 1);
  // It is safe to obtain the root frame tree node here, as it doesn't change.
  FrameTreeNode* root =
      static_cast<WebContentsImpl*>(shell()->web_contents())->
          GetFrameTree()->root();

  // Load cross-site page into iframe.
  GURL::Replacements replace_host;
  GURL cross_site_url(embedded_test_server()->GetURL("/title1.html"));
  replace_host.SetHostStr("foo.com");
  cross_site_url = cross_site_url.ReplaceComponents(replace_host);
  NavigateFrameToURL(root->child_at(0), cross_site_url);

  // Enable auto-attach.
  Attach();
  command_params.reset(new base::DictionaryValue());
  command_params->SetBoolean("autoAttach", true);
  command_params->SetBoolean("waitForDebuggerOnStart", true);
  SendCommand("Target.setAutoAttach", std::move(command_params), true);
  EXPECT_TRUE(notifications_.empty());
  command_params.reset(new base::DictionaryValue());
  command_params->SetBoolean("value", true);
  SendCommand("Target.setAttachToFrames", std::move(command_params), false);
  params = WaitForNotification("Target.attachedToTarget", true);
  std::string session_id;
  EXPECT_TRUE(params->GetString("sessionId", &session_id));
  EXPECT_TRUE(params->GetString("targetInfo.targetId", &target_id));
  EXPECT_TRUE(params->GetString("targetInfo.type", &temp));
  EXPECT_EQ("iframe", temp);

  // Load same-site page into iframe.
  FrameTreeNode* child = root->child_at(0);
  GURL http_url(embedded_test_server()->GetURL("/title1.html"));
  NavigateFrameToURL(child, http_url);
  params = WaitForNotification("Target.detachedFromTarget", true);
  EXPECT_TRUE(params->GetString("targetId", &temp));
  EXPECT_EQ(target_id, temp);
  EXPECT_TRUE(params->GetString("sessionId", &temp));
  EXPECT_EQ(session_id, temp);

  // Navigate back to cross-site iframe.
  NavigateFrameToURL(root->child_at(0), cross_site_url);
  params = WaitForNotification("Target.attachedToTarget", true);
  EXPECT_TRUE(params->GetString("sessionId", &session_id));
  EXPECT_TRUE(params->GetString("targetInfo.targetId", &target_id));
  EXPECT_TRUE(params->GetString("targetInfo.type", &temp));
  EXPECT_EQ("iframe", temp);

  // Disable auto-attach.
  command_params.reset(new base::DictionaryValue());
  command_params->SetBoolean("autoAttach", false);
  command_params->SetBoolean("waitForDebuggerOnStart", false);
  SendCommand("Target.setAutoAttach", std::move(command_params), false);
  params = WaitForNotification("Target.detachedFromTarget", true);
  EXPECT_TRUE(params->GetString("targetId", &temp));
  EXPECT_EQ(target_id, temp);
  EXPECT_TRUE(params->GetString("sessionId", &temp));
  EXPECT_EQ(session_id, temp);
}

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, SetAndGetCookies) {
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL test_url = embedded_test_server()->GetURL("/title1.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url, 1);
  Attach();

  // Set two cookies, one of which matches the loaded URL and another that
  // doesn't.
  std::unique_ptr<base::DictionaryValue> command_params;
  command_params.reset(new base::DictionaryValue());
  command_params->SetString("url", test_url.spec());
  command_params->SetString("name", "cookie_for_this_url");
  command_params->SetString("value", "mendacious");
  SendCommand("Network.setCookie", std::move(command_params), false);

  command_params.reset(new base::DictionaryValue());
  command_params->SetString("url", "https://www.chromium.org");
  command_params->SetString("name", "cookie_for_another_url");
  command_params->SetString("value", "polyglottal");
  SendCommand("Network.setCookie", std::move(command_params), false);

  // First get the cookies for just the loaded URL.
  SendCommand("Network.getCookies", nullptr, true);

  base::ListValue* cookies;
  EXPECT_TRUE(result_->HasKey("cookies"));
  EXPECT_TRUE(result_->GetList("cookies", &cookies));
  EXPECT_EQ(1u, cookies->GetSize());

  base::DictionaryValue* cookie;
  std::string name;
  std::string value;
  EXPECT_TRUE(cookies->GetDictionary(0, &cookie));
  EXPECT_TRUE(cookie->GetString("name", &name));
  EXPECT_TRUE(cookie->GetString("value", &value));
  EXPECT_EQ("cookie_for_this_url", name);
  EXPECT_EQ("mendacious", value);

  // Then get all the cookies in the cookie jar.
  SendCommand("Network.getAllCookies", nullptr, true);

  EXPECT_TRUE(result_->HasKey("cookies"));
  EXPECT_TRUE(result_->GetList("cookies", &cookies));
  EXPECT_EQ(2u, cookies->GetSize());

  // Note: the cookies will be returned in unspecified order.
  size_t found = 0;
  for (size_t i = 0; i < cookies->GetSize(); i++) {
    EXPECT_TRUE(cookies->GetDictionary(i, &cookie));
    EXPECT_TRUE(cookie->GetString("name", &name));
    if (name == "cookie_for_this_url") {
      EXPECT_TRUE(cookie->GetString("value", &value));
      EXPECT_EQ("mendacious", value);
      found++;
    } else if (name == "cookie_for_another_url") {
      EXPECT_TRUE(cookie->GetString("value", &value));
      EXPECT_EQ("polyglottal", value);
      found++;
    } else {
      FAIL();
    }
  }
  EXPECT_EQ(2u, found);
}

class DevToolsProtocolTouchTest : public DevToolsProtocolTest {
 public:
  ~DevToolsProtocolTouchTest() override {}

  void SetUpCommandLine(base::CommandLine* command_line) override {
    command_line->AppendSwitchASCII(
        switches::kTouchEventFeatureDetection,
        switches::kTouchEventFeatureDetectionDisabled);
  }
};

IN_PROC_BROWSER_TEST_F(DevToolsProtocolTouchTest, EnableTouch) {
  std::unique_ptr<base::DictionaryValue> params;
  bool result;

  content::SetupCrossSiteRedirector(embedded_test_server());
  ASSERT_TRUE(embedded_test_server()->Start());
  GURL test_url1 =
      embedded_test_server()->GetURL("A.com", "/devtools/enable_touch.html");
  GURL test_url2 =
      embedded_test_server()->GetURL("B.com", "/devtools/enable_touch.html");
  NavigateToURLBlockUntilNavigationsComplete(shell(), test_url1, 1);
  Attach();

  params.reset(new base::DictionaryValue());
  SendCommand("Page.enable", std::move(params), true);

  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "domAutomationController.send(checkProtos(false))", &result));
  EXPECT_TRUE(result);

  params.reset(new base::DictionaryValue());
  params->SetBoolean("enabled", true);
  SendCommand("Emulation.setTouchEmulationEnabled", std::move(params), true);
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "domAutomationController.send(checkProtos(false))", &result));
  EXPECT_TRUE(result);

  params.reset(new base::DictionaryValue());
  params->SetString("url", test_url2.spec());
  SendCommand("Page.navigate", std::move(params), false);
  WaitForNotification("Page.frameStoppedLoading");
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "domAutomationController.send(checkProtos(true))", &result));
  EXPECT_TRUE(result);

  params.reset(new base::DictionaryValue());
  params->SetBoolean("enabled", false);
  SendCommand("Emulation.setTouchEmulationEnabled", std::move(params), true);
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "domAutomationController.send(checkProtos(true))", &result));
  EXPECT_TRUE(result);

  params.reset(new base::DictionaryValue());
  SendCommand("Page.reload", std::move(params), false);
  WaitForNotification("Page.frameStoppedLoading");
  ASSERT_TRUE(content::ExecuteScriptAndExtractBool(
      shell()->web_contents(),
      "domAutomationController.send(checkProtos(false))", &result));
  EXPECT_TRUE(result);
}

// Tests that when a security explanation contains a certificate, it is properly
// serialized into the protocol message.
IN_PROC_BROWSER_TEST_F(DevToolsProtocolTest, CertificateExplanations) {
  net::EmbeddedTestServer https_server(net::EmbeddedTestServer::TYPE_HTTPS);
  https_server.AddDefaultHandlers(
      base::FilePath(FILE_PATH_LITERAL("content/test/data")));
  ASSERT_TRUE(https_server.Start());

  shell()->LoadURL(GURL("about:blank"));
  WaitForLoadStop(shell()->web_contents());

  // Navigate to a page on the server in order to retrieve its certificate
  // chain.
  NavigateToURLBlockUntilNavigationsComplete(
      shell(), https_server.GetURL("/title1.html"), 1);
  WebContentsImpl* wc = static_cast<WebContentsImpl*>(shell()->web_contents());
  NavigationEntry* entry = wc->GetController().GetLastCommittedEntry();
  ASSERT_TRUE(entry);
  scoped_refptr<net::X509Certificate> cert = entry->GetSSL().certificate;

  // Provide |cert| as the certificate on the security style explanations. When
  // the security handler is enabled, DidChangeVisibleSecurityState() is called
  // and the explanations with |cert| are sent to DevTools.
  SetSecurityExplanationCert(cert);
  Attach();
  SendCommand("Security.enable", nullptr, false);
  std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
  params = WaitForMatchingNotification(
      "Security.securityStateChanged",
      base::Bind(&SecurityStateChangedHasCertificateExplanation));

  // There should be one explanation containing the server's certificate chain.
  net::SHA256HashValue cert_chain_fingerprint =
      net::X509Certificate::CalculateChainFingerprint256(
          cert->os_cert_handle(), cert->GetIntermediateCertificates());

  // Read the certificate out of the first explanation.
  const base::ListValue* certificate;
  ASSERT_TRUE(GetCertificateFromNotificationParams(params.get(), &certificate));
  std::vector<std::string> der_certs;
  for (const auto& cert : *certificate) {
    std::string decoded;
    ASSERT_TRUE(base::Base64Decode(cert.GetString(), &decoded));
    der_certs.push_back(decoded);
  }
  std::vector<base::StringPiece> cert_string_piece;
  for (const auto& str : der_certs)
    cert_string_piece.push_back(str);

  // Check that the explanation certificate is correct.
  scoped_refptr<net::X509Certificate> explanation_cert =
      net::X509Certificate::CreateFromDERCertChain(cert_string_piece);
  ASSERT_TRUE(explanation_cert);
  EXPECT_EQ(cert_chain_fingerprint,
            net::X509Certificate::CalculateChainFingerprint256(
                explanation_cert->os_cert_handle(),
                explanation_cert->GetIntermediateCertificates()));
}

// Download tests are flaky on Android: https://crbug.com/7546
#if !defined(OS_ANDROID)
namespace {

static DownloadManagerImpl* DownloadManagerForShell(Shell* shell) {
  // We're in a content_browsertest; we know that the DownloadManager
  // is a DownloadManagerImpl.
  return static_cast<DownloadManagerImpl*>(
      content::BrowserContext::GetDownloadManager(
          shell->web_contents()->GetBrowserContext()));
}

static void RemoveShellDelegate(Shell* shell) {
  content::ShellDownloadManagerDelegate* shell_delegate =
      static_cast<content::ShellDownloadManagerDelegate*>(
          DownloadManagerForShell(shell)->GetDelegate());
  shell_delegate->SetDownloadManager(nullptr);
  DownloadManagerForShell(shell)->SetDelegate(nullptr);
}

class CountingDownloadFile : public DownloadFileImpl {
 public:
  CountingDownloadFile(std::unique_ptr<DownloadSaveInfo> save_info,
                       const base::FilePath& default_downloads_directory,
                       std::unique_ptr<DownloadManager::InputStream> stream,
                       const net::NetLogWithSource& net_log,
                       base::WeakPtr<DownloadDestinationObserver> observer)
      : DownloadFileImpl(std::move(save_info),
                         default_downloads_directory,
                         std::move(stream),
                         net_log,
                         observer) {}

  ~CountingDownloadFile() override {
    DCHECK(GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
    active_files_--;
  }

  void Initialize(const InitializeCallback& callback,
                  const CancelRequestCallback& cancel_request_callback,
                  const DownloadItem::ReceivedSlices& received_slices,
                  bool is_parallelizable) override {
    DCHECK(GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
    active_files_++;
    DownloadFileImpl::Initialize(callback, cancel_request_callback,
                                 received_slices, is_parallelizable);
  }

  static void GetNumberActiveFiles(int* result) {
    DCHECK(GetDownloadTaskRunner()->RunsTasksInCurrentSequence());
    *result = active_files_;
  }

  // Can be called on any thread, and will block (running message loop)
  // until data is returned.
  static int GetNumberActiveFilesFromFileThread() {
    int result = -1;
    GetDownloadTaskRunner()->PostTaskAndReply(
        FROM_HERE,
        base::BindOnce(&CountingDownloadFile::GetNumberActiveFiles, &result),
        base::MessageLoop::current()->QuitWhenIdleClosure());
    base::RunLoop().Run();
    DCHECK_NE(-1, result);
    return result;
  }

 private:
  static int active_files_;
};

int CountingDownloadFile::active_files_ = 0;

class CountingDownloadFileFactory : public DownloadFileFactory {
 public:
  CountingDownloadFileFactory() {}
  ~CountingDownloadFileFactory() override {}

  // DownloadFileFactory interface.
  DownloadFile* CreateFile(
      std::unique_ptr<DownloadSaveInfo> save_info,
      const base::FilePath& default_downloads_directory,
      std::unique_ptr<DownloadManager::InputStream> stream,
      const net::NetLogWithSource& net_log,
      base::WeakPtr<DownloadDestinationObserver> observer) override {
    return new CountingDownloadFile(std::move(save_info),
                                    default_downloads_directory,
                                    std::move(stream), net_log, observer);
  }
};

class TestShellDownloadManagerDelegate : public ShellDownloadManagerDelegate {
 public:
  TestShellDownloadManagerDelegate() : delay_download_open_(false) {}
  ~TestShellDownloadManagerDelegate() override {}

  bool ShouldOpenDownload(
      DownloadItem* item,
      const DownloadOpenDelayedCallback& callback) override {
    if (delay_download_open_) {
      delayed_callbacks_.push_back(callback);
      return false;
    }
    return true;
  }

  bool GenerateFileHash() override { return true; }

  void SetDelayedOpen(bool delay) { delay_download_open_ = delay; }

  void GetDelayedCallbacks(
      std::vector<DownloadOpenDelayedCallback>* callbacks) {
    callbacks->swap(delayed_callbacks_);
  }

 private:
  bool delay_download_open_;
  std::vector<DownloadOpenDelayedCallback> delayed_callbacks_;
};

// Get the next created download.
class DownloadCreateObserver : DownloadManager::Observer {
 public:
  explicit DownloadCreateObserver(DownloadManager* manager)
      : manager_(manager), item_(nullptr) {
    manager_->AddObserver(this);
  }

  ~DownloadCreateObserver() override {
    if (manager_)
      manager_->RemoveObserver(this);
    manager_ = nullptr;
  }

  void ManagerGoingDown(DownloadManager* manager) override {
    DCHECK_EQ(manager_, manager);
    manager_->RemoveObserver(this);
    manager_ = nullptr;
  }

  void OnDownloadCreated(DownloadManager* manager,
                         DownloadItem* download) override {
    if (!item_)
      item_ = download;

    if (!completion_closure_.is_null())
      base::ResetAndReturn(&completion_closure_).Run();
  }

  DownloadItem* WaitForFinished() {
    DCHECK_CURRENTLY_ON(BrowserThread::UI);
    if (!item_) {
      base::RunLoop run_loop;
      completion_closure_ = run_loop.QuitClosure();
      run_loop.Run();
    }
    return item_;
  }

 private:
  DownloadManager* manager_;
  DownloadItem* item_;
  base::Closure completion_closure_;
};

bool IsDownloadInState(DownloadItem::DownloadState state, DownloadItem* item) {
  return item->GetState() == state;
}

class DevToolsDownloadContentTest : public DevToolsProtocolTest {
 protected:
  void SetUpOnMainThread() override {
    base::ThreadRestrictions::SetIOAllowed(true);
    ASSERT_TRUE(downloads_directory_.CreateUniqueTempDir());

    // Set shell default download manager to test proxy reset behavior.
    test_delegate_.reset(new TestShellDownloadManagerDelegate());
    test_delegate_->SetDownloadBehaviorForTesting(
        downloads_directory_.GetPath());
    DownloadManager* manager = DownloadManagerForShell(shell());
    manager->GetDelegate()->Shutdown();
    manager->SetDelegate(test_delegate_.get());
    test_delegate_->SetDownloadManager(manager);

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&net::URLRequestSlowDownloadJob::AddUrlHandler));
    base::FilePath mock_base(GetTestFilePath("download", ""));

    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&net::URLRequestMockHTTPJob::AddUrlHandlers, mock_base));
    ASSERT_TRUE(embedded_test_server()->Start());
  }

  void SetDownloadBehavior(const std::string& behavior) {
    std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
    params->SetString("behavior", behavior);
    SendCommand("Page.setDownloadBehavior", std::move(params));

    EXPECT_GE(result_ids_.size(), 1u);
  }

  void SetDownloadBehavior(const std::string& behavior,
                           const std::string& download_path) {
    std::unique_ptr<base::DictionaryValue> params(new base::DictionaryValue());
    params->SetString("behavior", behavior);
    params->SetString("downloadPath", download_path);
    SendCommand("Page.setDownloadBehavior", std::move(params));

    EXPECT_GE(result_ids_.size(), 1u);
  }

  // Create a DownloadTestObserverTerminal that will wait for the
  // specified number of downloads to finish.
  DownloadTestObserver* CreateWaiter(Shell* shell, int num_downloads) {
    DownloadManager* download_manager = DownloadManagerForShell(shell);
    return new DownloadTestObserverTerminal(
        download_manager, num_downloads,
        DownloadTestObserver::ON_DANGEROUS_DOWNLOAD_FAIL);
  }

  // Note: Cannot be used with other alternative DownloadFileFactorys
  void SetupEnsureNoPendingDownloads() {
    DownloadManagerForShell(shell())->SetDownloadFileFactoryForTesting(
        std::unique_ptr<DownloadFileFactory>(
            new CountingDownloadFileFactory()));
  }

  void WaitForCompletion(DownloadItem* download) {
    DownloadUpdatedObserver(
        download, base::Bind(&IsDownloadInState, DownloadItem::COMPLETE))
        .WaitForEvent();
  }

  bool EnsureNoPendingDownloads() {
    bool result = true;
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(&EnsureNoPendingDownloadJobsOnIO, &result));
    base::RunLoop().Run();
    return result &&
           (CountingDownloadFile::GetNumberActiveFilesFromFileThread() == 0);
  }

  // Checks that |path| is has |file_size| bytes, and matches the |value|
  // string.
  bool VerifyFile(const base::FilePath& path,
                  const std::string& value,
                  const int64_t file_size) {
    std::string file_contents;

    {
      base::ThreadRestrictions::ScopedAllowIO allow_io_during_test_verification;
      bool read = base::ReadFileToString(path, &file_contents);
      EXPECT_TRUE(read) << "Failed reading file: " << path.value() << std::endl;
      if (!read)
        return false;  // Couldn't read the file.
    }

    // Note: we don't handle really large files (more than size_t can hold)
    // so we will fail in that case.
    size_t expected_size = static_cast<size_t>(file_size);

    // Check the size.
    EXPECT_EQ(expected_size, file_contents.size());
    if (expected_size != file_contents.size())
      return false;

    // Check the contents.
    EXPECT_EQ(value, file_contents);
    if (memcmp(file_contents.c_str(), value.c_str(), expected_size) != 0)
      return false;

    return true;
  }

  // Start a download and return the item.
  DownloadItem* StartDownloadAndReturnItem(Shell* shell, GURL url) {
    std::unique_ptr<DownloadCreateObserver> observer(
        new DownloadCreateObserver(DownloadManagerForShell(shell)));
    shell->LoadURL(url);
    return observer->WaitForFinished();
  }

 private:
  static void EnsureNoPendingDownloadJobsOnIO(bool* result) {
    if (net::URLRequestSlowDownloadJob::NumberOutstandingRequests())
      *result = false;
    BrowserThread::PostTask(
        BrowserThread::UI, FROM_HERE,
        base::MessageLoop::current()->QuitWhenIdleClosure());
  }

  // Location of the downloads directory for these tests
  base::ScopedTempDir downloads_directory_;
  std::unique_ptr<TestShellDownloadManagerDelegate> test_delegate_;
};

}  // namespace

// Check that downloading a single file works.
IN_PROC_BROWSER_TEST_F(DevToolsDownloadContentTest, SingleDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  SetDownloadBehavior("allow", "download");
  // Create a download, wait until it's started, and confirm
  // we're in the expected state.
  DownloadItem* download = StartDownloadAndReturnItem(
      shell(),
      GURL(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib")));
  ASSERT_EQ(DownloadItem::IN_PROGRESS, download->GetState());

  WaitForCompletion(download);
  ASSERT_EQ(DownloadItem::COMPLETE, download->GetState());
}

// Check that downloads can be cancelled gracefully.
IN_PROC_BROWSER_TEST_F(DevToolsDownloadContentTest, DownloadCancelled) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  SetDownloadBehavior("allow", "download");
  // Create a download, wait until it's started, and confirm
  // we're in the expected state.
  DownloadItem* download = StartDownloadAndReturnItem(
      shell(), GURL(net::URLRequestSlowDownloadJob::kUnknownSizeUrl));
  ASSERT_EQ(DownloadItem::IN_PROGRESS, download->GetState());

  // Cancel the download and wait for download system quiesce.
  download->Cancel(true);
  scoped_refptr<DownloadTestFlushObserver> flush_observer(
      new DownloadTestFlushObserver(DownloadManagerForShell(shell())));
  flush_observer->WaitForFlush();

  // Get the important info from other threads and check it.
  EXPECT_TRUE(EnsureNoPendingDownloads());
}

// Check that denying downloads works.
IN_PROC_BROWSER_TEST_F(DevToolsDownloadContentTest, DeniedDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  SetDownloadBehavior("deny");
  // Create a download, wait and confirm it was cancelled.
  DownloadItem* download = StartDownloadAndReturnItem(
      shell(),
      GURL(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib")));
  EnsureNoPendingDownloads();
  ASSERT_EQ(DownloadItem::CANCELLED, download->GetState());
}

// Check that defaulting downloads works as expected.
IN_PROC_BROWSER_TEST_F(DevToolsDownloadContentTest, DefaultDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  SetDownloadBehavior("default");
  // Create a download, wait until it's started, and confirm
  // we're in the expected state.
  DownloadItem* download = StartDownloadAndReturnItem(
      shell(), GURL(net::URLRequestSlowDownloadJob::kUnknownSizeUrl));
  ASSERT_EQ(DownloadItem::IN_PROGRESS, download->GetState());

  // Cancel the download and wait for download system quiesce.
  download->Cancel(true);
  scoped_refptr<DownloadTestFlushObserver> flush_observer(
      new DownloadTestFlushObserver(DownloadManagerForShell(shell())));
  flush_observer->WaitForFlush();

  // Get the important info from other threads and check it.
  EXPECT_TRUE(EnsureNoPendingDownloads());
}

// Check that defaulting downloads works as expected when there's no proxy
// download delegate.
IN_PROC_BROWSER_TEST_F(DevToolsDownloadContentTest, DefaultDownloadHeadless) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();
  RemoveShellDelegate(shell());

  SetDownloadBehavior("default");
  // Create a download, wait and confirm it was cancelled.
  DownloadItem* download = StartDownloadAndReturnItem(
      shell(),
      GURL(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib")));
  EnsureNoPendingDownloads();
  ASSERT_EQ(DownloadItem::CANCELLED, download->GetState());
}

// Check that download logic is reset when creating a new target.
IN_PROC_BROWSER_TEST_F(DevToolsDownloadContentTest, ResetDownloadState) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  SetDownloadBehavior("deny");

  Shell* new_window = CreateBrowser();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  // Create a download, wait and confirm it wasn't cancelled.
  DownloadItem* download = StartDownloadAndReturnItem(
      new_window,
      GURL(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib")));
  WaitForCompletion(download);
  ASSERT_EQ(DownloadItem::COMPLETE, download->GetState());
}

// Check that downloading multiple (in this case, 2) files does not result in
// corrupted files.
IN_PROC_BROWSER_TEST_F(DevToolsDownloadContentTest, MultiDownload) {
  base::ThreadRestrictions::SetIOAllowed(true);
  SetupEnsureNoPendingDownloads();
  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  Attach();

  SetDownloadBehavior("allow", "download1");
  // Create a download, wait until it's started, and confirm
  // we're in the expected state.
  DownloadItem* download1 = StartDownloadAndReturnItem(
      shell(), GURL(net::URLRequestSlowDownloadJob::kUnknownSizeUrl));
  ASSERT_EQ(DownloadItem::IN_PROGRESS, download1->GetState());

  NavigateToURLBlockUntilNavigationsComplete(shell(), GURL("about:blank"), 1);
  SetDownloadBehavior("allow", "download2");
  // Start the second download and wait until it's done.
  GURL url(net::URLRequestMockHTTPJob::GetMockUrl("download-test.lib"));
  DownloadItem* download2 = StartDownloadAndReturnItem(shell(), url);
  WaitForCompletion(download2);

  ASSERT_EQ(DownloadItem::IN_PROGRESS, download1->GetState());
  ASSERT_EQ(DownloadItem::COMPLETE, download2->GetState());

  // Allow the first request to finish.
  std::unique_ptr<DownloadTestObserver> observer2(CreateWaiter(shell(), 1));
  NavigateToURL(shell(),
                GURL(net::URLRequestSlowDownloadJob::kFinishDownloadUrl));
  observer2->WaitForFinished();  // Wait for the third request.
  EXPECT_EQ(1u, observer2->NumDownloadsSeenInState(DownloadItem::COMPLETE));

  // Get the important info from other threads and check it.
  EXPECT_TRUE(EnsureNoPendingDownloads());

  // The |DownloadItem|s should now be done and have the final file names.
  // Verify that the files have the expected data and size.
  // |file1| should be full of '*'s, and |file2| should be the same as the
  // source file.
  base::FilePath file1(download1->GetTargetFilePath());
  ASSERT_EQ(file1.DirName().MaybeAsASCII(), "download1");
  size_t file_size1 = net::URLRequestSlowDownloadJob::kFirstDownloadSize +
                      net::URLRequestSlowDownloadJob::kSecondDownloadSize;
  std::string expected_contents(file_size1, '*');
  ASSERT_TRUE(VerifyFile(file1, expected_contents, file_size1));

  base::FilePath file2(download2->GetTargetFilePath());
  ASSERT_EQ(file2.DirName().MaybeAsASCII(), "download2");
  ASSERT_TRUE(base::ContentsEqual(
      file2, GetTestFilePath("download", "download-test.lib")));
}
#endif  // !defined(ANDROID)
}  // namespace content
