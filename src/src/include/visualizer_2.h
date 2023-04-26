#ifndef VISUALIZER_H
#define VISUALIZER_H

# define sampling_frequency 1e-5
# define LEN_GAIAEVENTS 5625

class Visualizer{
    public:

        int slow_down =1;
        bool record;
        double fps;
        float tmp1= 0, tmp2=0;
        std::string default_filename = "Recordings/tmp.dat";
        std::ofstream* ofs;

        std::vector<int> index_time_bin;
        std::vector<Event2d> gaia_events;
        std::vector<Event2d> loaded_gaia_events;
        std::vector<Event2d> loaded_gaia_events_all;
        std::vector<Event2d> single_events_plots;
    Visualizer ();
        ~Visualizer ();

        void ShowGaiaVisualization(bool* live_visualization_open, bool reset);
        void renderEvents(bool* live_visualization_open, bool reset);
        void create_event_vector(std::string path,  int num_ev);
        void write_dummy_data();
        void copy_events(std::string old_path, std::string new_path);
        void populate_index_time_bin(int itorator);
        void WriteEvent(std::ofstream* of, Event2d ev);
        void enable_record(bool state);
        void enable_save_processed_data(bool state);

    float distance_calculator(Event2d p1,Event2d p2 );
        void background_filter(  );
        void hot_pixel_filter(  );

};
// Declare filter activation states and set default values
    static bool hotpixel_active = false;
    static bool coincidence_active = false;

    float distance_cutoff= 2;
    float time_cutoff = 0.001;
    int num_ev = -1;
    int index_time_plot=-1;
    int itorator =0;
    bool playback = false;
    bool live = true;
    bool pause_replay = false;
    bool stop_replay = false;
    bool next_replay = false;
    bool prev_replay = false;
    bool bg_noise = false;
    bool heatmap_plot = false;
    bool heatmap_on_ev = true;
    bool heatmap_off_ev = true;
    bool event_trace_plot = false;
    std::vector<int> X_plot ;
    std::vector<int>  Y_plot;
    std::vector<Event2d> foundElements;
    float tot_time_plot;
    float interval_time = 0.001;
    std::vector<float> plt_ev;
    std::vector<float> plt_time;
    bool save_img = false;
    bool save_processed_data = false;

    bool hot_pixels =false;
    std::vector<Event2d> blacklist;



float new_X_plot, new_Y_plot,rm_X_plot,rm_Y_plot;
    bool add_plot= false, add_plot_all=false;
    bool rm_plot= false, rm_plot_all=false;

// Visualizatin parameters
    float colormap_baseline_color = 0;
    float colormap_on_color_lowbound = 1;
    float colormap_on_color_upbound = 2;
    float colormap_off_color_lowbound = 3;
    float colormap_off_color_upbound = 4;
    float dummycolor=5;


#endif

