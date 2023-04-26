//
// Created by Matteo Cartiglia on 22.04.23.
//

#ifndef FILTER_H
#define FILTER_H

#pragma once
# define LEN_GAIAEVENTS 75*75

#include <unordered_set>
#include "../include/events.h"
//#include "../include/Visualizer.h"

//#include "../include/Visualizer_fwd.h"
enum FilterType {
    Hotpixel,
    Coincidence,
    Recording,
    NUM_FILTERS // Keep this at the end
};

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator()(const std::pair<T1, T2>& p) const {
        auto h1 = std::hash<T1>{}(p.first);
        auto h2 = std::hash<T2>{}(p.second);
        return h1 ^ h2;
    }
};

class Filter {
public:
    virtual ~Filter() {}
    virtual void process(std::vector <Event2d> &events) {};
};

class RecordingFilter : public Filter {
public:
    RecordingFilter(const std::string& filename);
    ~RecordingFilter();

    void process(std::vector<Event2d>& events) override;
    void enable_record();

private:
    void write_event(std::ofstream* of, const Event2d& ev);

    std::ofstream* ofs;
    std::string default_filename;
    bool record=false;

};

class HotPixelFilter : public Filter {
public:
    HotPixelFilter();
    ~HotPixelFilter();
    void process(std::vector<Event2d>& events) override;
    void checkHotpixel(int x, int y);
    void plotHotPixels();
    const std::unordered_set<std::pair<int, int>, pair_hash>& getSelectedHotPixels() const;

private:

    std::unordered_set<std::pair<int, int>,pair_hash> selected_hot_pixels;

};
class BaseTimingFilter {
public:
    BaseTimingFilter() {}
    virtual ~BaseTimingFilter() {}

    // Shared variables
    float distance_cutoff = 1.5f;
    float time_cutoff = 1;
    float distanceCalculator(Event2d p1, Event2d p2);

protected:
    // You can also add shared functions here, if needed
};

class CoincidenceFilter : public BaseTimingFilter {
public:
    CoincidenceFilter();
    ~CoincidenceFilter();

    void processing(std::vector<Event2d>& ev);
    void liveFiltering(std::vector<Event2d>& ev);
    std::vector<Event2d> filteredEvents;

private:
};

class APFilter : public BaseTimingFilter {
public:
    APFilter();
    ~APFilter();
    void processing(std::vector<Event2d>& ev);
    std::vector<Event2d> filteredEvents;

private:
};



class HeatmapFilter {
        public:
        HeatmapFilter(int width, int height);
        void process(const std::vector<Event2d>& events);
        void visualize() const;
        //void getVector(const std::vector<Event2d>& events);
        int GAIAEvents[LEN_GAIAEVENTS] = {0};
        std::vector<int> onEventCounts;
        std::vector<int> offEventCounts;

        int event_number_on = 0;
        int event_number_off = 0;
        void calculateEventCounts( std::vector<Event2d>& events, float timeBinSize);

        bool visualize_on =true;
        bool visualize_off =true;
        std::vector<Event2d> blacklist;

    int event_number=0;
        int width=75;
        int height=75;
        //std::vector<Event2d>& ev;
        private:

};



#endif