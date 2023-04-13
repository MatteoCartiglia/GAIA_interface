#ifndef __EVENTS_H
#define __EVENTS_H

#include <fstream>
#include <iostream>



class Event2d {
public:
    Event2d();
    Event2d(unsigned long long _ts, unsigned int _x, unsigned int _y, unsigned int _p);
    ~Event2d(){};
    unsigned long long ts;
    unsigned int x,y;
    unsigned int p;
};

void WriteEvent(std::ofstream* of, Event2d ev);

class EventRaw {
public:
    EventRaw();
    EventRaw(unsigned long long _ts, unsigned short _data);
    ~EventRaw(){};
    //void WriteEvent(std::ofstream* of);
    unsigned long long ts;
    unsigned short data;
};

// create buffer of events 
//std::queue<Event2d> ev_buffer; // Events from FPGA
moodycamel::ConcurrentQueue<Event2d> ev_buffer;

// GAIA 4096 Event Matrix variables 
int address_mask = 0x3f; 
int address_shift = 6; 
int pol_mask =  0x1;
int pol_shift =  12;

#endif /* __EVENTS_H */
