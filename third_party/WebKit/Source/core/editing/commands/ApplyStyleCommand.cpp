/*
 * Copyright (C) 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "core/editing/commands/ApplyStyleCommand.h"

#include "core/CSSPropertyNames.h"
#include "core/CSSValueKeywords.h"
#include "core/css/CSSComputedStyleDeclaration.h"
#include "core/css/CSSPrimitiveValue.h"
#include "core/css/StylePropertySet.h"
#include "core/dom/Document.h"
#include "core/dom/NodeList.h"
#include "core/dom/NodeTraversal.h"
#include "core/dom/Range.h"
#include "core/dom/Text.h"
#include "core/editing/EditingStyle.h"
#include "core/editing/EditingStyleUtilities.h"
#include "core/editing/EditingUtilities.h"
#include "core/editing/EphemeralRange.h"
#include "core/editing/PlainTextRange.h"
#include "core/editing/SelectionTemplate.h"
#include "core/editing/VisiblePosition.h"
#include "core/editing/VisibleSelection.h"
#include "core/editing/VisibleUnits.h"
#include "core/editing/iterators/TextIterator.h"
#include "core/editing/serializers/HTMLInterchange.h"
#include "core/frame/UseCounter.h"
#include "core/html/HTMLFontElement.h"
#include "core/html/HTMLSpanElement.h"
#include "core/html_names.h"
#include "core/layout/LayoutObject.h"
#include "core/layout/LayoutText.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/StdLibExtras.h"
#include "platform/wtf/text/StringBuilder.h"

namespace blink {

using namespace HTMLNames;

static bool HasNoAttributeOrOnlyStyleAttribute(
    const HTMLElement* element,
    ShouldStyleAttributeBeEmpty should_style_attribute_be_empty) {
  AttributeCollection attributes = element->Attributes();
  if (attributes.IsEmpty())
    return true;

  unsigned matched_attributes = 0;
  if (element->hasAttribute(styleAttr) &&
      (should_style_attribute_be_empty == kAllowNonEmptyStyleAttribute ||
       !element->InlineStyle() || element->InlineStyle()->IsEmpty()))
    matched_attributes++;

  DCHECK_LE(matched_attributes, attributes.size());
  return matched_attributes == attributes.size();
}

bool IsStyleSpanOrSpanWithOnlyStyleAttribute(const Element* element) {
  if (auto* span = ToHTMLSpanElementOrNull(element)) {
    return HasNoAttributeOrOnlyStyleAttribute(span,
                                              kAllowNonEmptyStyleAttribute);
  }
  return false;
}

static inline bool IsSpanWithoutAttributesOrUnstyledStyleSpan(
    const Node* node) {
  if (auto* span = ToHTMLSpanElementOrNull(node)) {
    return HasNoAttributeOrOnlyStyleAttribute(span,
                                              kStyleAttributeShouldBeEmpty);
  }
  return false;
}

bool IsEmptyFontTag(
    const Element* element,
    ShouldStyleAttributeBeEmpty should_style_attribute_be_empty) {
  if (auto* font = ToHTMLFontElementOrNull(element)) {
    return HasNoAttributeOrOnlyStyleAttribute(font,
                                              should_style_attribute_be_empty);
  }
  return false;
}

static bool OffsetIsBeforeLastNodeOffset(int offset, Node* anchor_node) {
  if (anchor_node->IsCharacterDataNode())
    return offset < anchor_node->MaxCharacterOffset();
  int current_offset = 0;
  for (Node* node = NodeTraversal::FirstChild(*anchor_node);
       node && current_offset < offset;
       node = NodeTraversal::NextSibling(*node))
    current_offset++;
  return offset < current_offset;
}

ApplyStyleCommand::ApplyStyleCommand(Document& document,
                                     const EditingStyle* style,
                                     InputEvent::InputType input_type,
                                     EPropertyLevel property_level)
    : CompositeEditCommand(document),
      style_(style->Copy()),
      input_type_(input_type),
      property_level_(property_level),
      start_(MostForwardCaretPosition(EndingSelection().Start())),
      end_(MostBackwardCaretPosition(EndingSelection().End())),
      use_ending_selection_(true),
      styled_inline_element_(nullptr),
      remove_only_(false),
      is_inline_element_to_remove_function_(0) {}

ApplyStyleCommand::ApplyStyleCommand(Document& document,
                                     const EditingStyle* style,
                                     const Position& start,
                                     const Position& end)
    : CompositeEditCommand(document),
      style_(style->Copy()),
      input_type_(InputEvent::InputType::kNone),
      property_level_(kPropertyDefault),
      start_(start),
      end_(end),
      use_ending_selection_(false),
      styled_inline_element_(nullptr),
      remove_only_(false),
      is_inline_element_to_remove_function_(0) {}

ApplyStyleCommand::ApplyStyleCommand(Element* element, bool remove_only)
    : CompositeEditCommand(element->GetDocument()),
      style_(EditingStyle::Create()),
      input_type_(InputEvent::InputType::kNone),
      property_level_(kPropertyDefault),
      start_(MostForwardCaretPosition(EndingSelection().Start())),
      end_(MostBackwardCaretPosition(EndingSelection().End())),
      use_ending_selection_(true),
      styled_inline_element_(element),
      remove_only_(remove_only),
      is_inline_element_to_remove_function_(0) {}

ApplyStyleCommand::ApplyStyleCommand(
    Document& document,
    const EditingStyle* style,
    IsInlineElementToRemoveFunction is_inline_element_to_remove_function,
    InputEvent::InputType input_type)
    : CompositeEditCommand(document),
      style_(style->Copy()),
      input_type_(input_type),
      property_level_(kPropertyDefault),
      start_(MostForwardCaretPosition(EndingSelection().Start())),
      end_(MostBackwardCaretPosition(EndingSelection().End())),
      use_ending_selection_(true),
      styled_inline_element_(nullptr),
      remove_only_(true),
      is_inline_element_to_remove_function_(
          is_inline_element_to_remove_function) {}

void ApplyStyleCommand::UpdateStartEnd(const Position& new_start,
                                       const Position& new_end) {
  DCHECK_GE(new_end, new_start);

  if (!use_ending_selection_ && (new_start != start_ || new_end != end_))
    use_ending_selection_ = true;
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();
  const VisibleSelection& visible_selection = CreateVisibleSelection(
      SelectionInDOMTree::Builder()
          .Collapse(new_start)
          .Extend(new_end)
          .SetIsDirectional(EndingSelection().IsDirectional())
          .Build());
  SetEndingSelection(
      SelectionForUndoStep::From(visible_selection.AsSelection()));
  start_ = new_start;
  end_ = new_end;
}

Position ApplyStyleCommand::StartPosition() {
  if (use_ending_selection_)
    return EndingSelection().Start();

  return start_;
}

Position ApplyStyleCommand::EndPosition() {
  if (use_ending_selection_)
    return EndingSelection().End();

  return end_;
}

void ApplyStyleCommand::DoApply(EditingState* editing_state) {
  DCHECK(StartPosition().IsNotNull());
  DCHECK(EndPosition().IsNotNull());
  switch (property_level_) {
    case kPropertyDefault: {
      // Apply the block-centric properties of the style.
      EditingStyle* block_style = style_->ExtractAndRemoveBlockProperties();
      if (!block_style->IsEmpty()) {
        ApplyBlockStyle(block_style, editing_state);
        if (editing_state->IsAborted())
          return;
      }
      // Apply any remaining styles to the inline elements.
      if (!style_->IsEmpty() || styled_inline_element_ ||
          is_inline_element_to_remove_function_) {
        ApplyRelativeFontStyleChange(style_.Get(), editing_state);
        if (editing_state->IsAborted())
          return;
        ApplyInlineStyle(style_.Get(), editing_state);
        if (editing_state->IsAborted())
          return;
      }
      break;
    }
    case kForceBlockProperties:
      // Force all properties to be applied as block styles.
      ApplyBlockStyle(style_.Get(), editing_state);
      break;
  }
}

InputEvent::InputType ApplyStyleCommand::GetInputType() const {
  return input_type_;
}

void ApplyStyleCommand::ApplyBlockStyle(EditingStyle* style,
                                        EditingState* editing_state) {
  // update document layout once before removing styles
  // so that we avoid the expense of updating before each and every call
  // to check a computed style
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  // get positions we want to use for applying style
  Position start = StartPosition();
  Position end = EndPosition();
  if (ComparePositions(end, start) < 0) {
    Position swap = start;
    start = end;
    end = swap;
  }

  VisiblePosition visible_start = CreateVisiblePosition(start);
  VisiblePosition visible_end = CreateVisiblePosition(end);

  if (visible_start.IsNull() || visible_start.IsOrphan() ||
      visible_end.IsNull() || visible_end.IsOrphan())
    return;

  // Save and restore the selection endpoints using their indices in the
  // document, since addBlockStyleIfNeeded may moveParagraphs, which can remove
  // these endpoints. Calculate start and end indices from the start of the tree
  // that they're in.
  Node& scope = NodeTraversal::HighestAncestorOrSelf(
      *visible_start.DeepEquivalent().AnchorNode());
  Range* start_range =
      Range::Create(GetDocument(), Position::FirstPositionInNode(scope),
                    visible_start.DeepEquivalent().ParentAnchoredEquivalent());
  Range* end_range =
      Range::Create(GetDocument(), Position::FirstPositionInNode(scope),
                    visible_end.DeepEquivalent().ParentAnchoredEquivalent());

  const TextIteratorBehavior behavior =
      TextIteratorBehavior::AllVisiblePositionsRangeLengthBehavior();

  int start_index =
      TextIterator::RangeLength(EphemeralRange(start_range), behavior);
  int end_index =
      TextIterator::RangeLength(EphemeralRange(end_range), behavior);

  VisiblePosition paragraph_start(StartOfParagraph(visible_start));
  VisiblePosition next_paragraph_start(
      NextPositionOf(EndOfParagraph(paragraph_start)));
  Position beyond_end =
      NextPositionOf(EndOfParagraph(visible_end)).DeepEquivalent();
  // TODO(editing-dev): Use a saner approach (e.g., temporary Ranges) to keep
  // these positions in document instead of iteratively performing orphan checks
  // and recalculating them when they become orphans.
  while (paragraph_start.IsNotNull() &&
         paragraph_start.DeepEquivalent() != beyond_end) {
    DCHECK(!paragraph_start.IsOrphan()) << paragraph_start;
    StyleChange style_change(style, paragraph_start.DeepEquivalent());
    if (style_change.CssStyle().length() || remove_only_) {
      Element* block =
          EnclosingBlock(paragraph_start.DeepEquivalent().AnchorNode());
      const Position& paragraph_start_to_move =
          paragraph_start.DeepEquivalent();
      if (!remove_only_ && IsEditablePosition(paragraph_start_to_move)) {
        HTMLElement* new_block = MoveParagraphContentsToNewBlockIfNecessary(
            paragraph_start_to_move, editing_state);
        if (editing_state->IsAborted())
          return;
        if (new_block) {
          block = new_block;
          if (paragraph_start.IsOrphan()) {
            GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();
            paragraph_start = CreateVisiblePosition(
                Position::FirstPositionInNode(*new_block));
          }
        }
        DCHECK(!paragraph_start.IsOrphan()) << paragraph_start;
      }
      if (block && block->IsHTMLElement()) {
        RemoveCSSStyle(style, ToHTMLElement(block), editing_state);
        if (editing_state->IsAborted())
          return;
        DCHECK(!paragraph_start.IsOrphan()) << paragraph_start;
        if (!remove_only_) {
          AddBlockStyle(style_change, ToHTMLElement(block));
          DCHECK(!paragraph_start.IsOrphan()) << paragraph_start;
        }
      }

      GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

      // Make the VisiblePositions valid again after style changes.
      // TODO(editing-dev): We shouldn't store VisiblePositions and inspect
      // their properties after they have been invalidated by mutations. See
      // crbug.com/648949 for details.
      DCHECK(!paragraph_start.IsOrphan()) << paragraph_start;
      paragraph_start =
          CreateVisiblePosition(paragraph_start.ToPositionWithAffinity());
      if (next_paragraph_start.IsOrphan()) {
        next_paragraph_start = NextPositionOf(EndOfParagraph(paragraph_start));
      } else {
        next_paragraph_start = CreateVisiblePosition(
            next_paragraph_start.ToPositionWithAffinity());
      }
    }

    DCHECK(!next_paragraph_start.IsOrphan()) << next_paragraph_start;
    paragraph_start = next_paragraph_start;
    next_paragraph_start = NextPositionOf(EndOfParagraph(paragraph_start));
  }

  // Update style and layout again, since added or removed styles could have
  // affected the layout. We need clean layout in order to compute
  // plain-text ranges below.
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  EphemeralRange start_ephemeral_range =
      PlainTextRange(start_index)
          .CreateRangeForSelection(ToContainerNode(scope));
  if (start_ephemeral_range.IsNull())
    return;
  EphemeralRange end_ephemeral_range =
      PlainTextRange(end_index).CreateRangeForSelection(ToContainerNode(scope));
  if (end_ephemeral_range.IsNull())
    return;
  UpdateStartEnd(start_ephemeral_range.StartPosition(),
                 end_ephemeral_range.StartPosition());
}

static MutableStylePropertySet* CopyStyleOrCreateEmpty(
    const StylePropertySet* style) {
  if (!style)
    return MutableStylePropertySet::Create(kHTMLQuirksMode);
  return style->MutableCopy();
}

void ApplyStyleCommand::ApplyRelativeFontStyleChange(
    EditingStyle* style,
    EditingState* editing_state) {
  static const float kMinimumFontSize = 0.1f;

  if (!style || !style->HasFontSizeDelta())
    return;

  Position start = StartPosition();
  Position end = EndPosition();
  if (ComparePositions(end, start) < 0) {
    Position swap = start;
    start = end;
    end = swap;
  }

  // Join up any adjacent text nodes.
  if (start.AnchorNode()->IsTextNode()) {
    JoinChildTextNodes(start.AnchorNode()->parentNode(), start, end);
    start = StartPosition();
    end = EndPosition();
  }

  if (start.IsNull() || end.IsNull())
    return;

  if (end.AnchorNode()->IsTextNode() &&
      start.AnchorNode()->parentNode() != end.AnchorNode()->parentNode()) {
    JoinChildTextNodes(end.AnchorNode()->parentNode(), start, end);
    start = StartPosition();
    end = EndPosition();
  }

  if (start.IsNull() || end.IsNull())
    return;

  // Split the start text nodes if needed to apply style.
  if (IsValidCaretPositionInTextNode(start)) {
    SplitTextAtStart(start, end);
    start = StartPosition();
    end = EndPosition();
  }

  if (IsValidCaretPositionInTextNode(end)) {
    SplitTextAtEnd(start, end);
    start = StartPosition();
    end = EndPosition();
  }

  DCHECK(start.AnchorNode());
  DCHECK(end.AnchorNode());
  // Calculate loop end point.
  // If the end node is before the start node (can only happen if the end node
  // is an ancestor of the start node), we gather nodes up to the next sibling
  // of the end node
  const Node* const beyond_end = end.NodeAsRangePastLastNode();
  // Move upstream to ensure we do not add redundant spans.
  start = MostBackwardCaretPosition(start);
  Node* start_node = start.AnchorNode();
  DCHECK(start_node);

  // Make sure we're not already at the end or the next NodeTraversal::next()
  // will traverse past it.
  if (start_node == beyond_end)
    return;

  if (start_node->IsTextNode() &&
      start.ComputeOffsetInContainerNode() >= CaretMaxOffset(start_node)) {
    // Move out of text node if range does not include its characters.
    start_node = NodeTraversal::Next(*start_node);
    if (!start_node)
      return;
  }

  // Store away font size before making any changes to the document.
  // This ensures that changes to one node won't effect another.
  HeapHashMap<Member<Node>, float> starting_font_sizes;
  for (Node* node = start_node; node != beyond_end;
       node = NodeTraversal::Next(*node)) {
    DCHECK(node);
    starting_font_sizes.Set(node, ComputedFontSize(node));
  }

  // These spans were added by us. If empty after font size changes, they can be
  // removed.
  HeapVector<Member<HTMLElement>> unstyled_spans;

  Node* last_styled_node = nullptr;
  for (Node* node = start_node; node != beyond_end;
       node = NodeTraversal::Next(*node)) {
    DCHECK(node);
    HTMLElement* element = nullptr;
    if (node->IsHTMLElement()) {
      // Only work on fully selected nodes.
      if (!ElementFullySelected(ToHTMLElement(*node), start, end))
        continue;
      element = ToHTMLElement(node);
    } else if (node->IsTextNode() && node->GetLayoutObject() &&
               node->parentNode() != last_styled_node) {
      // Last styled node was not parent node of this text node, but we wish to
      // style this text node. To make this possible, add a style span to
      // surround this text node.
      HTMLSpanElement* span = HTMLSpanElement::Create(GetDocument());
      SurroundNodeRangeWithElement(node, node, span, editing_state);
      if (editing_state->IsAborted())
        return;
      element = span;
    } else {
      // Only handle HTML elements and text nodes.
      continue;
    }
    last_styled_node = node;

    MutableStylePropertySet* inline_style =
        CopyStyleOrCreateEmpty(element->InlineStyle());
    float current_font_size = ComputedFontSize(node);
    float desired_font_size =
        max(kMinimumFontSize,
            starting_font_sizes.at(node) + style->FontSizeDelta());
    const CSSValue* value =
        inline_style->GetPropertyCSSValue(CSSPropertyFontSize);
    if (value) {
      element->RemoveInlineStyleProperty(CSSPropertyFontSize);
      current_font_size = ComputedFontSize(node);
    }
    if (current_font_size != desired_font_size) {
      inline_style->SetProperty(
          CSSPropertyFontSize,
          *CSSPrimitiveValue::Create(desired_font_size,
                                     CSSPrimitiveValue::UnitType::kPixels),
          false);
      SetNodeAttribute(element, styleAttr,
                       AtomicString(inline_style->AsText()));
    }
    if (inline_style->IsEmpty()) {
      RemoveElementAttribute(element, styleAttr);
      if (IsSpanWithoutAttributesOrUnstyledStyleSpan(element))
        unstyled_spans.push_back(element);
    }
  }

  for (const auto& unstyled_span : unstyled_spans) {
    RemoveNodePreservingChildren(unstyled_span, editing_state);
    if (editing_state->IsAborted())
      return;
  }
}

static ContainerNode* DummySpanAncestorForNode(const Node* node) {
  while (node && (!node->IsElementNode() ||
                  !IsStyleSpanOrSpanWithOnlyStyleAttribute(ToElement(node))))
    node = node->parentNode();

  return node ? node->parentNode() : 0;
}

void ApplyStyleCommand::CleanupUnstyledAppleStyleSpans(
    ContainerNode* dummy_span_ancestor,
    EditingState* editing_state) {
  if (!dummy_span_ancestor)
    return;

  // Dummy spans are created when text node is split, so that style information
  // can be propagated, which can result in more splitting. If a dummy span gets
  // cloned/split, the new node is always a sibling of it. Therefore, we scan
  // all the children of the dummy's parent
  Node* next;
  for (Node* node = dummy_span_ancestor->firstChild(); node; node = next) {
    next = node->nextSibling();
    if (IsSpanWithoutAttributesOrUnstyledStyleSpan(node)) {
      RemoveNodePreservingChildren(node, editing_state);
      if (editing_state->IsAborted())
        return;
    }
  }
}

HTMLElement* ApplyStyleCommand::SplitAncestorsWithUnicodeBidi(
    Node* node,
    bool before,
    WritingDirection allowed_direction) {
  // We are allowed to leave the highest ancestor with unicode-bidi unsplit if
  // it is unicode-bidi: embed and direction: allowedDirection. In that case, we
  // return the unsplit ancestor. Otherwise, we return 0.
  Element* block = EnclosingBlock(node);
  if (!block)
    return 0;

  ContainerNode* highest_ancestor_with_unicode_bidi = nullptr;
  ContainerNode* next_highest_ancestor_with_unicode_bidi = nullptr;
  int highest_ancestor_unicode_bidi = 0;
  for (Node& runner : NodeTraversal::AncestorsOf(*node)) {
    if (runner == block)
      break;
    int unicode_bidi = GetIdentifierValue(
        CSSComputedStyleDeclaration::Create(&runner), CSSPropertyUnicodeBidi);
    if (unicode_bidi && unicode_bidi != CSSValueNormal) {
      highest_ancestor_unicode_bidi = unicode_bidi;
      next_highest_ancestor_with_unicode_bidi =
          highest_ancestor_with_unicode_bidi;
      highest_ancestor_with_unicode_bidi = static_cast<ContainerNode*>(&runner);
    }
  }

  if (!highest_ancestor_with_unicode_bidi)
    return 0;

  HTMLElement* unsplit_ancestor = 0;

  WritingDirection highest_ancestor_direction;
  if (allowed_direction != NaturalWritingDirection &&
      highest_ancestor_unicode_bidi != CSSValueBidiOverride &&
      highest_ancestor_with_unicode_bidi->IsHTMLElement() &&
      EditingStyle::Create(highest_ancestor_with_unicode_bidi,
                           EditingStyle::kAllProperties)
          ->GetTextDirection(highest_ancestor_direction) &&
      highest_ancestor_direction == allowed_direction) {
    if (!next_highest_ancestor_with_unicode_bidi)
      return ToHTMLElement(highest_ancestor_with_unicode_bidi);

    unsplit_ancestor = ToHTMLElement(highest_ancestor_with_unicode_bidi);
    highest_ancestor_with_unicode_bidi =
        next_highest_ancestor_with_unicode_bidi;
  }

  // Split every ancestor through highest ancestor with embedding.
  Node* current_node = node;
  while (current_node) {
    Element* parent = ToElement(current_node->parentNode());
    if (before ? current_node->previousSibling() : current_node->nextSibling())
      SplitElement(parent, before ? current_node : current_node->nextSibling());
    if (parent == highest_ancestor_with_unicode_bidi)
      break;
    current_node = parent;
  }
  return unsplit_ancestor;
}

void ApplyStyleCommand::RemoveEmbeddingUpToEnclosingBlock(
    Node* node,
    HTMLElement* unsplit_ancestor,
    EditingState* editing_state) {
  Element* block = EnclosingBlock(node);
  if (!block)
    return;

  for (Node& runner : NodeTraversal::AncestorsOf(*node)) {
    if (runner == block || runner == unsplit_ancestor)
      break;
    if (!runner.IsStyledElement())
      continue;

    Element* element = ToElement(&runner);
    int unicode_bidi = GetIdentifierValue(
        CSSComputedStyleDeclaration::Create(element), CSSPropertyUnicodeBidi);
    if (!unicode_bidi || unicode_bidi == CSSValueNormal)
      continue;

    // FIXME: This code should really consider the mapped attribute 'dir', the
    // inline style declaration, and all matching style rules in order to
    // determine how to best set the unicode-bidi property to 'normal'. For now,
    // it assumes that if the 'dir' attribute is present, then removing it will
    // suffice, and otherwise it sets the property in the inline style
    // declaration.
    if (element->hasAttribute(dirAttr)) {
      // FIXME: If this is a BDO element, we should probably just remove it if
      // it has no other attributes, like we (should) do with B and I elements.
      RemoveElementAttribute(element, dirAttr);
    } else {
      MutableStylePropertySet* inline_style =
          CopyStyleOrCreateEmpty(element->InlineStyle());
      inline_style->SetProperty(CSSPropertyUnicodeBidi, CSSValueNormal);
      inline_style->RemoveProperty(CSSPropertyDirection);
      SetNodeAttribute(element, styleAttr,
                       AtomicString(inline_style->AsText()));
      if (IsSpanWithoutAttributesOrUnstyledStyleSpan(element)) {
        RemoveNodePreservingChildren(element, editing_state);
        if (editing_state->IsAborted())
          return;
      }
    }
  }
}

static HTMLElement* HighestEmbeddingAncestor(Node* start_node,
                                             Node* enclosing_node) {
  for (Node* n = start_node; n && n != enclosing_node; n = n->parentNode()) {
    if (n->IsHTMLElement() &&
        EditingStyleUtilities::IsEmbedOrIsolate(GetIdentifierValue(
            CSSComputedStyleDeclaration::Create(n), CSSPropertyUnicodeBidi))) {
      return ToHTMLElement(n);
    }
  }

  return 0;
}

void ApplyStyleCommand::ApplyInlineStyle(EditingStyle* style,
                                         EditingState* editing_state) {
  ContainerNode* start_dummy_span_ancestor = nullptr;
  ContainerNode* end_dummy_span_ancestor = nullptr;

  // update document layout once before removing styles
  // so that we avoid the expense of updating before each and every call
  // to check a computed style
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  // adjust to the positions we want to use for applying style
  Position start = StartPosition();
  Position end = EndPosition();

  if (start.IsNull() || end.IsNull())
    return;

  if (ComparePositions(end, start) < 0) {
    Position swap = start;
    start = end;
    end = swap;
  }

  // split the start node and containing element if the selection starts inside
  // of it
  bool split_start = IsValidCaretPositionInTextNode(start);
  if (split_start) {
    if (ShouldSplitTextElement(start.AnchorNode()->parentElement(), style))
      SplitTextElementAtStart(start, end);
    else
      SplitTextAtStart(start, end);
    start = StartPosition();
    end = EndPosition();
    if (start.IsNull() || end.IsNull())
      return;
    start_dummy_span_ancestor = DummySpanAncestorForNode(start.AnchorNode());
  }

  // split the end node and containing element if the selection ends inside of
  // it
  bool split_end = IsValidCaretPositionInTextNode(end);
  if (split_end) {
    if (ShouldSplitTextElement(end.AnchorNode()->parentElement(), style))
      SplitTextElementAtEnd(start, end);
    else
      SplitTextAtEnd(start, end);
    start = StartPosition();
    end = EndPosition();
    if (start.IsNull() || end.IsNull())
      return;
    end_dummy_span_ancestor = DummySpanAncestorForNode(end.AnchorNode());
  }

  // Remove style from the selection.
  // Use the upstream position of the start for removing style.
  // This will ensure we remove all traces of the relevant styles from the
  // selection and prevent us from adding redundant ones, as described in:
  // <rdar://problem/3724344> Bolding and unbolding creates extraneous tags
  Position remove_start = MostBackwardCaretPosition(start);
  WritingDirection text_direction = NaturalWritingDirection;
  bool has_text_direction = style->GetTextDirection(text_direction);
  EditingStyle* style_without_embedding = nullptr;
  EditingStyle* embedding_style = nullptr;
  if (has_text_direction) {
    // Leave alone an ancestor that provides the desired single level embedding,
    // if there is one.
    HTMLElement* start_unsplit_ancestor =
        SplitAncestorsWithUnicodeBidi(start.AnchorNode(), true, text_direction);
    HTMLElement* end_unsplit_ancestor =
        SplitAncestorsWithUnicodeBidi(end.AnchorNode(), false, text_direction);
    RemoveEmbeddingUpToEnclosingBlock(start.AnchorNode(),
                                      start_unsplit_ancestor, editing_state);
    if (editing_state->IsAborted())
      return;
    RemoveEmbeddingUpToEnclosingBlock(end.AnchorNode(), end_unsplit_ancestor,
                                      editing_state);
    if (editing_state->IsAborted())
      return;

    // Avoid removing the dir attribute and the unicode-bidi and direction
    // properties from the unsplit ancestors.
    Position embedding_remove_start = remove_start;
    if (start_unsplit_ancestor &&
        ElementFullySelected(*start_unsplit_ancestor, remove_start, end))
      embedding_remove_start =
          Position::InParentAfterNode(*start_unsplit_ancestor);

    Position embedding_remove_end = end;
    if (end_unsplit_ancestor &&
        ElementFullySelected(*end_unsplit_ancestor, remove_start, end))
      embedding_remove_end = MostForwardCaretPosition(
          Position::InParentBeforeNode(*end_unsplit_ancestor));

    if (embedding_remove_end != remove_start || embedding_remove_end != end) {
      style_without_embedding = style->Copy();
      embedding_style =
          style_without_embedding->ExtractAndRemoveTextDirection();

      if (ComparePositions(embedding_remove_start, embedding_remove_end) <= 0) {
        RemoveInlineStyle(embedding_style, embedding_remove_start,
                          embedding_remove_end, editing_state);
        if (editing_state->IsAborted())
          return;
      }
    }
  }

  RemoveInlineStyle(style_without_embedding ? style_without_embedding : style,
                    remove_start, end, editing_state);
  if (editing_state->IsAborted())
    return;
  start = StartPosition();
  end = EndPosition();
  if (start.IsNull() || start.IsOrphan() || end.IsNull() || end.IsOrphan())
    return;

  if (split_start) {
    bool merge_result =
        MergeStartWithPreviousIfIdentical(start, end, editing_state);
    if (editing_state->IsAborted())
      return;
    if (split_start && merge_result) {
      start = StartPosition();
      end = EndPosition();
    }
  }

  if (split_end) {
    MergeEndWithNextIfIdentical(start, end, editing_state);
    if (editing_state->IsAborted())
      return;
    start = StartPosition();
    end = EndPosition();
  }

  // update document layout once before running the rest of the function
  // so that we avoid the expense of updating before each and every call
  // to check a computed style
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  EditingStyle* style_to_apply = style;
  if (has_text_direction) {
    // Avoid applying the unicode-bidi and direction properties beneath
    // ancestors that already have them.
    HTMLElement* embedding_start_element = HighestEmbeddingAncestor(
        start.AnchorNode(), EnclosingBlock(start.AnchorNode()));
    HTMLElement* embedding_end_element = HighestEmbeddingAncestor(
        end.AnchorNode(), EnclosingBlock(end.AnchorNode()));

    if (embedding_start_element || embedding_end_element) {
      Position embedding_apply_start =
          embedding_start_element
              ? Position::InParentAfterNode(*embedding_start_element)
              : start;
      Position embedding_apply_end =
          embedding_end_element
              ? Position::InParentBeforeNode(*embedding_end_element)
              : end;
      DCHECK(embedding_apply_start.IsNotNull());
      DCHECK(embedding_apply_end.IsNotNull());

      if (!embedding_style) {
        style_without_embedding = style->Copy();
        embedding_style =
            style_without_embedding->ExtractAndRemoveTextDirection();
      }
      FixRangeAndApplyInlineStyle(embedding_style, embedding_apply_start,
                                  embedding_apply_end, editing_state);
      if (editing_state->IsAborted())
        return;

      style_to_apply = style_without_embedding;
    }
  }

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();
  FixRangeAndApplyInlineStyle(style_to_apply, start, end, editing_state);
  if (editing_state->IsAborted())
    return;

  // Remove dummy style spans created by splitting text elements.
  CleanupUnstyledAppleStyleSpans(start_dummy_span_ancestor, editing_state);
  if (editing_state->IsAborted())
    return;
  if (end_dummy_span_ancestor != start_dummy_span_ancestor)
    CleanupUnstyledAppleStyleSpans(end_dummy_span_ancestor, editing_state);
}

void ApplyStyleCommand::FixRangeAndApplyInlineStyle(
    EditingStyle* style,
    const Position& start,
    const Position& end,
    EditingState* editing_state) {
  Node* start_node = start.AnchorNode();
  DCHECK(start_node);

  if (start.ComputeEditingOffset() >= CaretMaxOffset(start.AnchorNode())) {
    start_node = NodeTraversal::Next(*start_node);
    if (!start_node ||
        ComparePositions(end, FirstPositionInOrBeforeNode(*start_node)) < 0)
      return;
  }

  Node* past_end_node = end.AnchorNode();
  if (end.ComputeEditingOffset() >= CaretMaxOffset(end.AnchorNode()))
    past_end_node = NodeTraversal::NextSkippingChildren(*end.AnchorNode());

  // FIXME: Callers should perform this operation on a Range that includes the
  // br if they want style applied to the empty line.
  if (start == end && IsHTMLBRElement(*start.AnchorNode()))
    past_end_node = NodeTraversal::Next(*start.AnchorNode());

  // Start from the highest fully selected ancestor so that we can modify the
  // fully selected node. e.g. When applying font-size: large on <font
  // color="blue">hello</font>, we need to include the font element in our run
  // to generate <font color="blue" size="4">hello</font> instead of <font
  // color="blue"><font size="4">hello</font></font>
  Range* range = Range::Create(start_node->GetDocument(), start, end);
  Element* editable_root = RootEditableElement(*start_node);
  if (start_node != editable_root) {
    while (editable_root && start_node->parentNode() != editable_root &&
           IsNodeVisiblyContainedWithin(*start_node->parentNode(), *range))
      start_node = start_node->parentNode();
  }

  ApplyInlineStyleToNodeRange(style, start_node, past_end_node, editing_state);
}

static bool ContainsNonEditableRegion(Node& node) {
  if (!HasEditableStyle(node))
    return true;

  Node* sibling = NodeTraversal::NextSkippingChildren(node);
  for (Node* descendent = node.firstChild();
       descendent && descendent != sibling;
       descendent = NodeTraversal::Next(*descendent)) {
    if (!HasEditableStyle(*descendent))
      return true;
  }

  return false;
}

class InlineRunToApplyStyle {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  InlineRunToApplyStyle(Node* start, Node* end, Node* past_end_node)
      : start(start), end(end), past_end_node(past_end_node) {
    DCHECK_EQ(start->parentNode(), end->parentNode());
  }

  bool StartAndEndAreStillInDocument() {
    return start && end && start->isConnected() && end->isConnected();
  }

  DEFINE_INLINE_TRACE() {
    visitor->Trace(start);
    visitor->Trace(end);
    visitor->Trace(past_end_node);
    visitor->Trace(position_for_style_computation);
    visitor->Trace(dummy_element);
  }

  Member<Node> start;
  Member<Node> end;
  Member<Node> past_end_node;
  Position position_for_style_computation;
  Member<HTMLSpanElement> dummy_element;
  StyleChange change;
};

}  // namespace blink

WTF_ALLOW_INIT_WITH_MEM_FUNCTIONS(blink::InlineRunToApplyStyle);

namespace blink {

void ApplyStyleCommand::ApplyInlineStyleToNodeRange(
    EditingStyle* style,
    Node* start_node,
    Node* past_end_node,
    EditingState* editing_state) {
  if (remove_only_)
    return;

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  HeapVector<InlineRunToApplyStyle> runs;
  Node* node = start_node;
  for (Node* next; node && node != past_end_node; node = next) {
    next = NodeTraversal::Next(*node);

    if (!node->GetLayoutObject() || !HasEditableStyle(*node))
      continue;

    if (!HasRichlyEditableStyle(*node) && node->IsHTMLElement()) {
      HTMLElement* element = ToHTMLElement(node);
      // This is a plaintext-only region. Only proceed if it's fully selected.
      // pastEndNode is the node after the last fully selected node, so if it's
      // inside node then node isn't fully selected.
      if (past_end_node && past_end_node->IsDescendantOf(element))
        break;
      // Add to this element's inline style and skip over its contents.
      next = NodeTraversal::NextSkippingChildren(*node);
      if (!style->Style())
        continue;
      MutableStylePropertySet* inline_style =
          CopyStyleOrCreateEmpty(element->InlineStyle());
      inline_style->MergeAndOverrideOnConflict(style->Style());
      SetNodeAttribute(element, styleAttr,
                       AtomicString(inline_style->AsText()));
      continue;
    }

    if (IsEnclosingBlock(node))
      continue;

    if (node->hasChildren()) {
      if (node->contains(past_end_node) || ContainsNonEditableRegion(*node) ||
          !HasEditableStyle(*node->parentNode()))
        continue;
      if (EditingIgnoresContent(*node)) {
        next = NodeTraversal::NextSkippingChildren(*node);
        continue;
      }
    }

    Node* run_start = node;
    Node* run_end = node;
    Node* sibling = node->nextSibling();
    while (sibling && sibling != past_end_node &&
           !sibling->contains(past_end_node) &&
           (!IsEnclosingBlock(sibling) || IsHTMLBRElement(*sibling)) &&
           !ContainsNonEditableRegion(*sibling)) {
      run_end = sibling;
      sibling = run_end->nextSibling();
    }
    DCHECK(run_end);
    next = NodeTraversal::NextSkippingChildren(*run_end);

    Node* past_end_node = NodeTraversal::NextSkippingChildren(*run_end);
    if (!ShouldApplyInlineStyleToRun(style, run_start, past_end_node))
      continue;

    runs.push_back(InlineRunToApplyStyle(run_start, run_end, past_end_node));
  }

  for (auto& run : runs) {
    RemoveConflictingInlineStyleFromRun(style, run.start, run.end,
                                        run.past_end_node, editing_state);
    if (editing_state->IsAborted())
      return;
    if (run.StartAndEndAreStillInDocument()) {
      run.position_for_style_computation = PositionToComputeInlineStyleChange(
          run.start, run.dummy_element, editing_state);
      if (editing_state->IsAborted())
        return;
    }
  }

  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  for (auto& run : runs) {
    if (run.position_for_style_computation.IsNotNull())
      run.change = StyleChange(style, run.position_for_style_computation);
  }

  for (auto& run : runs) {
    if (run.dummy_element) {
      RemoveNode(run.dummy_element, editing_state);
      if (editing_state->IsAborted())
        return;
    }
    if (run.StartAndEndAreStillInDocument()) {
      ApplyInlineStyleChange(run.start.Release(), run.end.Release(), run.change,
                             kAddStyledElement, editing_state);
      if (editing_state->IsAborted())
        return;
    }
  }
}

bool ApplyStyleCommand::IsStyledInlineElementToRemove(Element* element) const {
  return (styled_inline_element_ &&
          element->HasTagName(styled_inline_element_->TagQName())) ||
         (is_inline_element_to_remove_function_ &&
          is_inline_element_to_remove_function_(element));
}

bool ApplyStyleCommand::ShouldApplyInlineStyleToRun(EditingStyle* style,
                                                    Node* run_start,
                                                    Node* past_end_node) {
  DCHECK(style);
  DCHECK(run_start);

  for (Node* node = run_start; node && node != past_end_node;
       node = NodeTraversal::Next(*node)) {
    if (node->hasChildren())
      continue;
    // We don't consider m_isInlineElementToRemoveFunction here because we never
    // apply style when m_isInlineElementToRemoveFunction is specified
    if (!style->StyleIsPresentInComputedStyleOfNode(node))
      return true;
    if (styled_inline_element_ &&
        !EnclosingElementWithTag(Position::BeforeNode(*node),
                                 styled_inline_element_->TagQName()))
      return true;
  }
  return false;
}

void ApplyStyleCommand::RemoveConflictingInlineStyleFromRun(
    EditingStyle* style,
    Member<Node>& run_start,
    Member<Node>& run_end,
    Node* past_end_node,
    EditingState* editing_state) {
  DCHECK(run_start);
  DCHECK(run_end);
  Node* next = run_start;
  for (Node* node = next; node && node->isConnected() && node != past_end_node;
       node = next) {
    if (EditingIgnoresContent(*node)) {
      DCHECK(!node->contains(past_end_node)) << node << " " << past_end_node;
      next = NodeTraversal::NextSkippingChildren(*node);
    } else {
      next = NodeTraversal::Next(*node);
    }
    if (!node->IsHTMLElement())
      continue;

    HTMLElement& element = ToHTMLElement(*node);
    Node* previous_sibling = element.previousSibling();
    Node* next_sibling = element.nextSibling();
    ContainerNode* parent = element.parentNode();
    RemoveInlineStyleFromElement(style, &element, editing_state, kRemoveAlways);
    if (editing_state->IsAborted())
      return;
    if (!element.isConnected()) {
      // FIXME: We might need to update the start and the end of current
      // selection here but need a test.
      if (run_start == element)
        run_start = previous_sibling ? previous_sibling->nextSibling()
                                     : parent->firstChild();
      if (run_end == element)
        run_end = next_sibling ? next_sibling->previousSibling()
                               : parent->lastChild();
    }
  }
}

bool ApplyStyleCommand::RemoveInlineStyleFromElement(
    EditingStyle* style,
    HTMLElement* element,
    EditingState* editing_state,
    InlineStyleRemovalMode mode,
    EditingStyle* extracted_style) {
  DCHECK(element);
  GetDocument().UpdateStyleAndLayoutTree();
  if (!element->parentNode() || !HasEditableStyle(*element->parentNode()))
    return false;

  if (IsStyledInlineElementToRemove(element)) {
    if (mode == kRemoveNone)
      return true;
    if (extracted_style)
      extracted_style->MergeInlineStyleOfElement(element,
                                                 EditingStyle::kOverrideValues);
    RemoveNodePreservingChildren(element, editing_state);
    if (editing_state->IsAborted())
      return false;
    return true;
  }

  bool removed = RemoveImplicitlyStyledElement(style, element, mode,
                                               extracted_style, editing_state);
  if (editing_state->IsAborted())
    return false;

  if (!element->isConnected())
    return removed;

  // If the node was converted to a span, the span may still contain relevant
  // styles which must be removed (e.g. <b style='font-weight: bold'>)
  if (RemoveCSSStyle(style, element, editing_state, mode, extracted_style))
    removed = true;
  if (editing_state->IsAborted())
    return false;

  return removed;
}

void ApplyStyleCommand::ReplaceWithSpanOrRemoveIfWithoutAttributes(
    HTMLElement* elem,
    EditingState* editing_state) {
  if (HasNoAttributeOrOnlyStyleAttribute(elem, kStyleAttributeShouldBeEmpty))
    RemoveNodePreservingChildren(elem, editing_state);
  else
    ReplaceElementWithSpanPreservingChildrenAndAttributes(elem);
}

bool ApplyStyleCommand::RemoveImplicitlyStyledElement(
    EditingStyle* style,
    HTMLElement* element,
    InlineStyleRemovalMode mode,
    EditingStyle* extracted_style,
    EditingState* editing_state) {
  DCHECK(style);
  if (mode == kRemoveNone) {
    DCHECK(!extracted_style);
    return style->ConflictsWithImplicitStyleOfElement(element) ||
           style->ConflictsWithImplicitStyleOfAttributes(element);
  }

  DCHECK(mode == kRemoveIfNeeded || mode == kRemoveAlways);
  if (style->ConflictsWithImplicitStyleOfElement(
          element, extracted_style,
          mode == kRemoveAlways ? EditingStyle::kExtractMatchingStyle
                                : EditingStyle::kDoNotExtractMatchingStyle)) {
    ReplaceWithSpanOrRemoveIfWithoutAttributes(element, editing_state);
    if (editing_state->IsAborted())
      return false;
    return true;
  }

  // unicode-bidi and direction are pushed down separately so don't push down
  // with other styles
  Vector<QualifiedName> attributes;
  if (!style->ExtractConflictingImplicitStyleOfAttributes(
          element,
          extracted_style ? EditingStyle::kPreserveWritingDirection
                          : EditingStyle::kDoNotPreserveWritingDirection,
          extracted_style, attributes,
          mode == kRemoveAlways ? EditingStyle::kExtractMatchingStyle
                                : EditingStyle::kDoNotExtractMatchingStyle))
    return false;

  for (const auto& attribute : attributes)
    RemoveElementAttribute(element, attribute);

  if (IsEmptyFontTag(element) ||
      IsSpanWithoutAttributesOrUnstyledStyleSpan(element)) {
    RemoveNodePreservingChildren(element, editing_state);
    if (editing_state->IsAborted())
      return false;
  }

  return true;
}

bool ApplyStyleCommand::RemoveCSSStyle(EditingStyle* style,
                                       HTMLElement* element,
                                       EditingState* editing_state,
                                       InlineStyleRemovalMode mode,
                                       EditingStyle* extracted_style) {
  DCHECK(style);
  DCHECK(element);

  if (mode == kRemoveNone)
    return style->ConflictsWithInlineStyleOfElement(element);

  Vector<CSSPropertyID> properties;
  if (!style->ConflictsWithInlineStyleOfElement(element, extracted_style,
                                                properties))
    return false;

  // FIXME: We should use a mass-removal function here but we don't have an
  // undoable one yet.
  for (const auto& property : properties)
    RemoveCSSProperty(element, property);

  if (IsSpanWithoutAttributesOrUnstyledStyleSpan(element))
    RemoveNodePreservingChildren(element, editing_state);

  return true;
}

// Finds the enclosing element until which the tree can be split.
// When a user hits ENTER, they won't expect this element to be split into two.
// You may pass it as the second argument of splitTreeToNode.
static Element* UnsplittableElementForPosition(const Position& p) {
  // Since enclosingNodeOfType won't search beyond the highest root editable
  // node, this code works even if the closest table cell was outside of the
  // root editable node.
  Element* enclosing_cell = ToElement(EnclosingNodeOfType(p, &IsTableCell));
  if (enclosing_cell)
    return enclosing_cell;

  return RootEditableElementOf(p);
}

HTMLElement* ApplyStyleCommand::HighestAncestorWithConflictingInlineStyle(
    EditingStyle* style,
    Node* node) {
  if (!node)
    return 0;

  HTMLElement* result = nullptr;
  Node* unsplittable_element =
      UnsplittableElementForPosition(FirstPositionInOrBeforeNode(*node));

  for (Node* n = node; n; n = n->parentNode()) {
    if (n->IsHTMLElement() &&
        ShouldRemoveInlineStyleFromElement(style, ToHTMLElement(n)))
      result = ToHTMLElement(n);
    // Should stop at the editable root (cannot cross editing boundary) and
    // also stop at the unsplittable element to be consistent with other UAs
    if (n == unsplittable_element)
      break;
  }

  return result;
}

void ApplyStyleCommand::ApplyInlineStyleToPushDown(
    Node* node,
    EditingStyle* style,
    EditingState* editing_state) {
  DCHECK(node);

  node->GetDocument().UpdateStyleAndLayoutTree();

  if (!style || style->IsEmpty() || !node->GetLayoutObject() ||
      IsHTMLIFrameElement(*node))
    return;

  EditingStyle* new_inline_style = style;
  if (node->IsHTMLElement() && ToHTMLElement(node)->InlineStyle()) {
    new_inline_style = style->Copy();
    new_inline_style->MergeInlineStyleOfElement(ToHTMLElement(node),
                                                EditingStyle::kOverrideValues);
  }

  // Since addInlineStyleIfNeeded can't add styles to block-flow layout objects,
  // add style attribute instead.
  // FIXME: applyInlineStyleToRange should be used here instead.
  if ((node->GetLayoutObject()->IsLayoutBlockFlow() || node->hasChildren()) &&
      node->IsHTMLElement()) {
    SetNodeAttribute(ToHTMLElement(node), styleAttr,
                     AtomicString(new_inline_style->Style()->AsText()));
    return;
  }

  if (node->GetLayoutObject()->IsText() &&
      ToLayoutText(node->GetLayoutObject())->IsAllCollapsibleWhitespace())
    return;

  // We can't wrap node with the styled element here because new styled element
  // will never be removed if we did. If we modified the child pointer in
  // pushDownInlineStyleAroundNode to point to new style element then we fall
  // into an infinite loop where we keep removing and adding styled element
  // wrapping node.
  AddInlineStyleIfNeeded(new_inline_style, node, node, editing_state);
}

void ApplyStyleCommand::PushDownInlineStyleAroundNode(
    EditingStyle* style,
    Node* target_node,
    EditingState* editing_state) {
  HTMLElement* highest_ancestor =
      HighestAncestorWithConflictingInlineStyle(style, target_node);
  if (!highest_ancestor)
    return;

  // The outer loop is traversing the tree vertically from highestAncestor to
  // targetNode
  Node* current = highest_ancestor;
  // Along the way, styled elements that contain targetNode are removed and
  // accumulated into elementsToPushDown. Each child of the removed element,
  // exclusing ancestors of targetNode, is then wrapped by clones of elements in
  // elementsToPushDown.
  HeapVector<Member<Element>> elements_to_push_down;
  while (current && current != target_node && current->contains(target_node)) {
    NodeVector current_children;
    GetChildNodes(ToContainerNode(*current), current_children);
    Element* styled_element = nullptr;
    if (current->IsStyledElement() &&
        IsStyledInlineElementToRemove(ToElement(current))) {
      styled_element = ToElement(current);
      elements_to_push_down.push_back(styled_element);
    }

    EditingStyle* style_to_push_down = EditingStyle::Create();
    if (current->IsHTMLElement()) {
      RemoveInlineStyleFromElement(style, ToHTMLElement(current), editing_state,
                                   kRemoveIfNeeded, style_to_push_down);
      if (editing_state->IsAborted())
        return;
    }

    // The inner loop will go through children on each level
    // FIXME: we should aggregate inline child elements together so that we
    // don't wrap each child separately.
    for (const auto& current_child : current_children) {
      Node* child = current_child;
      if (!child->parentNode())
        continue;
      if (!child->contains(target_node) && elements_to_push_down.size()) {
        for (const auto& element : elements_to_push_down) {
          Element* wrapper = element->CloneElementWithoutChildren();
          wrapper->removeAttribute(styleAttr);
          // Delete id attribute from the second element because the same id
          // cannot be used for more than one element
          element->removeAttribute(HTMLNames::idAttr);
          if (IsHTMLAnchorElement(element))
            element->removeAttribute(HTMLNames::nameAttr);
          SurroundNodeRangeWithElement(child, child, wrapper, editing_state);
          if (editing_state->IsAborted())
            return;
        }
      }

      // Apply style to all nodes containing targetNode and their siblings but
      // NOT to targetNode But if we've removed styledElement then go ahead and
      // always apply the style.
      if (child != target_node || styled_element) {
        ApplyInlineStyleToPushDown(child, style_to_push_down, editing_state);
        if (editing_state->IsAborted())
          return;
      }

      // We found the next node for the outer loop (contains targetNode)
      // When reached targetNode, stop the outer loop upon the completion of the
      // current inner loop
      if (child == target_node || child->contains(target_node))
        current = child;
    }
  }
}

void ApplyStyleCommand::RemoveInlineStyle(EditingStyle* style,
                                          const Position& start,
                                          const Position& end,
                                          EditingState* editing_state) {
  DCHECK(start.IsNotNull());
  DCHECK(end.IsNotNull());
  DCHECK(start.IsConnected()) << start;
  DCHECK(end.IsConnected()) << end;
  DCHECK(Position::CommonAncestorTreeScope(start, end)) << start << " " << end;
  DCHECK_LE(start, end);
  // FIXME: We should assert that start/end are not in the middle of a text
  // node.

  // TODO(editing-dev): Use of updateStyleAndLayoutIgnorePendingStylesheets
  // needs to be audited.  See http://crbug.com/590369 for more details.
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  Position push_down_start = MostForwardCaretPosition(start);
  // If the pushDownStart is at the end of a text node, then this node is not
  // fully selected. Move it to the next deep quivalent position to avoid
  // removing the style from this node. e.g. if pushDownStart was at
  // Position("hello", 5) in <b>hello<div>world</div></b>, we want
  // Position("world", 0) instead.
  Node* push_down_start_container = push_down_start.ComputeContainerNode();
  if (push_down_start_container && push_down_start_container->IsTextNode() &&
      push_down_start.ComputeOffsetInContainerNode() ==
          push_down_start_container->MaxCharacterOffset())
    push_down_start = NextVisuallyDistinctCandidate(push_down_start);

  // TODO(editing-dev): Use of updateStyleAndLayoutIgnorePendingStylesheets
  // needs to be audited.  See http://crbug.com/590369 for more details.
  GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  Position push_down_end = MostBackwardCaretPosition(end);
  // If pushDownEnd is at the start of a text node, then this node is not fully
  // selected. Move it to the previous deep equivalent position to avoid
  // removing the style from this node.
  Node* push_down_end_container = push_down_end.ComputeContainerNode();
  if (push_down_end_container && push_down_end_container->IsTextNode() &&
      !push_down_end.ComputeOffsetInContainerNode())
    push_down_end = PreviousVisuallyDistinctCandidate(push_down_end);

  PushDownInlineStyleAroundNode(style, push_down_start.AnchorNode(),
                                editing_state);
  if (editing_state->IsAborted())
    return;
  PushDownInlineStyleAroundNode(style, push_down_end.AnchorNode(),
                                editing_state);
  if (editing_state->IsAborted())
    return;

  // The s and e variables store the positions used to set the ending selection
  // after style removal takes place. This will help callers to recognize when
  // either the start node or the end node are removed from the document during
  // the work of this function.
  // If pushDownInlineStyleAroundNode has pruned start.anchorNode() or
  // end.anchorNode(), use pushDownStart or pushDownEnd instead, which
  // pushDownInlineStyleAroundNode won't prune.
  Position s = start.IsNull() || start.IsOrphan() ? push_down_start : start;
  Position e = end.IsNull() || end.IsOrphan() ? push_down_end : end;

  // Current ending selection resetting algorithm assumes |start| and |end|
  // are in a same DOM tree even if they are not in document.
  if (!Position::CommonAncestorTreeScope(start, end))
    return;

  Node* node = start.AnchorNode();
  while (node) {
    Node* next = nullptr;
    if (EditingIgnoresContent(*node)) {
      DCHECK(node == end.AnchorNode() || !node->contains(end.AnchorNode()))
          << node << " " << end;
      next = NodeTraversal::NextSkippingChildren(*node);
    } else {
      next = NodeTraversal::Next(*node);
    }
    if (node->IsHTMLElement() &&
        ElementFullySelected(ToHTMLElement(*node), start, end)) {
      HTMLElement* elem = ToHTMLElement(node);
      Node* prev = NodeTraversal::PreviousPostOrder(*elem);
      Node* next = NodeTraversal::Next(*elem);
      EditingStyle* style_to_push_down = nullptr;
      Node* child_node = nullptr;
      if (IsStyledInlineElementToRemove(elem)) {
        style_to_push_down = EditingStyle::Create();
        child_node = elem->firstChild();
      }

      RemoveInlineStyleFromElement(style, elem, editing_state, kRemoveIfNeeded,
                                   style_to_push_down);
      if (editing_state->IsAborted())
        return;
      if (!elem->isConnected()) {
        if (s.AnchorNode() == elem) {
          // Since elem must have been fully selected, and it is at the start
          // of the selection, it is clear we can set the new s offset to 0.
          DCHECK(s.IsBeforeAnchor() || s.IsBeforeChildren() ||
                 s.OffsetInContainerNode() <= 0)
              << s;
          s = FirstPositionInOrBeforeNodeDeprecated(next);
        }
        if (e.AnchorNode() == elem) {
          // Since elem must have been fully selected, and it is at the end
          // of the selection, it is clear we can set the new e offset to
          // the max range offset of prev.
          DCHECK(s.IsAfterAnchor() ||
                 !OffsetIsBeforeLastNodeOffset(s.OffsetInContainerNode(),
                                               s.ComputeContainerNode()))
              << s;
          e = LastPositionInOrAfterNodeDeprecated(prev);
        }
      }

      if (style_to_push_down) {
        for (; child_node; child_node = child_node->nextSibling()) {
          ApplyInlineStyleToPushDown(child_node, style_to_push_down,
                                     editing_state);
          if (editing_state->IsAborted())
            return;
        }
      }
    }
    if (node == end.AnchorNode())
      break;
    node = next;
  }

  UpdateStartEnd(s, e);
}

bool ApplyStyleCommand::ElementFullySelected(const HTMLElement& element,
                                             const Position& start,
                                             const Position& end) const {
  // The tree may have changed and Position::upstream() relies on an up-to-date
  // layout.
  element.GetDocument().UpdateStyleAndLayoutIgnorePendingStylesheets();

  return ComparePositions(FirstPositionInOrBeforeNode(element), start) >= 0 &&
         ComparePositions(
             MostBackwardCaretPosition(LastPositionInOrAfterNode(element)),
             end) <= 0;
}

void ApplyStyleCommand::SplitTextAtStart(const Position& start,
                                         const Position& end) {
  DCHECK(start.ComputeContainerNode()->IsTextNode()) << start;

  Position new_end;
  if (end.IsOffsetInAnchor() &&
      start.ComputeContainerNode() == end.ComputeContainerNode())
    new_end =
        Position(end.ComputeContainerNode(),
                 end.OffsetInContainerNode() - start.OffsetInContainerNode());
  else
    new_end = end;

  Text* text = ToText(start.ComputeContainerNode());
  SplitTextNode(text, start.OffsetInContainerNode());
  UpdateStartEnd(Position::FirstPositionInNode(*text), new_end);
}

void ApplyStyleCommand::SplitTextAtEnd(const Position& start,
                                       const Position& end) {
  DCHECK(end.ComputeContainerNode()->IsTextNode()) << end;

  bool should_update_start =
      start.IsOffsetInAnchor() &&
      start.ComputeContainerNode() == end.ComputeContainerNode();
  Text* text = ToText(end.AnchorNode());
  SplitTextNode(text, end.OffsetInContainerNode());

  Node* prev_node = text->previousSibling();
  if (!prev_node || !prev_node->IsTextNode())
    return;

  Position new_start =
      should_update_start
          ? Position(ToText(prev_node), start.OffsetInContainerNode())
          : start;
  UpdateStartEnd(new_start, Position::LastPositionInNode(*prev_node));
}

void ApplyStyleCommand::SplitTextElementAtStart(const Position& start,
                                                const Position& end) {
  DCHECK(start.ComputeContainerNode()->IsTextNode()) << start;

  Position new_end;
  if (start.ComputeContainerNode() == end.ComputeContainerNode())
    new_end =
        Position(end.ComputeContainerNode(),
                 end.OffsetInContainerNode() - start.OffsetInContainerNode());
  else
    new_end = end;

  SplitTextNodeContainingElement(ToText(start.ComputeContainerNode()),
                                 start.OffsetInContainerNode());
  UpdateStartEnd(Position::BeforeNode(*start.ComputeContainerNode()), new_end);
}

void ApplyStyleCommand::SplitTextElementAtEnd(const Position& start,
                                              const Position& end) {
  DCHECK(end.ComputeContainerNode()->IsTextNode()) << end;

  bool should_update_start =
      start.ComputeContainerNode() == end.ComputeContainerNode();
  SplitTextNodeContainingElement(ToText(end.ComputeContainerNode()),
                                 end.OffsetInContainerNode());

  Node* parent_element = end.ComputeContainerNode()->parentNode();
  if (!parent_element || !parent_element->previousSibling())
    return;
  Node* first_text_node = parent_element->previousSibling()->lastChild();
  if (!first_text_node || !first_text_node->IsTextNode())
    return;

  Position new_start =
      should_update_start
          ? Position(ToText(first_text_node), start.OffsetInContainerNode())
          : start;
  UpdateStartEnd(new_start, Position::AfterNode(*first_text_node));
}

bool ApplyStyleCommand::ShouldSplitTextElement(Element* element,
                                               EditingStyle* style) {
  if (!element || !element->IsHTMLElement())
    return false;

  return ShouldRemoveInlineStyleFromElement(style, ToHTMLElement(element));
}

bool ApplyStyleCommand::IsValidCaretPositionInTextNode(
    const Position& position) {
  DCHECK(position.IsNotNull());

  Node* node = position.ComputeContainerNode();
  if (!position.IsOffsetInAnchor() || !node->IsTextNode())
    return false;
  int offset_in_text = position.OffsetInContainerNode();
  return offset_in_text > CaretMinOffset(node) &&
         offset_in_text < CaretMaxOffset(node);
}

bool ApplyStyleCommand::MergeStartWithPreviousIfIdentical(
    const Position& start,
    const Position& end,
    EditingState* editing_state) {
  Node* start_node = start.ComputeContainerNode();
  int start_offset = start.ComputeOffsetInContainerNode();
  if (start_offset)
    return false;

  if (IsAtomicNode(start_node)) {
    // note: prior siblings could be unrendered elements. it's silly to miss the
    // merge opportunity just for that.
    if (start_node->previousSibling())
      return false;

    start_node = start_node->parentNode();
  }

  if (!start_node->IsElementNode())
    return false;

  Node* previous_sibling = start_node->previousSibling();

  if (previous_sibling &&
      AreIdenticalElements(*start_node, *previous_sibling)) {
    Element* previous_element = ToElement(previous_sibling);
    Element* element = ToElement(start_node);
    Node* start_child = element->firstChild();
    DCHECK(start_child);
    MergeIdenticalElements(previous_element, element, editing_state);
    if (editing_state->IsAborted())
      return false;

    int start_offset_adjustment = start_child->NodeIndex();
    int end_offset_adjustment =
        start_node == end.AnchorNode() ? start_offset_adjustment : 0;
    UpdateStartEnd(Position(start_node, start_offset_adjustment),
                   Position(end.AnchorNode(), end.ComputeEditingOffset() +
                                                  end_offset_adjustment));
    return true;
  }

  return false;
}

bool ApplyStyleCommand::MergeEndWithNextIfIdentical(
    const Position& start,
    const Position& end,
    EditingState* editing_state) {
  Node* end_node = end.ComputeContainerNode();

  if (IsAtomicNode(end_node)) {
    int end_offset = end.ComputeOffsetInContainerNode();
    if (OffsetIsBeforeLastNodeOffset(end_offset, end_node))
      return false;

    if (end.AnchorNode()->nextSibling())
      return false;

    end_node = end.AnchorNode()->parentNode();
  }

  if (!end_node->IsElementNode() || IsHTMLBRElement(*end_node))
    return false;

  Node* next_sibling = end_node->nextSibling();
  if (next_sibling && AreIdenticalElements(*end_node, *next_sibling)) {
    Element* next_element = ToElement(next_sibling);
    Element* element = ToElement(end_node);
    Node* next_child = next_element->firstChild();

    MergeIdenticalElements(element, next_element, editing_state);
    if (editing_state->IsAborted())
      return false;

    bool should_update_start = start.ComputeContainerNode() == end_node;
    int end_offset = next_child ? next_child->NodeIndex()
                                : next_element->childNodes()->length();
    UpdateStartEnd(should_update_start
                       ? Position(next_element, start.OffsetInContainerNode())
                       : start,
                   Position(next_element, end_offset));
    return true;
  }

  return false;
}

void ApplyStyleCommand::SurroundNodeRangeWithElement(
    Node* passed_start_node,
    Node* end_node,
    Element* element_to_insert,
    EditingState* editing_state) {
  DCHECK(passed_start_node);
  DCHECK(end_node);
  DCHECK(element_to_insert);
  Node* node = passed_start_node;
  Element* element = element_to_insert;

  InsertNodeBefore(element, node, editing_state);
  if (editing_state->IsAborted())
    return;

  GetDocument().UpdateStyleAndLayoutTree();
  while (node) {
    Node* next = node->nextSibling();
    if (HasEditableStyle(*node)) {
      RemoveNode(node, editing_state);
      if (editing_state->IsAborted())
        return;
      AppendNode(node, element, editing_state);
      if (editing_state->IsAborted())
        return;
    }
    if (node == end_node)
      break;
    node = next;
  }

  Node* next_sibling = element->nextSibling();
  Node* previous_sibling = element->previousSibling();
  if (next_sibling && next_sibling->IsElementNode() &&
      HasEditableStyle(*next_sibling) &&
      AreIdenticalElements(*element, ToElement(*next_sibling))) {
    MergeIdenticalElements(element, ToElement(next_sibling), editing_state);
    if (editing_state->IsAborted())
      return;
  }

  if (previous_sibling && previous_sibling->IsElementNode() &&
      HasEditableStyle(*previous_sibling)) {
    Node* merged_element = previous_sibling->nextSibling();
    if (merged_element->IsElementNode() && HasEditableStyle(*merged_element) &&
        AreIdenticalElements(ToElement(*previous_sibling),
                             ToElement(*merged_element))) {
      MergeIdenticalElements(ToElement(previous_sibling),
                             ToElement(merged_element), editing_state);
      if (editing_state->IsAborted())
        return;
    }
  }

  // FIXME: We should probably call updateStartEnd if the start or end was in
  // the node range so that the endingSelection() is canonicalized.  See the
  // comments at the end of VisibleSelection::validate().
}

void ApplyStyleCommand::AddBlockStyle(const StyleChange& style_change,
                                      HTMLElement* block) {
  // Do not check for legacy styles here. Those styles, like <B> and <I>, only
  // apply for inline content.
  if (!block)
    return;

  String css_style = style_change.CssStyle();
  StringBuilder css_text;
  css_text.Append(css_style);
  if (const StylePropertySet* decl = block->InlineStyle()) {
    if (!css_style.IsEmpty())
      css_text.Append(' ');
    css_text.Append(decl->AsText());
  }
  SetNodeAttribute(block, styleAttr, css_text.ToAtomicString());
}

void ApplyStyleCommand::AddInlineStyleIfNeeded(EditingStyle* style,
                                               Node* passed_start,
                                               Node* passed_end,
                                               EditingState* editing_state) {
  if (!passed_start || !passed_end || !passed_start->isConnected() ||
      !passed_end->isConnected())
    return;

  Node* start = passed_start;
  Member<HTMLSpanElement> dummy_element = nullptr;
  StyleChange style_change(style, PositionToComputeInlineStyleChange(
                                      start, dummy_element, editing_state));
  if (editing_state->IsAborted())
    return;

  if (dummy_element) {
    RemoveNode(dummy_element, editing_state);
    if (editing_state->IsAborted())
      return;
  }

  ApplyInlineStyleChange(start, passed_end, style_change,
                         kDoNotAddStyledElement, editing_state);
}

Position ApplyStyleCommand::PositionToComputeInlineStyleChange(
    Node* start_node,
    Member<HTMLSpanElement>& dummy_element,
    EditingState* editing_state) {
  DCHECK(start_node);
  // It's okay to obtain the style at the startNode because we've removed all
  // relevant styles from the current run.
  if (!start_node->IsElementNode()) {
    dummy_element = HTMLSpanElement::Create(GetDocument());
    InsertNodeAt(dummy_element, Position::BeforeNode(*start_node),
                 editing_state);
    if (editing_state->IsAborted())
      return Position();
    return Position::BeforeNode(*dummy_element);
  }

  return FirstPositionInOrBeforeNode(*start_node);
}

void ApplyStyleCommand::ApplyInlineStyleChange(
    Node* passed_start,
    Node* passed_end,
    StyleChange& style_change,
    EAddStyledElement add_styled_element,
    EditingState* editing_state) {
  Node* start_node = passed_start;
  Node* end_node = passed_end;
  DCHECK(start_node->isConnected()) << start_node;
  DCHECK(end_node->isConnected()) << end_node;

  // Find appropriate font and span elements top-down.
  HTMLFontElement* font_container = nullptr;
  HTMLElement* style_container = nullptr;
  for (Node* container = start_node; container && start_node == end_node;
       container = container->firstChild()) {
    if (auto* font = ToHTMLFontElementOrNull(container))
      font_container = font;
    bool style_container_is_not_span = !IsHTMLSpanElement(style_container);
    if (container->IsHTMLElement()) {
      HTMLElement* container_element = ToHTMLElement(container);
      if (IsHTMLSpanElement(*container_element) ||
          (style_container_is_not_span && container_element->HasChildren()))
        style_container = ToHTMLElement(container);
    }
    if (!container->hasChildren())
      break;
    start_node = container->firstChild();
    end_node = container->lastChild();
  }

  // Font tags need to go outside of CSS so that CSS font sizes override leagcy
  // font sizes.
  if (style_change.ApplyFontColor() || style_change.ApplyFontFace() ||
      style_change.ApplyFontSize()) {
    if (font_container) {
      if (style_change.ApplyFontColor())
        SetNodeAttribute(font_container, colorAttr,
                         AtomicString(style_change.FontColor()));
      if (style_change.ApplyFontFace())
        SetNodeAttribute(font_container, faceAttr,
                         AtomicString(style_change.FontFace()));
      if (style_change.ApplyFontSize())
        SetNodeAttribute(font_container, sizeAttr,
                         AtomicString(style_change.FontSize()));
    } else {
      HTMLFontElement* font_element = HTMLFontElement::Create(GetDocument());
      if (style_change.ApplyFontColor())
        font_element->setAttribute(colorAttr,
                                   AtomicString(style_change.FontColor()));
      if (style_change.ApplyFontFace())
        font_element->setAttribute(faceAttr,
                                   AtomicString(style_change.FontFace()));
      if (style_change.ApplyFontSize())
        font_element->setAttribute(sizeAttr,
                                   AtomicString(style_change.FontSize()));
      SurroundNodeRangeWithElement(start_node, end_node, font_element,
                                   editing_state);
      if (editing_state->IsAborted())
        return;
    }
  }

  if (style_change.CssStyle().length()) {
    if (style_container) {
      if (const StylePropertySet* existing_style =
              style_container->InlineStyle()) {
        String existing_text = existing_style->AsText();
        StringBuilder css_text;
        css_text.Append(existing_text);
        if (!existing_text.IsEmpty())
          css_text.Append(' ');
        css_text.Append(style_change.CssStyle());
        SetNodeAttribute(style_container, styleAttr, css_text.ToAtomicString());
      } else {
        SetNodeAttribute(style_container, styleAttr,
                         AtomicString(style_change.CssStyle()));
      }
    } else {
      HTMLSpanElement* style_element = HTMLSpanElement::Create(GetDocument());
      style_element->setAttribute(styleAttr,
                                  AtomicString(style_change.CssStyle()));
      SurroundNodeRangeWithElement(start_node, end_node, style_element,
                                   editing_state);
      if (editing_state->IsAborted())
        return;
    }
  }

  if (style_change.ApplyBold()) {
    SurroundNodeRangeWithElement(start_node, end_node,
                                 HTMLElement::Create(bTag, GetDocument()),
                                 editing_state);
    if (editing_state->IsAborted())
      return;
  }

  if (style_change.ApplyItalic()) {
    SurroundNodeRangeWithElement(start_node, end_node,
                                 HTMLElement::Create(iTag, GetDocument()),
                                 editing_state);
    if (editing_state->IsAborted())
      return;
  }

  if (style_change.ApplyUnderline()) {
    SurroundNodeRangeWithElement(start_node, end_node,
                                 HTMLElement::Create(uTag, GetDocument()),
                                 editing_state);
    if (editing_state->IsAborted())
      return;
  }

  if (style_change.ApplyLineThrough()) {
    SurroundNodeRangeWithElement(start_node, end_node,
                                 HTMLElement::Create(strikeTag, GetDocument()),
                                 editing_state);
    if (editing_state->IsAborted())
      return;
  }

  if (style_change.ApplySubscript()) {
    SurroundNodeRangeWithElement(start_node, end_node,
                                 HTMLElement::Create(subTag, GetDocument()),
                                 editing_state);
    if (editing_state->IsAborted())
      return;
  } else if (style_change.ApplySuperscript()) {
    SurroundNodeRangeWithElement(start_node, end_node,
                                 HTMLElement::Create(supTag, GetDocument()),
                                 editing_state);
    if (editing_state->IsAborted())
      return;
  }

  if (styled_inline_element_ && add_styled_element == kAddStyledElement)
    SurroundNodeRangeWithElement(
        start_node, end_node,
        styled_inline_element_->CloneElementWithoutChildren(), editing_state);
}

float ApplyStyleCommand::ComputedFontSize(Node* node) {
  if (!node)
    return 0;

  CSSComputedStyleDeclaration* style =
      CSSComputedStyleDeclaration::Create(node);
  if (!style)
    return 0;

  const CSSPrimitiveValue* value =
      ToCSSPrimitiveValue(style->GetPropertyCSSValue(CSSPropertyFontSize));
  if (!value)
    return 0;

  // TODO(yosin): We should have printer for |CSSPrimitiveValue::UnitType|.
  DCHECK(value->TypeWithCalcResolved() == CSSPrimitiveValue::UnitType::kPixels);
  return value->GetFloatValue();
}

void ApplyStyleCommand::JoinChildTextNodes(ContainerNode* node,
                                           const Position& start,
                                           const Position& end) {
  if (!node)
    return;

  Position new_start = start;
  Position new_end = end;

  HeapVector<Member<Text>> text_nodes;
  for (Node* curr = node->firstChild(); curr; curr = curr->nextSibling()) {
    if (!curr->IsTextNode())
      continue;

    text_nodes.push_back(ToText(curr));
  }

  for (const auto& text_node : text_nodes) {
    Text* child_text = text_node;
    Node* next = child_text->nextSibling();
    if (!next || !next->IsTextNode())
      continue;

    Text* next_text = ToText(next);
    if (start.IsOffsetInAnchor() && next == start.ComputeContainerNode())
      new_start = Position(
          child_text, child_text->length() + start.OffsetInContainerNode());
    if (end.IsOffsetInAnchor() && next == end.ComputeContainerNode())
      new_end = Position(child_text,
                         child_text->length() + end.OffsetInContainerNode());
    String text_to_move = next_text->data();
    InsertTextIntoNode(child_text, child_text->length(), text_to_move);
    // Removing a Text node doesn't dispatch synchronous events.
    RemoveNode(next, ASSERT_NO_EDITING_ABORT);
    // don't move child node pointer. it may want to merge with more text nodes.
  }

  UpdateStartEnd(new_start, new_end);
}

DEFINE_TRACE(ApplyStyleCommand) {
  visitor->Trace(style_);
  visitor->Trace(start_);
  visitor->Trace(end_);
  visitor->Trace(styled_inline_element_);
  CompositeEditCommand::Trace(visitor);
}

}  // namespace blink
