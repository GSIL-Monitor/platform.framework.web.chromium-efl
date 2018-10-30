// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ewk_object_private_h
#define ewk_object_private_h

#include <platform/wtf/RefCounted.h>

class EwkObject : public RefCounted<EwkObject> {
public:
    virtual ~EwkObject() { }
};

#endif // ewk_object_private_h
