// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ArticleRecognition.h"

#include "bindings/core/v8/ScriptRegexp.h"
#include "core/css/CSSStyleDeclaration.h"
#include "core/dom/ElementTraversal.h"
#include "core/dom/NodeComputedStyle.h"
#include "core/dom/TagCollection.h"
#include "core/dom/Text.h"
#include "core/frame/Frame.h"
#include "core/frame/LocalFrame.h"
#include "core/geometry/DOMRect.h"
#include "core/html/HTMLAnchorElement.h"
#include "core/html/HTMLBRElement.h"
#include "core/html/HTMLCollection.h"
#include "core/html/HTMLFormElement.h"
#include "core/html/HTMLParagraphElement.h"
#include "core/style/ComputedStyle.h"
#include "platform/text/Character.h"
#include "platform/wtf/text/StringBuilder.h"
#include "platform/wtf/text/WTFString.h"
#include "public/web/WebScriptSource.h"
#include "web/WebLocalFrameImpl.h"

namespace blink {

using namespace HTMLNames;

static const unsigned cjkSampleSize = 30;
static const unsigned articleHeightThreshold = 200;
//static const unsigned articleTagThreshold = 15;

// Checks whether it is Chinese, Japanese or Korean Page.
static inline bool isCJKPage(const String& searchString)
{
    unsigned length = std::min(searchString.length(), cjkSampleSize);
    for (unsigned i = 0; i < length; ++i) {
        if (Character::IsCJKIdeographOrSymbol(searchString[i]))
            return true;
    }
    return false;
}

// Checks if the article height is greater than the minimum threshold.
static inline bool isArticleHeightSufficient(Element* articleElement)
{
    return articleElement && articleElement->getBoundingClientRect()->height() > articleHeightThreshold;
}

// Takes a search string and a search array and finds whether searchString
// contains any of '|' seperated values present in the searchArray.
static bool regExpSearch(const String& searchString, const char* const* searchArray, unsigned arrayLength)
{
    for (unsigned i = 0; i < arrayLength; ++i) {
        if (searchString.FindIgnoringCase(searchArray[i]) != kNotFound)
            return true;
    }
    return false;
}

static inline void textContent(Node& node, StringBuilder& builder)
{
    if (node.parentNode() && isHTMLScriptElement(*node.parentNode()))
        return;

    if (node.IsTextNode()) {
        builder.Append(ToText(node).data());
        return;
    }

    for (Node* child = node.firstChild(); child; child = child->nextSibling())
        textContent(*child, builder);
}

static String visibleTextContent(Node& node)
{
    StringBuilder builder;
    textContent(node, builder);
    return builder.ToString();
}

// Returns the ratio of all anchor tags inner text length (within element for which
// query is made) and elements inner Text.
static double linkDensityForElement(Element& element)
{
    unsigned textLength = element.innerTextLength();
    unsigned anchorLength = 0;

    if (!textLength)
        return 0;

    for (HTMLAnchorElement* anchor = Traversal<HTMLAnchorElement>::FirstWithin(element); anchor; anchor = Traversal<HTMLAnchorElement>::Next(*anchor, &element))
        anchorLength += anchor->innerTextLength();
    return static_cast<double>(anchorLength) / textLength;
}

static int scoreForTag(const QualifiedName& tag)
{
    if (tag.Matches(divTag))
        return 5;
    if (tag.Matches(articleTag))
        return 25;
    if (tag.Matches(preTag) || tag.Matches(tdTag) || tag.Matches(blockquoteTag))
        return 3;
    if (tag.Matches(addressTag) || tag.Matches(ulTag) || tag.Matches(dlTag) || tag.Matches(ddTag)
        || tag.Matches(dtTag) || tag.Matches(liTag) || tag.Matches(formTag)) {
        return -3;
    }
    if (tag.Matches(h1Tag) || tag.Matches(h2Tag) || tag.Matches(h3Tag) || tag.Matches(h4Tag)
        || tag.Matches(h5Tag) || tag.Matches(h6Tag) || tag.Matches(thTag)) {
        return -5;
    }
    return 0;
}

static int classWeightForElement(const Element& element)
{
    DEFINE_STATIC_LOCAL(const String, regexPositiveString, ("article|body|content|entry|hentry|main|page|pagination|post|text|blog|story|windowclassic"));
    DEFINE_STATIC_LOCAL(const String, regexNegativeString, ("contents|combx|comment|com-|contact|foot|footer|footnote|masthead|media|meta|outbrain|promo|related|scroll|shoutbox|sidebar|date|sponsor|shopping|tags|script|tool|widget|scbox|rail|reply|div_dispalyslide|galleryad|disqus_thread|cnn_strylftcntnt|topRightNarrow|fs-stylelist-thumbnails|replText|ttalk_layer|disqus_post_message|disqus_post_title|cnn_strycntntrgt|wpadvert|sharedaddy sd-like-enabled sd-sharing-enabled|fs-slideshow-wrapper|fs-stylelist-launch|fs-stylelist-next|fs-thumbnail-194230|reply_box|textClass errorContent|mainHeadlineBrief|mainSlideDetails|curvedContent|photo|home_|XMOD"));
    DEFINE_STATIC_LOCAL(const ScriptRegexp, positiveRegex, (regexPositiveString, kTextCaseASCIIInsensitive));
    DEFINE_STATIC_LOCAL(const ScriptRegexp, negativeRegex, (regexNegativeString, kTextCaseASCIIInsensitive));

    int weight = 0;
    const AtomicString& classAttribute = element.GetClassAttribute();
    if (!classAttribute.IsNull()) {
        if (positiveRegex.Match(classAttribute) != -1)
            weight += 30;

        if (negativeRegex.Match(classAttribute) != -1)
            weight -= 25;
    }

    const AtomicString& idAttribute = element.GetIdAttribute();
    if (!idAttribute.IsNull()) {
        if (positiveRegex.Match(idAttribute) != -1)
            weight += 30;

        if (negativeRegex.Match(idAttribute) != -1)
            weight -= 25;
    }
    return weight;
}

static void initializeReadabilityAttributeForElement(Element& element)
{
    // FIXME: Use custom data-* attribute everywhere since readability is not
    // a standard HTML attribute.
    if (int tagScore = scoreForTag(element.TagQName()))
        element.SetFloatingPointAttribute(readabilityAttr, tagScore + classWeightForElement(element));
}

static bool isFormPage(LocalFrame& frame)
{

    Element* bodyElement = frame.GetDocument()->body();
    if (!bodyElement)
        return false;

    double formTotalHeight = 0;

    for (HTMLFormElement* form = Traversal<HTMLFormElement>::FirstWithin(*bodyElement); form; form = Traversal<HTMLFormElement>::Next(*form, bodyElement))
        formTotalHeight += form->getBoundingClientRect()->height();

    return formTotalHeight > bodyElement->getBoundingClientRect()->height() * 0.5;
}

// Utility Function: Calculates max count for BR tag and other Tags and points maxBRContainingElement to element
// containing maximum BR count.
static void calculateBRTagAndOtherTagMaxCount(Element& bodyElement, unsigned& brTagMaxCount, unsigned& otherTagMaxCount, unsigned& totalNumberOfBRTags, Element*& maxBRContainingElement)
{
    for (HTMLBRElement* currentElement = Traversal<HTMLBRElement>::FirstWithin(bodyElement); currentElement; currentElement = Traversal<HTMLBRElement>::Next(*currentElement, &bodyElement)) {
        unsigned brTagCount = 0;
        unsigned otherTagCount = 0;
        ++totalNumberOfBRTags;
        unsigned nextCount = 0;
        for (Element* sibling = ElementTraversal::NextSibling(*currentElement); sibling && nextCount < 5; sibling = ElementTraversal::NextSibling(*sibling)) {
            ++nextCount;
            if (isHTMLBRElement(*sibling)) {
                ++brTagCount;
                nextCount = 0;
            } else if (isHTMLAnchorElement(*sibling) || sibling->HasTagName(bTag)) {
                ++otherTagCount;
            }
        }

        if (brTagCount > brTagMaxCount) {
            if (isArticleHeightSufficient(currentElement->parentElement())) {
                brTagMaxCount = brTagCount;
                otherTagMaxCount = otherTagCount;
                maxBRContainingElement = currentElement;
            }
        }
    }
}

// Utility Function: Calculates max count for P tag and points maxPContainingElement to element
// containing maximum P count.
static void calculatePTagMaxCount(Element& bodyElement, unsigned& pTagMaxCount, unsigned& totalNumberOfPTags, Element*& maxPContainingElement)
{
    for (HTMLParagraphElement* currentElement = Traversal<HTMLParagraphElement>::FirstWithin(bodyElement); currentElement; currentElement = Traversal<HTMLParagraphElement>::Next(*currentElement, &bodyElement)) {
        ++totalNumberOfPTags;
        unsigned pTagCount = 0;
        for (HTMLParagraphElement* sibling = Traversal<HTMLParagraphElement>::NextSibling(*currentElement); sibling; sibling = Traversal<HTMLParagraphElement>::NextSibling(*sibling))
            ++pTagCount;

        if (pTagCount > pTagMaxCount) {
            if (isArticleHeightSufficient(currentElement->parentElement())) {
                pTagMaxCount = pTagCount;
                maxPContainingElement = currentElement;
            }
        }
    }
}

static unsigned countNumberOfSpaceSeparatedValues(const String& string)
{
    unsigned numberOfSpaceSeparatedValues = 0;
    unsigned lastSpaceFoundPosition = 0;
    unsigned length = string.length();
    for (unsigned i = 0; i < length; ++i) {
        if (string[i] == ' ') {
            if (!i || string[i-1] == ' ')
                continue;
            lastSpaceFoundPosition = i;
            ++numberOfSpaceSeparatedValues;
        }
    }
    // Case like "a b c d" -> Three spaces but no of values = 4.
    if (lastSpaceFoundPosition < length - 1)
        ++numberOfSpaceSeparatedValues;
    return numberOfSpaceSeparatedValues;
}

// Utility Function: To populate the Vector with probable nodes likely to
// score more as compared to other Nodes.
static void populateScoringNodesVector(const Element& bodyElement, HeapVector<Member<Node>>& scoringNodes)
{
    static const char* divToPElements[] = {
        "<a>", "<blockquote>", "<dl>", "<div>", "<img", "<ol>", "<p>", "<pre>", "<table>", "<ul>", "<script>", "<article>", "<form>",
        "</a>", "</blockquote>", "</dl>", "</div>", "</ol>", "</p>", "</pre>", "</table>", "</ul>", "</script>", "</article>", "</form>",
    };
    DEFINE_STATIC_LOCAL(const String, regexStringForUnlikelyCandidate, ("combx|comment|community|disqus|extra|foot|header|menu|remark|rss|shoutbox|sidebar|sponsor|ad-break|agegate|pagination|pager|popup|tweet|twitter"));
    DEFINE_STATIC_LOCAL(const String, regexStringForALikelyCandidate, ("and|article|body|column|main|shadow"));
    DEFINE_STATIC_LOCAL(const ScriptRegexp, unlikelyRegex, (regexStringForUnlikelyCandidate, kTextCaseASCIIInsensitive));
    DEFINE_STATIC_LOCAL(const ScriptRegexp, maybeRegex, (regexStringForALikelyCandidate, kTextCaseASCIIInsensitive));

    for (Element* currentElement = ElementTraversal::FirstWithin(bodyElement); currentElement; currentElement = ElementTraversal::Next(*currentElement, &bodyElement)) {
        const AtomicString& classAttribute = currentElement->GetClassAttribute();
        const AtomicString& id = currentElement->GetIdAttribute();

        bool isUnlikelyToBeACandidate = (unlikelyRegex.Match(classAttribute) != -1 || unlikelyRegex.Match(id) != -1);
        bool isLikelyToBeACandidate = (maybeRegex.Match(classAttribute) == -1 || maybeRegex.Match(id) == -1);

        if (isUnlikelyToBeACandidate && !isLikelyToBeACandidate && !isHTMLBodyElement(*currentElement))
            continue;

        DOMRect* currentElementRect = currentElement->getBoundingClientRect();
        if (!currentElementRect->height() && !currentElementRect->width())
            continue;

        if (isHTMLParagraphElement(*currentElement) || isHTMLUListElement(*currentElement)
            || (currentElement->HasTagName(tdTag) && currentElement->getElementsByTagName(tableTag.LocalName())->IsEmpty())
            || currentElement->HasTagName(preTag)) {
            scoringNodes.push_back(currentElement);
        } else if (isHTMLDivElement(*currentElement)) {
            String elementInnerHTML = ToHTMLElement(currentElement)->innerHTML();
            if (currentElement->parentElement() && !regExpSearch(elementInnerHTML, divToPElements, WTF_ARRAY_LENGTH(divToPElements))) {
                Element* parentElement = currentElement->parentElement();

                const AtomicString& parentClassAttribute = parentElement->GetClassAttribute();
                const AtomicString& parentId = parentElement->GetIdAttribute();

                bool isUnlikelyParentCandidate = (unlikelyRegex.Match(parentClassAttribute) != -1 || unlikelyRegex.Match(parentId) != -1);
                bool isLikelyParentCandidate = (maybeRegex.Match(parentClassAttribute) == -1 || maybeRegex.Match(parentId) == -1);

                if (isUnlikelyParentCandidate && !isLikelyParentCandidate && !isHTMLBodyElement(*currentElement))
                    continue;
                scoringNodes.push_back(currentElement);
            } else {
                for (Node* child = currentElement->firstChild(); child; child = child->nextSibling()) {
                    if (child->IsTextNode())
                        scoringNodes.push_back(child);
                }
            }
        }
    }
}

static void populateCandidateElementsVector(const HeapVector<Member<Node>>& scoringNodes, HeapVector<Member<Element>>& candidateElements, bool& isCJK)
{
    unsigned scoringNodesSize = scoringNodes.size();
    for (unsigned i = 0; i < scoringNodesSize; ++i) {
        String scoringNodeVisibleTextContent;
        if (scoringNodes[i]->IsElementNode())
            scoringNodeVisibleTextContent = visibleTextContent(*scoringNodes[i]);

        if (scoringNodeVisibleTextContent.length() < 30)
            continue;

        Element* parentElement = scoringNodes[i]->parentElement();
        if (parentElement && !parentElement->FastHasAttribute(readabilityAttr)) {
            initializeReadabilityAttributeForElement(*parentElement);
            candidateElements.push_back(parentElement);
        }

        Element* grandParentElement = parentElement ? parentElement->parentElement() : 0;
        if (grandParentElement && !grandParentElement->FastHasAttribute(readabilityAttr)) {
            initializeReadabilityAttributeForElement(*grandParentElement);
            candidateElements.push_back(parentElement);
        }

        double contentScore = 1 + countNumberOfSpaceSeparatedValues(scoringNodeVisibleTextContent);

        // On detection of CJK character, contentscore is incremented further.
        isCJK = isCJKPage(scoringNodeVisibleTextContent);

        unsigned textLength = scoringNodeVisibleTextContent.length();
        if (isCJK) {
            contentScore += std::min(floor(textLength / 100.), 3.);
            contentScore *= 3;
        } else {
            if (textLength < 25)
                continue;
            contentScore += std::min(floor(textLength / 100.), 3.);
        }

        if (parentElement) {
            double parentElementScore = contentScore + parentElement->GetFloatingPointAttribute(readabilityAttr, 0);
            parentElement->SetFloatingPointAttribute(readabilityAttr, parentElementScore);
            if (grandParentElement) {
                double grandParentElementScore = grandParentElement->GetFloatingPointAttribute(readabilityAttr, 0) + contentScore / 2;
                grandParentElement->SetFloatingPointAttribute(readabilityAttr, grandParentElementScore);
            }
        }
    }
}

bool ArticleRecognition::recognizeArticleSimpleNativeRecognitionMode(LocalFrame* frame, WebString& pageURL)
{
    DCHECK(frame);
    static const char* homepage[] = {
        "?mview=desktop", "?ref=smartphone", "apple.com", "query=", "search?", "?from=mobile", "signup", "twitter", "facebook", "youtube", "?f=mnate", "linkedin",
        "romaeo", "chrome:", "gsshop", "gdive", "?nytmobile=0", "?CMP=mobile_site", "?main=true", "home-page", "anonymMain", "index.asp", "?s=&b.x=", "eenadu.net",
        "search.cgi?kwd=opposite", "Main_Page", "index.do", "Portal:Main"
    };

    Element* bodyElement = frame ? frame->GetDocument()->body() : 0;

    if (!bodyElement)
        return false;

    const KURL& url = frame->GetDocument()->baseURI();
    pageURL = url.GetString();
    String hostName = url.Host();

    if (url.GetPath() == "/") {
        return false;
    }

    // If any of the '|' separated values in homepage are found in url, return false.
    if (regExpSearch(url, homepage, WTF_ARRAY_LENGTH(homepage))) {
        return false;
    }

    // FIXME: Commented during dev/m60_3112 rebaseline
    // It needs to a relook by author.
    /* unsigned articleTagCount = 0;
    TagCollection* articleTagCollection = bodyElement->getElementsByTagName("article");
    if (articleTagCollection)
        articleTagCount = articleTagCollection->length();

    if (articleTagCount >= articleTagThreshold) {
        return false;
    }*/

    if (isFormPage(*frame)) {
        return false;
    }
    unsigned brTagMaxCount = 0;
    unsigned otherTagMaxCount = 0;
    unsigned totalNumberOfBRTags = 0;
    Element* maxBRContainingElement = 0;
    calculateBRTagAndOtherTagMaxCount(*bodyElement, brTagMaxCount, otherTagMaxCount, totalNumberOfBRTags, maxBRContainingElement);

    unsigned pTagMaxCount = 0;
    unsigned totalNumberOfPTags = 0;
    Element* maxPContainingElement = 0;
    calculatePTagMaxCount(*bodyElement, pTagMaxCount, totalNumberOfPTags, maxPContainingElement);

    if (!brTagMaxCount && !pTagMaxCount && bodyElement->getElementsByTagName(preTag.LocalName())->IsEmpty())
        return false;

    unsigned mainBodyTextLength = bodyElement->innerTextLength();
    if (!mainBodyTextLength)
        return false;

    unsigned articleTextLength = 0;
    unsigned articleAnchorTextLength = 0;
    unsigned brTextLength = 0;
    unsigned pTextLength = 0;
    Element* articleElement = 0;

    if (maxBRContainingElement) {
        if (totalNumberOfBRTags > 0 && maxBRContainingElement->parentElement() && brTagMaxCount >= 1)
            brTextLength = maxBRContainingElement->parentElement()->innerTextLength();
    }

    if (maxPContainingElement) {
        if (totalNumberOfPTags > 0 && maxPContainingElement->parentElement() && pTagMaxCount >= 1)
            pTextLength = maxPContainingElement->parentElement()->innerTextLength();
    }

    articleTextLength = std::max(brTextLength, pTextLength);

    if (!articleTextLength)
        return false;

    if (brTextLength >= pTextLength) {
        if (maxBRContainingElement && maxBRContainingElement->parentElement())
            articleElement = maxBRContainingElement->parentElement();
    } else {
        if (maxPContainingElement && maxPContainingElement->parentElement())
            articleElement = maxPContainingElement->parentElement();
    }

    if (articleElement) {
        for (HTMLAnchorElement* anchor = Traversal<HTMLAnchorElement>::FirstWithin(*articleElement); anchor; anchor = Traversal<HTMLAnchorElement>::Next(*anchor, articleElement)) {
            if (anchor->IsFocusable())
                articleAnchorTextLength += anchor->innerText().length();
        }
    }

    // FIXME: It is inefficient to construct the innerText string for the whole
    // bodyElement when we are only interested in the first 30 elements. Method
    // which does the same can be added to Element.
    String cjkTestString = bodyElement->innerText();

    // To check if there is any CJK character.
    bool isCJKpage = isCJKPage(cjkTestString);

    unsigned anchorTextLength = 1;
    for (HTMLAnchorElement* anchor = Traversal<HTMLAnchorElement>::FirstWithin(*bodyElement); anchor; anchor = Traversal<HTMLAnchorElement>::Next(*anchor, bodyElement)) {
        if (anchor->IsFocusable())
            anchorTextLength += anchor->innerTextLength();
    }

    double linkDensity =  static_cast<double>(anchorTextLength) / mainBodyTextLength;

    double articleLinkDensity = static_cast<double>(articleAnchorTextLength) / articleTextLength;

    if (isCJKpage && ((mainBodyTextLength - anchorTextLength) < 300 || articleTextLength < 150 || articleLinkDensity > 0.5)) {
        return false;
    }

    // FIXME: Why is a separate boolean for kroeftel.de needed?
    bool isKroeftel = (hostName.FindIgnoringCase("kroeftel.de") != kNotFound);

    if ((mainBodyTextLength - anchorTextLength) < 500 || (articleTextLength < 200 && !isKroeftel) || articleLinkDensity > 0.5) {
        return false;
    }

    // getElementsByClassName got introduced in html5. Some websites still have their own custom
    // implementation(e.g: IE-8) for the same which may take lots of time. Also, this may expose
    // sbrowser internal codes. So its better to avoid ReaderIcon in this case.
    v8::HandleScope handleScope(v8::Isolate::GetCurrent());
    v8::Handle<v8::Value> value = WebLocalFrameImpl::FromFrame(frame)->ExecuteScriptAndReturnValue(WebScriptSource("document.getElementsByClassName.ToString()"));
    if (!value.IsEmpty() && value->IsString()) {
        v8::String::Utf8Value temp(value->ToString());
        std::string str = std::string(*temp);
        if (str.find("[native code]") == std::string::npos)
            return false;
    }

    // FIXME: Why is a separate boolean for naver.com needed?
    bool isNaverNews = (hostName.FindIgnoringCase("m.news.naver.com") != kNotFound);

    if (((mainBodyTextLength > 4000 && linkDensity < 0.63) || (mainBodyTextLength > 3000 && linkDensity < 0.58)
        || (mainBodyTextLength > 2500 && linkDensity < 0.6) || (linkDensity < 0.4)) && (otherTagMaxCount <= 13)) {
        if ((articleTextLength == 743 && pTagMaxCount == 2) || (articleTextLength == 316 && pTagMaxCount == 1) || articleTextLength < 300) {
            return false;
        }
        return true;
    }
    if ((linkDensity < 0.7 && linkDensity > 0.4) && (brTagMaxCount >= 1 || pTagMaxCount >= 5) && (otherTagMaxCount <= 13))
        return true;
    if (isNaverNews && (linkDensity < 0.78 && linkDensity > 0.4) && (brTagMaxCount >= 5 || pTagMaxCount >= 5) && (otherTagMaxCount <= 13))
        return true;
    return false;
}

bool ArticleRecognition::recognizeArticleNativeRecognitionMode(LocalFrame* frame, WebString& pageURL)
{
    static const char* homepage[] = {
        "?mview=desktop", "?ref=smartphone", "apple.com", "query=", "|search?", "?from=mobile", "signup", "twitter", "facebook", "youtube", "?f=mnate", "linkedin",
        "romaeo", "chrome:", "gsshop", "gdive", "?nytmobile=0", "?CMP=mobile_site", "?main=true", "home-page", "anonymMain", "thetrainline"
    };

    Element* bodyElement = frame ? frame->GetDocument()->body() : 0;

    if (!bodyElement)
        return false;

    const KURL& url = frame->GetDocument()->Url();
    pageURL = url.GetString();

    if (url.GetPath() == "/") {
        return false;
    }

    // If any of the '|' separated values present in String homepage are found in url, return false.
    // Mostly used for sites using Relative URLs.
    if (regExpSearch(url, homepage, WTF_ARRAY_LENGTH(homepage))) {
        return false;
    }

    Element* recogDiv = frame->GetDocument()->createElement(divTag.LocalName(), ASSERT_NO_EXCEPTION);

    recogDiv->setAttribute(idAttr, "recog_div");
    recogDiv->setAttribute(styleAttr, "display:none;");

    HeapVector<Member<Node>> scoringNodes;
    // Populate scoringNodes vector.
    populateScoringNodesVector(*bodyElement, scoringNodes);

    HeapVector<Member<Element>> candidateElements;
    bool isCJK = false;
    // populate candidateElement Vector.
    populateCandidateElementsVector(scoringNodes, candidateElements, isCJK);

    Element* topCandidate = 0;
    unsigned candidateElementsSize = candidateElements.size();
    for (unsigned i = 0; i < candidateElementsSize; ++i) {
        Element* candidateElement = candidateElements[i];
        // FIXME: Use custom data-* attribute everywhere since readability is not a standard HTML attribute.
        double candidateElementScore = candidateElement->GetFloatingPointAttribute(readabilityAttr, 0);

        double topCandidateScore = topCandidate ? topCandidate->GetFloatingPointAttribute(readabilityAttr, 0) : 0;

        candidateElementScore *= (1 - linkDensityForElement(*candidateElement));
        candidateElement->SetFloatingPointAttribute(readabilityAttr, candidateElementScore);
        if (!topCandidate || candidateElementScore > topCandidateScore)
            topCandidate = candidateElement;
    }

    // After we find top candidates, we check how many similar top-candidates were within a 15% range of this
    // top-candidate - this is needed because on homepages, there are several possible topCandidates which differ
    // by a minute amount in score. The check can be within a 10% range, but to be on the safe-side we are using 15%.
    // Usually, for proper article pages, a clear, definitive topCandidate will be present.
    unsigned neighbourCandidates = 0;
    double topCandidateScore = topCandidate ? topCandidate->GetFloatingPointAttribute(readabilityAttr, 0) : 0;
    for (unsigned i = 0; i < candidateElementsSize; ++i) {
        Element* candidateElement = candidateElements[i];
        double candidateElementScore = candidateElement->GetFloatingPointAttribute(readabilityAttr, 0);
        if ((candidateElementScore >= topCandidateScore * 0.85) && (candidateElement != topCandidate))
            ++neighbourCandidates;
    }

    // For now, the check for neighbourCandidates has threshold of 2, it can be modified later as and when required.
    if (neighbourCandidates > 2) {
        // disabling reader icon.
        return false;
    }

    unsigned numberOfTRs = 0;

    if (!topCandidate)
        return false;

    if (isHTMLTableRowElement(*topCandidate) || topCandidate->HasTagName(tbodyTag))
        numberOfTRs = topCandidate->getElementsByTagName(trTag.LocalName())->length();

    if (topCandidate->GetComputedStyle()->Visibility() != EVisibility::kVisible &&
        !neighbourCandidates) {
      // control will come here if there is no other nodes which can be
      // considered as top candidate, & topCandidate is not visible to user.
      return false;
    }
    if (linkDensityForElement(*topCandidate) > 0.5) {
        // disabling reader icon due to higher link density in topCandidate.
        return false;
    }
    if (isHTMLBodyElement(*topCandidate) || isHTMLFormElement(*topCandidate)) {
        // disabling reader icon as invalid topCandidate.
        return false;
    }

    String elementInnerText = topCandidate->innerText();

    int splitLength = countNumberOfSpaceSeparatedValues(elementInnerText);
    unsigned readerTextLength = elementInnerText.length();
    unsigned readerPlength = topCandidate->getElementsByTagName(pTag.LocalName())->length();
    double readerScore = topCandidate->GetFloatingPointAttribute(readabilityAttr, 0);

    // FIXME: Use a meaningful name for these magic numbers instead of using them directly.
    if ((readerScore >= 40 && numberOfTRs < 3 )
        || (readerScore >= 20 && readerScore < 30 && readerTextLength >900 && readerPlength >=2 && numberOfTRs < 3 && !isCJK)
        || (readerScore >= 20 && readerScore < 30 && readerTextLength >1900 && readerPlength >=1 && numberOfTRs < 3 && !isCJK)
        || (readerScore >= 20 && readerScore < 30 && readerTextLength >1900 && numberOfTRs < 3 && !isCJK)
        || (readerScore > 15 && readerScore <=40  && splitLength >=100 && numberOfTRs < 3)
        || (readerScore >= 100 && readerTextLength >2000  && splitLength >=250 && numberOfTRs > 200)) {
        if (readerScore >= 40 && readerTextLength < 100)
            return false;
        return true;
    }
    return false;
}

} // namespace blink
