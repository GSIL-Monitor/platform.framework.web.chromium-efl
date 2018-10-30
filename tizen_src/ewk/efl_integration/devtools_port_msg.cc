// Copyright 2016 Samsung Electronics. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "devtools_port_msg.h"

#include <arpa/inet.h>
#include <net/if.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

bool DevToolsPortMsg::Send(int pid, int port) {
  int sockfd;
  struct ifreq ifr;
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  char eth[] = "eth0";
  ifr.ifr_addr.sa_family = AF_INET;
  strncpy(ifr.ifr_name, eth, sizeof(eth));

  if (ioctl(sockfd, SIOCGIFADDR, &ifr) < 0) {
    close(sockfd);
    return false;
  }
  struct sockaddr_in servaddr;
  bzero(&servaddr, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_port = htons(port);

  char* addr = inet_ntoa(((struct sockaddr_in*)&(ifr.ifr_addr))->sin_addr);
  if (inet_pton(AF_INET, addr, &servaddr.sin_addr) <= 0) {
    close(sockfd);
    return false;
  }

  if (connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
    return false;
  }
  const int str_len = 128;
  char closeport[str_len];
  memset(closeport, 0, str_len);
  strcat(closeport, "GET /closeport HTTP/1.1\n");
  strcat(closeport, "\r\n");
  int send_size =
      send(sockfd, closeport, strlen(closeport), MSG_WAITALL || MSG_NOSIGNAL);

  if (send_size < 0) {
    close(sockfd);
    return false;
  } else {
    while (1) {
      char read_buf[str_len];
      memset(read_buf, 0, str_len);
      int read_size = read(sockfd, read_buf, str_len);
      if (read_size) {
        continue;
      } else {
        close(sockfd);
        return true;
      }
    }
  }
  close(sockfd);
  return false;
}
