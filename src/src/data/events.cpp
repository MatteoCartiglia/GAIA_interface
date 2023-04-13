#include "../include/events.h"
#include <iostream>
Event2d::Event2d() {
  ts = 0;
  x = 0;
  y = 0;
  p = 0;
}
Event2d::Event2d(unsigned long long _ts, unsigned int _x, unsigned int _y, unsigned int _p) {
    ts = _ts;
    x = _x;
    y = _y;
    p = _p;
}

EventRaw::EventRaw() {
  ts = 0;
  data = 0;
}
EventRaw::EventRaw(unsigned long long _ts, unsigned short _data) {
  ts = _ts;
  data = _data;
}



