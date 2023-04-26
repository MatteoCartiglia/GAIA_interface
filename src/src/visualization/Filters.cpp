//
// Created by Matteo Cartiglia on 22.04.23.
//

#include "../include/Filters.h"
#include <opencv2/imgcodecs.hpp>
#include <opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


RecordingFilter::RecordingFilter(const std::string& filename) : default_filename(filename), record(false), ofs(nullptr) {
}

RecordingFilter::~RecordingFilter() {
    if (ofs != nullptr) {
        ofs->close();
        delete ofs;
    }
}
void RecordingFilter::process(std::vector<Event2d>& events) {
    std::cout<< record <<std::endl;

    if (record) {
        for (const auto& event : events) {
            write_event(ofs, event);
        }
    }
}
void RecordingFilter::enable_record() {
    record = !record;

    if (record) {
        ofs = new std::ofstream(default_filename, std::ofstream::out | std::ofstream::binary);
        if (ofs->is_open()) {
            std::cout << "Streaming file is open \n" << std::endl;
        }
    }
    else {
        ofs->close();
        std::cout << "Streaming file is closed \n" << std::endl;

    }
}
void RecordingFilter::write_event(std::ofstream* of, const Event2d& ev) {
    of->write(reinterpret_cast<const char*>(&ev), sizeof(ev));
}





HotPixelFilter::HotPixelFilter(){};
HotPixelFilter::~HotPixelFilter(){};

void HotPixelFilter::process(std::vector<Event2d>& events) {

    for (auto it = events.begin(); it != events.end();) {
        std::pair<int, int> current_pair(it->x, it->y);
        if (selected_hot_pixels.find(current_pair) != selected_hot_pixels.end()) {
            it = events.erase(it);
        } else {
            ++it;
        }
    }
}

// HotPixelFilter.cpp
const std::unordered_set<std::pair<int, int>, pair_hash>& HotPixelFilter::getSelectedHotPixels() const {
    return selected_hot_pixels;
}

void HotPixelFilter::checkHotpixel(int x, int y) {
    std::pair<int, int> coord = std::make_pair(x,y);

    if (selected_hot_pixels.find(coord) != selected_hot_pixels.end()) {
        selected_hot_pixels.erase(coord);
    } else {
        selected_hot_pixels.insert(coord);
    }
}


float BaseTimingFilter::distanceCalculator(Event2d p1,Event2d p2 )
{   //calculating Euclidean distance
    float distance=0;
    float p = abs(int(p1.x - p2.x));
    float l = abs(int(p1.y - p2.y));
    distance = pow(p, 2.) + pow(l, 2.);
    distance = sqrt(distance);
    return distance;
}


CoincidenceFilter::CoincidenceFilter() : BaseTimingFilter() {}

CoincidenceFilter::~CoincidenceFilter(){};




void CoincidenceFilter::processing(std::vector<Event2d>& events) {
    float dist;
    float time;
    filteredEvents.clear();
    std::set<int> addedIndices;

    for (int i = 0; i < events.size() - 1; i++) {
        if (addedIndices.find(i) != addedIndices.end()) {
            continue;
        }
        for (int j = i + 1; j < events.size(); j++) {
            if (addedIndices.find(j) != addedIndices.end()) {
                continue;
            }
            time = ((events[j].ts - events[i].ts)) * sampling_frequency;
            if (time >= time_cutoff *1e-3) {
                break;
            }
            if (time < time_cutoff*1e-3) {
                dist = distanceCalculator(events[i], events[j]);

                if (dist <= distance_cutoff ) {
                    filteredEvents.push_back(events[i]);
                    filteredEvents.push_back(events[j]);
                    addedIndices.insert(i);
                    addedIndices.insert(j);
                    break;
                }
            }
        }
    }
    events.swap(filteredEvents);
}


void CoincidenceFilter::liveFiltering(std::vector<Event2d>& events) {

}

APFilter::APFilter() : BaseTimingFilter() {}
APFilter::~APFilter(){};


void APFilter::processing(std::vector<Event2d>& events) {
    std::cout << " distance_cutoff: " << distance_cutoff << std::endl;
    float dist;
    float time;
    filteredEvents.clear();
    std::set<int> addedIndices;

    for (int i = 0; i < events.size() - 1; i++) {
        if (addedIndices.find(i) != addedIndices.end()) {
            continue;
        }
        for (int j = i + 1; j < events.size(); j++) {
            if (addedIndices.find(j) != addedIndices.end()) {
                continue;
            }
            time = ((events[j].ts - events[i].ts)) * sampling_frequency;
            if (time > time_cutoff *1e-3) {
                break;
            }
            if (time <= time_cutoff*1e-3) {
                std::cout<< "time: " << time <<" "<<time_cutoff<<std::endl;
                std::cout<< "j.ts: " << events[j].ts <<" "<<events[i].ts<<std::endl;

                dist = distanceCalculator(events[i], events[j]);

                if (dist <= distance_cutoff ) {
//   if (events[i].p == 0 && events[j].p == 1)
                {
                        filteredEvents.push_back(events[i]);
                        filteredEvents.push_back(events[j]);
                        addedIndices.insert(i);
                        addedIndices.insert(j);
                    }
                    break;
                }
            }
        }
    }
    events.swap(filteredEvents);
}


HeatmapFilter::HeatmapFilter(int width, int height) : width(width), height(height) {
}


void HeatmapFilter::process(const std::vector<Event2d>& events) {
    event_number = 0;
    event_number_on=0;
    event_number_off=0;
    std::fill(GAIAEvents, GAIAEvents + LEN_GAIAEVENTS, 0); // Re-initialize GAIAEvents to all zeros
    for (int i = 0; i < events.size(); i++) {
        auto it = std::find_if(blacklist.begin(), blacklist.end(), [&events, i](const Event2d &ev) {
            return ev.x == events[i].x && ev.y == events[i].y;
        });
        if (it == blacklist.end()) {
            if (events[i].p == 1 && visualize_on) {
                GAIAEvents[(74 - events[i].y) * 75 + events[i].x] += 1;
                event_number_on++;
                event_number++;

            }
            if (events[i].p == 0 && visualize_off) {
                GAIAEvents[(74 - events[i].y) * 75 + events[i].x] += 1;
                event_number_off++;
                event_number++;

            }
        }
    }
}
void HeatmapFilter::calculateEventCounts(std::vector<Event2d>& events, float timeBinSize) {
    onEventCounts.clear();
    offEventCounts.clear();
    int currentTimeBin = events[0].ts;
    int onCount = 0;
    int offCount = 0;
    // timeBinSize is in 10us
    for (Event2d& event : events) {
        if (event.ts >= (currentTimeBin + timeBinSize)) {
            onEventCounts.push_back(onCount);
            offEventCounts.push_back(offCount);
            onCount = 0;
            offCount = 0;
            currentTimeBin += timeBinSize;
        }

        if (event.p == 1) {
            onCount++;
            //std::cout << onCount << std::endl;

        } else {
            offCount++;
        }
    }

    onEventCounts.push_back(onCount);
    offEventCounts.push_back(offCount);
}
