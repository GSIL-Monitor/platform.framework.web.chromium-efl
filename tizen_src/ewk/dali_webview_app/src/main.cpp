// Copyright 2018 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <dali-toolkit/dali-toolkit.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>
#include <Elementary.h>
#include <EWebKit_internal.h>
#include <EWebKit_product.h>
#include <tbm_surface.h>

using namespace Dali;
using namespace Dali::Toolkit;

class DaliWebViewController : public ConnectionTracker
{
 public:
  DaliWebViewController(Application& application)
      : application_(application) {
    application_.InitSignal().Connect(this, &DaliWebViewController::Create);

    window_ = ecore_evas_new("wayland_egl", 0, 0, 1, 1, 0);
    webview_ = ewk_view_add(ecore_evas_get(window_));
    ewk_view_reader_mode_set(webview_, true);
    evas_object_smart_callback_add(webview_, "offscreen,frame,rendered",
                                   &DaliWebViewController::OnFrameRendered,
                                   this);
    evas_object_resize(webview_, 720, 1280);
    evas_object_show(webview_);
    ecore_evas_show(window_);
  }

  ~DaliWebViewController() {}

  void Create(Application& application) {
    stage_ =  Stage::GetCurrent();
    stage_.SetBackgroundColor(Color::YELLOW);
    TextLabel text_label = TextLabel::New("Now loading...");
    text_label.SetSize(stage_.GetSize());
    text_label.SetAnchorPoint(AnchorPoint::TOP_LEFT);
    text_label.SetProperty(TextLabel::Property::HORIZONTAL_ALIGNMENT, "CENTER");
    text_label.SetProperty(TextLabel::Property::VERTICAL_ALIGNMENT, "CENTER");
    stage_.Add(text_label);
  }

  void LoadURL(const char* url) {
    ewk_view_url_set(webview_, url);
  }

  void UpdateImageFromTBM(void* buffer) {
    if (!buffer)
      return;

    Any source(static_cast<tbm_surface_h>(buffer));
    if (!dali_image_src_) {
      dali_image_src_ = Dali::NativeImageSource::New(source);
      dali_image_ = NativeImage::New(*dali_image_src_);
      image_view_for_tbm_ = ImageView::New(dali_image_);

      image_view_for_tbm_.SetParentOrigin(ParentOrigin::TOP_CENTER);
      image_view_for_tbm_.SetAnchorPoint(AnchorPoint::TOP_CENTER);
      image_view_for_tbm_.SetSize(Size(720, 1280));
      image_view_for_tbm_.SetSizeScalePolicy(SizeScalePolicy::FIT_WITH_ASPECT_RATIO);
      image_view_for_tbm_.TouchSignal().Connect(&DaliWebViewController::OnTouch);

      stage_.Add(image_view_for_tbm_);
    } else {
      dali_image_src_->SetSource(source);
      image_view_for_tbm_.SetSize(Size(720, 1280));
    }
  }

  static bool OnTouch(Actor actor, const TouchData& touch) {
    Ewk_Touch_Event_Type type;
    Evas_Touch_Point_State state;
    switch (touch.GetState(0)) {
    case PointState::DOWN:
      type = EWK_TOUCH_START;
      state = EVAS_TOUCH_POINT_DOWN;
      break;
    case PointState::UP:
      type = EWK_TOUCH_END;
      state = EVAS_TOUCH_POINT_UP;
      break;
    case PointState::MOTION:
      type = EWK_TOUCH_MOVE;
      state = EVAS_TOUCH_POINT_MOVE;
      break;
    case PointState::INTERRUPTED:
      type = EWK_TOUCH_CANCEL;
      state = EVAS_TOUCH_POINT_CANCEL;
      break;
    default:
      break;
    }

    Eina_List* point_list = 0;
    Ewk_Touch_Point* point = new Ewk_Touch_Point;
    point->id = 0;
    point->x = touch.GetLocalPosition(0).x;
    point->y = touch.GetLocalPosition(0).y;
    point->state = state;
    point_list = eina_list_append(point_list, point);

    ewk_view_feed_touch_event(
        DaliWebViewController::webview_, type, point_list, 0);
    eina_list_free(point_list);
    return true;
  }

  static void OnFrameRendered(void* data, Evas_Object*, void* buffer) {
    auto app = static_cast<DaliWebViewController*>(data);
    app->UpdateImageFromTBM(buffer);
  }

 private:
  Application& application_;
  Stage stage_;

  NativeImageSourcePtr dali_image_src_;
  NativeImage dali_image_;
  ImageView image_view_for_tbm_;

  Ecore_Evas* window_;
  static Evas_Object* webview_;
};

Evas_Object* DaliWebViewController::webview_ = 0;

int main(int argc, char **argv)
{
  elm_init(argc, argv);
  ewk_init();

  Application application = Application::New( &argc, &argv );
  DaliWebViewController app(application);

  std::string url("http://google.com");
  if (argc > 1)
    url = argv[1];
  app.LoadURL(url.data());

  application.MainLoop();
  ewk_shutdown();
  return 0;
}

