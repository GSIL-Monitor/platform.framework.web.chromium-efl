// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>

#define EVIOCGVERSION _IOR('E', 0x01, int)

struct input_event {
  struct timeval time;
  __u16 type;
  __u16 code;
  __s32 value;
};

void writeEvent(int fd, unsigned int type, unsigned int code, int value)
{
  struct input_event event = { 0, };
  event.type = type;
  event.code = code;
  event.value = value;
  write(fd, &event, sizeof(event));
}

void changeTracking(int fd, int id)
{
  writeEvent(fd, 3, 47, id);
}

void createNewTracking(int fd, int id)
{
  changeTracking(fd, id);
  writeEvent(fd, 3, 57, id);
}

void releaseTracking(int fd)
{
  writeEvent(fd, 3, 57, -1);
}

void dummyEvent(int fd)
{
  writeEvent(fd, 0, 0, 0);
}

void touchDown(int fd, int x, int y)
{
  writeEvent(fd, 1, 330, 1);
  writeEvent(fd, 3, 53, x);
  writeEvent(fd, 3, 54, y);
  dummyEvent(fd);
}

void touchMove(int fd, int x, int y)
{
  if (x != -1)
    writeEvent(fd, 3, 53, x);

  if (y != -1)
    writeEvent(fd, 3, 54, y);

  dummyEvent(fd);
}

void touchUp(int fd)
{
  writeEvent(fd, 1, 330, 0);
  dummyEvent(fd);
}

void tap(int fd, int x, int y)
{
  createNewTracking(fd, 0);
  touchDown(fd, x, y);
  releaseTracking(fd);
  touchUp(fd);
}

void doubleTap(int fd, int x, int y)
{
  tap(fd, x, y);
  usleep(150000);
  tap(fd, x, y);
}

void scroll(int fd, int x, int y, int dir, int length)
{
  createNewTracking(fd, 0);

  y -= length * dir;
  touchDown(fd, x, y);

  int i;
  int length2 = length * 2;
  for (i = 0; i < length2; i += length2 / 100) {
    touchMove(fd, -1, y + i * dir);
    usleep(2000);
  }

  releaseTracking(fd);
  touchUp(fd);
}

void pinchZoom(int fd, int x, int y, int dir, int length)
{
  int x1 = x;
  int y1 = y;
  int x2 = x;
  int y2 = y;

  if (dir == -1) {
    x1 -= length;
    y1 -= length;
    x2 += length;
    y2 += length;
  }

  createNewTracking(fd, 0);
  touchDown(fd, x1, y1);
  createNewTracking(fd, 1);
  touchDown(fd, x2, y2);

  int i;
  int delta;
  for (i = 0; i < length; i += length / 100) {
    delta = i * dir;
    changeTracking(fd, 0);
    touchMove(fd, x1 - delta, y1 - delta);
    changeTracking(fd, 1);
    touchMove(fd, x2 + delta, y2 + delta);
    usleep(8000);
  }

  releaseTracking(fd);
  changeTracking(fd, 0);
  releaseTracking(fd);
}

int main(int argc, char* argv[])
{
  if (argc < 5) {
    fprintf(stdout, "Usage: %s <device> <tap|doubletap|scrollup|scrolldown|"
        "zoomin|zoomout> <x> <y> [length=200]\n\n", argv[0]);
    return 1;
  }

  int fd = open(argv[1], O_RDWR);
  if (fd < 0) {
    fprintf(stderr, "could not open %s, %s\n", argv[optind], strerror(errno));
    return 1;
  }

  int version;
  if (ioctl(fd, EVIOCGVERSION, &version)) {
    close(fd);
    fprintf(stderr, "could not get driver version for %s, %s\n",
        argv[optind], strerror(errno));
    return 1;
  }

  int x = atoi(argv[3]);
  int y = atoi(argv[4]);
  int length = argc >= 6 ? atoi(argv[5]) : 200;

  printf("%d \n", length);

  char* command = argv[2];
  if (!strcmp(command, "tap")) {
    tap(fd, x, y);
  } else if (!strcmp(command, "doubletap")) {
    doubleTap(fd, x, y);
  } else if (!strcmp(command, "scrollup")) {
    scroll(fd, x, y, 1, length);
  } else if (!strcmp(command, "scrolldown")) {
    scroll(fd, x, y, -1, length);
  } else if (!strcmp(command, "zoomin")) {
    pinchZoom(fd, x, y, 1, length);
  } else if (!strcmp(command, "zoomout")) {
    pinchZoom(fd, x, y, -1, length);
  } else {
    fprintf(stdout, "Usage: %s <device> <tap|doubletap|scrollup|scrolldown|"
        "zoomin|zoomout> <x> <y> [length=200]>\n\n", argv[0]);
  }

  close(fd);

  return 0;
}
