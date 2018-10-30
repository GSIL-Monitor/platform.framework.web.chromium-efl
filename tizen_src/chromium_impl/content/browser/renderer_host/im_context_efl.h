// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IM_CONTEXT_EFL
#define IM_CONTEXT_EFL

#include <Evas.h>

#include "content/browser/renderer_host/web_event_factory_efl.h"
#include "content/public/browser/native_web_keyboard_event.h"
#include "ui/base/ime/composition_text.h"
#include "ui/gfx/geometry/rect.h"

typedef struct _Ecore_IMF_Context Ecore_IMF_Context;

namespace content {

class RenderWidgetHostViewEfl;

class IMContextEfl {
 public:
  // Returns NULL if there is no available backend.
  static IMContextEfl* Create(RenderWidgetHostViewEfl*);

  ~IMContextEfl();

  void HandleKeyDownEvent(const Evas_Event_Key_Down* event, bool* was_filtered);
  void HandleKeyUpEvent(const Evas_Event_Key_Up* event, bool* was_filtered);

  void UpdateInputMethodType(ui::TextInputType,
                             ui::TextInputMode,
                             bool can_compose_inline
#if defined(OS_TIZEN_TV_PRODUCT)
                             ,
                             int password_input_minlength
#endif
                             );
  void UpdateInputMethodState(ui::TextInputType,
                              bool can_compose_inline,
                              bool show_if_needed
#if defined(OS_TIZEN_TV_PRODUCT)
                              ,
                              int password_input_minlength
#endif
                              );
  void UpdateCaretBounds(const gfx::Rect& caret_bounds);

  void OnFocusIn();
  void OnFocusOut();

  void OnResume();
  void OnSuspend();
  void DoNotShowAfterResume();

  void ShowPanel();
  void HidePanel();

  void ResetIMFContext();
  void CancelComposition();
  void ConfirmComposition();
  void SetSurroundingText(std::string value);
  void SetIsInFormTag(bool is_in_form_tag);
  void SetCaretPosition(int position);

  // Return visible state based on ecore_imf API.
  bool IsVisible();
  bool WebViewWillBeResized();

  bool IsPreeditQueueEmpty() const { return preedit_queue_.empty(); }
  bool IsCommitQueueEmpty() const { return commit_queue_.empty(); }
  bool IsKeyUpQueueEmpty() const { return keyup_event_queue_.empty(); }
  bool IsKeyDownQueueEmpty() const { return keydown_event_queue_.empty(); }
  void PreeditQueuePop() { preedit_queue_.pop(); }
  void ClearQueues();
  void HandleKeyEvent(bool processed, blink::WebInputEvent::Type);

  void PushToKeyUpEventQueue(int key);
  void PushToKeyDownEventQueue(NativeWebKeyboardEvent key);

  void SendFakeCompositionKeyEvent(const base::string16& buf);

  void UpdateLayoutVariation(bool is_minimum_negative, bool is_step_integer);
  bool IsDefaultReturnKeyType();
#if defined(OS_TIZEN_MOBILE_PRODUCT)
  void PrevNextButtonUpdate(bool prev_state, bool next_state);
#endif
  bool ShouldHandleImmediately(const char* key);
  void SetIMERecommendedWords(const std::string&);
  void SetIMERecommendedWordsType(bool should_enable);

  void SetDefaultViewSize(const gfx::Size& size) { default_view_size_ = size; }

 private:
  IMContextEfl(RenderWidgetHostViewEfl*, Ecore_IMF_Context*);

  void InitializeIMFContext(Ecore_IMF_Context*);

