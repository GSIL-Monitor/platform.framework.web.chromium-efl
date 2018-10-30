// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVTOOLS_PORT_MSG_H_
#define DEVTOOLS_PORT_MSG_H_

class DevToolsPortMsg {
 public:
  DevToolsPortMsg(){};
  bool Send(int pid, int port);
};

#endif  // DEVTOOLS_PORT_MSG_H_
