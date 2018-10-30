// Copyright 2014 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ArticleRecognition_h
#define ArticleRecognition_h

namespace blink {

class LocalFrame;
class WebString;

class ArticleRecognition {
public:
    static bool recognizeArticleSimpleNativeRecognitionMode(LocalFrame*, WebString& pageURL);
    static bool recognizeArticleNativeRecognitionMode(LocalFrame*, WebString& pageURL);
};

} // namespace blink

#endif // ArticleRecognition_h