  // callbacks
  static Eina_Bool IMFRetrieveSurroundingCallback(void* data,
                                                  Ecore_IMF_Context*,
                                                  char** text,
                                                  int* offset);
  static void IMFCandidatePanelGeometryChangedCallback(void* data,
                                                       Ecore_IMF_Context*,
                                                       int state);
  static void IMFCandidatePanelLanguageChangedCallback(
      void* data,
      Ecore_IMF_Context* context,
      int value);
  static void IMFCandidatePanelStateChangedCallback(void* data,
                                                    Ecore_IMF_Context*,
                                                    int state);
  static void IMFCommitCallback(void* data,
                                Ecore_IMF_Context*,
                                void* event_info);
  static void IMFDeleteSurroundingCallback(void* data,
                                           Ecore_IMF_Context*,
                                           void* event_info);
#if defined(OS_TIZEN_TV_PRODUCT)
  static void IMFEventPrivateCommandCallback(void* data,
                                             Ecore_IMF_Context* context,
                                             void* eventInfo);
#endif
  static void IMFInputPanelStateChangedCallback(void* data,
                                                Ecore_IMF_Context*,
                                                int state);
  static void IMFInputPanelGeometryChangedCallback(void* data,
                                                   Ecore_IMF_Context*,
                                                   int state);
  static void IMFPreeditChangedCallback(void* data,
                                        Ecore_IMF_Context* context,
                                        void* event_info);
  static void IMFTransactionPrivateCommandSendCallback(
      void* data,
      Ecore_IMF_Context* context,
      void* event_info);
#if defined(OS_TIZEN_MOBILE_PRODUCT)
  static void IMFEventPrivateCmdCallback(void* data,
                                         Ecore_IMF_Context* context,
                                         void* eventInfo);
#endif
  void SetComposition(const char* buffer);
  // callback handlers
  void OnCommit(void* event_info);
  void OnPreeditChanged(void* data,
                        Ecore_IMF_Context* context,
                        void* event_info);
#if defined(OS_TIZEN_MOBILE_PRODUCT)
  void OnIMFPrevNextButtonPressedCallback(std::string command);
#endif
  void OnInputPanelStateChanged(int state);
  void OnInputPanelGeometryChanged();

  void OnCandidateInputPanelStateChanged(int state);
  void OnCandidateInputPanelGeometryChanged();

  bool OnRetrieveSurrounding(char** text, int* offset);
  void OnDeleteSurrounding(void* event_info);
  void OnCandidateInputPanelLanguageChanged(Ecore_IMF_Context* context);
#if defined(OS_TIZEN_TV_PRODUCT)
  void OnIMFEventPrivateCommand(std::string command);
#endif

  void ProcessNextCommitText(bool processed);
  void ProcessNextPreeditText(bool processed);
  void ProcessNextKeyDownEvent();
  void ProcessNextKeyUpEvent();
  void SendCompositionKeyUpEvent(int code);

  RenderWidgetHostImpl* GetRenderWidgetHostImpl() const;

  RenderWidgetHostViewEfl* view_;

  Ecore_IMF_Context* context_;

  // Whether or not the associated widget is focused.
  bool is_focused_;

  // Whether or not is in form tag.
  bool is_in_form_tag_;

  bool is_showing_;

  // The current soft keyboard mode and layout
  ui::TextInputMode current_mode_;
  ui::TextInputType current_type_;
  bool can_compose_inline_;

  ui::CompositionText composition_;

  // View size when IME isn't shown.
  gfx::Size default_view_size_;

  typedef std::queue<base::string16> CommitQueue;
  typedef std::queue<ui::CompositionText> PreeditQueue;
  typedef std::queue<int> KeyUpEventQueue;
  typedef std::queue<NativeWebKeyboardEvent> KeyDownEventQueue;

  CommitQueue commit_queue_;
  PreeditQueue preedit_queue_;
  KeyUpEventQueue keyup_event_queue_;
  KeyDownEventQueue keydown_event_queue_;

  bool is_ime_ctx_reset_;

  bool should_show_on_resume_;

  bool is_get_lookup_table_from_app_;

  int caret_position_;
#if defined(OS_TIZEN_TV_PRODUCT)
  int password_input_minlength_;
  int surrounding_text_length_;
#endif

  std::string surrounding_text_;
  bool is_surrounding_text_change_in_progress_;

  bool is_keyevent_processing_;
  bool is_transaction_finished_;
};

}  // namespace content

#endif
