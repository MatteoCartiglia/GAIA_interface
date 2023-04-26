#ifndef VISUALIZER_H
#define VISUALIZER_H

# define sampling_frequency 1e-5



struct pixel_click{
    int x;
    int y;
};

class Visualizer {
public:
    Visualizer();
    ~Visualizer ();
    void renderEvents(bool* live_visualization_open, bool reset, bool fifo_full);
    void processEvents(std::vector<Event2d>& input_events, uint8_t filter_map);
    pixel_click onMouseClick(int mouse_x, int mouse_y);
    void handleMouseClick();
    bool checkFifoFull(bool fifo_full);

    void enableRecord(bool state);
    void writeEvent(std::ofstream* of, const Event2d& ev);
    void recordingInitializer();
    void recordingFinisher();
    void showSavingPopup(const std::string& filename);


    void filePicker();
    void loadEvents(const std::string& filename);
    void filterEventsByTimestamp();
    void updateEventTimeframe();
    void handlePlayback();
    void handlePrevFrame();
    void handleNextFrame();


    void removeTsOffset();
    void updateGaiaEvents();
    void sortEvents();

    void displaySelectedHotPixels();
    void visualizeSelectedHotPixels();


    void drawHeatmap();
    std::vector<Event2d> eventstoDraw, gaia_events, gaia_loaded_events;

    double fps;
    bool isLive = true;


private:
    void popIncomingEvents();

    float circle_diameter = 10;
    int pixels_X_offset= 150;
    int pixels_Y_offset = 300;
    uint16_t filter_map;

    bool recording_active = false;
    bool coincidence_active = false;
    bool hotpixel_active = false;
    bool AP_filter_Active = false;

    int64_t current_timestamp = 0;

    void drawCanvas(bool* live_visualization_open);

    RecordingFilter* recording_filter;
    bool prev_recording_active = false;
    std::string filename = "tmp.dat";
    std::string loading_filename;

    bool show_rename_popup = false;
    bool show_file_picker = false;

    std::chrono::duration<double> frame_interval;
    std::chrono::high_resolution_clock::time_point prev_frame_time, current_frame_time;
    uint64_t start_timestamp = 0;
    float time_scale = 1.0f;
    int itorator;
    uint64_t start_ts, end_ts;
    int buffer_time;
    bool isPlayback = false;

    bool playback_paused = false;
    bool nextFrame = false;
    bool prevFrame = false;

    HotPixelFilter hotPixel_Filter;
    CoincidenceFilter CoincidenceFilter;
    APFilter APFilter;

    HeatmapFilter heatmap_filter;
    bool show_heatmap_window = true;
    bool heatmap_active=true;

};


#endif

