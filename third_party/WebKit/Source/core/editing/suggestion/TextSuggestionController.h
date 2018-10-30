// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TextSuggestionController_h
#define TextSuggestionController_h

#include "core/CoreExport.h"
#include "core/dom/DocumentShutdownObserver.h"
#include "core/editing/Forward.h"
#include "core/editing/markers/DocumentMarker.h"
#include "platform/heap/Handle.h"
#include "public/platform/input_host.mojom-blink.h"

namespace blink {

class Document;
class DocumentMarker;
class LocalFrame;
struct TextSuggestionInfo;

// This class handles functionality related to displaying a menu of text
// suggestions (e.g. from spellcheck), and performing actions relating to those
// suggestions. Android is currently the only platform that has such a menu.
class CORE_EXPORT TextSuggestionController final
    : public GarbageCollectedFinalized<TextSuggestionController>,
      public DocumentShutdownObserver {
  USING_GARBAGE_COLLECTED_MIXIN(TextSuggestionController);

 public:
  explicit TextSuggestionController(LocalFrame&);

  void DocumentAttached(Document*);

  bool IsMenuOpen() const;

  void HandlePotentialSuggestionTap(const PositionInFlatTree& caret_position);

  void ApplySpellCheckSuggestion(const String& suggestion);
  void ApplyTextSuggestion(int32_t marker_tag, uint32_t suggestion_index);
  void DeleteActiveSuggestionRange();
  void OnNewWordAddedToDictionary(const String& word);
  void OnSuggestionMenuClosed();
  void SuggestionMenuTimeoutCallback(size_t max_number_of_suggestions);

  DECLARE_TRACE();

 private:
  Document& GetDocument() const;
  bool IsAvailable() const;
  LocalFrame& GetFrame() const;

  std::pair<const Node*, const DocumentMarker*> FirstMarkerIntersectingRange(
      const EphemeralRangeInFlatTree&,
      DocumentMarker::MarkerTypes) const;
  std::pair<const Node*, const DocumentMarker*> FirstMarkerTouchingSelection(
      DocumentMarker::MarkerTypes) const;

  void AttemptToDeleteActiveSuggestionRange();
  void CallMojoShowTextSuggestionMenu(
      const Vector<TextSuggestionInfo>& text_suggestion_infos,
      const String& misspelled_word);
  void ShowSpellCheckMenu(
      const std::pair<Node*, DocumentMarker*>& node_spelling_marker_pair);
  void ShowSuggestionMenu(
      const HeapVector<std::pair<Member<Node>, Member<DocumentMarker>>>&
          node_suggestion_marker_pairs,
      size_t max_number_of_suggestions);
  void ReplaceActiveSuggestionRange(const String&);
  void ReplaceRangeWithText(const EphemeralRange&, const String& replacement);

  bool is_suggestion_menu_open_;
  const Member<LocalFrame> frame_;
  mojom::blink::TextSuggestionHostPtr text_suggestion_host_;

  DISALLOW_COPY_AND_ASSIGN(TextSuggestionController);
};

}  // namespace blink

#endif  // TextSuggestionController_h
