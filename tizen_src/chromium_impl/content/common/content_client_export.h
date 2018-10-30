// Copyright 2015 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_CONTENT_CLIENT_EXPORT_H_
#define CONTENT_PUBLIC_COMMON_CONTENT_CLIENT_EXPORT_H_

#include "content/common/content_export.h"

namespace content {

class ContentClient;

CONTENT_EXPORT ContentClient* GetContentClientExport();

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_CONTENT_CLIENT_EXPORT_H_
