#include "../../imgui/imgui_backend/imgui_impl_glfw.h"
#include "../../imgui/imgui_backend/imgui_impl_opengl3.h"
#include <stdio.h>
#include <stack>         
#include <iostream>
#include <string>
#include <fstream> 
#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <string.h>  /* String function definitions */
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sstream>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <list>
#include <random>
#include "implot.h"
#include <vector>
#include <algorithm>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <experimental/filesystem>
#include <cstdlib>

#include <opencv2/imgcodecs.hpp>

#include "../include/bias_control.h"
#include "../include/events.h"
#include "../include/visualizer.h"
#include "../include/dev.h"

using namespace std;


long long first_ts;
extern const int begin_time;

Visualizer::Visualizer(){}
Visualizer::~Visualizer(){}

void Visualizer::write_dummy_data()
{
    std::ofstream* of;

    of = new std::ofstream( "dummy.dat", std::ofstream::out | std::ofstream::binary);
    if (of->is_open())
    {
        // two of the last one
        Event2d evs = Event2d(100000,1,1,1);
        WriteEvent(of, evs);

        Event2d ev = Event2d(100000,2,2,1);
        WriteEvent(of, ev);

        evs = Event2d(200000,1,1,1);
        WriteEvent(of, evs);

        ev = Event2d(200000,2,2,1);
        WriteEvent(of, ev);

        evs = Event2d(300000,1,1,1);
        WriteEvent(of, evs);

        std::cout << "ev len: "<< sizeof (evs) << std::endl;
        of->close();
    }
}

void Visualizer::enable_record(bool state) {
    if(state == true) 
    {
        ofs = new std::ofstream(default_filename, std::ofstream::out | std::ofstream::binary);
        if (ofs->is_open())
        {
          std::cout << "Streaming file is open \n" << std::endl;
        }
    }
    else
    {
        ofs->close();
        std::cout << "Streaming file is closed \n" << std::endl;
    }
    record = state;
}

void Visualizer::WriteEvent(std::ofstream* of, Event2d ev)
{
    of->write( reinterpret_cast<const char*>(&ev), sizeof(ev) ) ;
    //std::chrono::time_point<std::chrono::high_resolution_clock>  time_before_read = std::chrono::high_resolution_clock::now();

    //of->write( reinterpret_cast<const char*>(&(int)time_before_read), sizeof(int) ) ;

}

void Visualizer::copy_events(string old_path, string new_path )
{
    std::ifstream old_file;
    old_file.open ( old_path, std::ios::in | std::ios::binary);
    std::ofstream* new_file  = new std::ofstream(new_path, std::ofstream::out | std::ofstream::binary);

    if (old_file.is_open() && new_file->is_open())  
    {
        Event2d ev; 
        while (old_file.good())
        {
            old_file.read((char *)&ev, sizeof(ev));
            WriteEvent(new_file, ev);
        }
        cout << "Copied events "<< endl;
        old_file.close();    
        new_file->close();  
    }
}

void Visualizer::create_event_vector(string path, int num_ev)
{
    std::cout << "Loading events  \n"<< std::endl;

    loaded_gaia_events.clear();
    loaded_gaia_events_all.clear();
    std::ifstream listStream1;
    listStream1.open ( path, std::ios::in | std::ios::binary);

    int loaded_ev =0;
    if (listStream1.is_open())  {
        std::cout << "Opened:  \n"<< std::endl;
        Event2d ev; 
        while (listStream1.good())
        {
            listStream1.read((char *)&ev, sizeof(ev));
            //std::cout << "p: "<< ev.p<< " x: "<< ev.x<< " y: "<< ev.y<< " ts: "<<  ev.ts << "\n"<< std::endl;
            if (bg_noise){
                loaded_gaia_events_all.push_back(ev);
            }
            else {   loaded_gaia_events.push_back(ev);}

            loaded_ev++;
            if (loaded_ev > num_ev & num_ev != -1)
            {
                break;
            }
        }
        //cout << "Loaded events: " << loaded_gaia_events.size() << endl;
        listStream1.close();      
    }
    itorator = 0;

    if (hot_pixels) {
        hot_pixel_filter();
        cout << "Blacklist pixels: "<<endl;
        for (int i=0;i<blacklist.size();i++)
        {   cout <<  blacklist[i].x << " "<< blacklist[i].y <<endl;}
    }


    if (bg_noise)
    {
        cout << "Size of array before filter : " << loaded_gaia_events_all.size() << endl;
        background_filter();
        cout << "done filtering "<<endl;
    }


    if (loaded_gaia_events.size()>0) {
        cout << "Size of array : " << loaded_gaia_events.size() << endl;
    }
    else{// dummy event so it doesnt crash
        cout << "No events passing the filters! " << endl;
        loaded_gaia_events.push_back(Event2d( 0,0,0,0));
    }
    foundElements.clear();
    if (event_trace_plot){
        foundElements[sizeof(loaded_gaia_events)];
        auto it = std::copy_if(loaded_gaia_events.begin(), loaded_gaia_events.end(), back_inserter(foundElements), [&](const Event2d& ev) {
            return std::find(std::begin(X_plot), std::end(X_plot), ev.x) != std::end(X_plot)
                   && std::find(std::begin(Y_plot), std::end(Y_plot), ev.y) != std::end(Y_plot);
        });
        int size = foundElements.size();
        if (size) {
            std::cout << "Size of array from selected pixels :" << " " << size<< std::endl;
           // std::cout << "Elements found: " << std::endl;
            for (int i = 0; i < size; i++) {
              //  std::cout << "X, Y, Ts: " << foundElements[i].x << " " << foundElements[i].y << " " << foundElements[i].ts <<std::endl;
            }
        } else {
            std::cout << "No Elements found" << std::endl;
        }

        if (size){
            tot_time_plot = (foundElements.back().ts -  foundElements.front().ts)*sampling_frequency; // in sec
            int number_time_bins= ceil(tot_time_plot/interval_time);
            plt_time.clear();
            plt_ev.clear();
            plt_time.shrink_to_fit();
            plt_ev.shrink_to_fit();

            cout << "tot_time_plot: " << tot_time_plot <<endl;
            cout << "number_time_bins: " << number_time_bins <<endl;

            for(int i=0; i<number_time_bins;i++) {
                plt_time.push_back(i * interval_time);
            }
            cout << "len plt_time " << plt_time.size()<< endl;

            plt_ev.resize(plt_time.size());
            std::fill(plt_ev.begin(), plt_ev.end(), 0);

            for (int i=0; i<foundElements.size(); i++)
            {
                int n = floor (( (foundElements[i].ts - foundElements.front().ts)*sampling_frequency )/ ( interval_time));
                plt_ev[n]++;
            }

           // for (int i=0 ; i <plt_ev.size();i++ ){cout <<"ev: " <<plt_ev[i] << "time: "<< plt_time[i] <<endl;}
        }
    }
}
void Visualizer::ShowGaiaVisualization(bool* live_visualization_open, bool reset) {
    ImGui::Begin("GAIA Live Visualization board", live_visualization_open);

    static ImPlotColormap GAIA_Colormaps = -1;
    if (GAIA_Colormaps == -1) {
        std::cout << "Creating colormap" << std::endl;
        //
        ImU32 Gaia_Colormap_Data[6] = {ImColor(255, 255, 255, 255),
                                       ImColor(0, 150, 0, 255),
                                       ImColor(0, 255, 0, 255),
                                       ImColor(150, 0, 0, 255),
                                       ImColor(255, 0, 0, 255),
                                       ImColor(255, 255, 0, 255),

        };

        GAIA_Colormaps = ImPlot::AddColormap("Gaia Colormaps", Gaia_Colormap_Data, 6, false);
    }
    ImPlot::PushColormap("Gaia Colormaps");
    float GAIAEvents[LEN_GAIAEVENTS] = {0};
    /*for (int i = 0; i < LEN_GAIAEVENTS; i++){GAIAEvents[i] = 0;}
    for (int i = 0; i < 10; i++){GAIAEvents[(74-0)*75 +i] = 1;}
    for (int i = 0; i < 20; i++){ GAIAEvents[(74-6)*75 +i] = 2;}
    for (int i = 1; i < 75; i++){GAIAEvents[(74-74)+i] = 3;}
     for (int i = 100; i < 200; i++){GAIAEvents[i] = 1;}
     for (int i = 300; i < 400; i++){GAIAEvents[i] = 1.5;}
     for (int i = 600; i < 700; i++){GAIAEvents[i] = 2;}
     for (int i = 800; i < 900; i++){GAIAEvents[i] = 3;}
     for (int i = 1800; i < 1900; i++){GAIAEvents[i] = 3.5;}
     for (int i = 1900; i < 2000; i++){GAIAEvents[i] = 4;}*/


    // ----- Time handling ---
    static auto start_time = std::chrono::high_resolution_clock::now();
    if (reset == true) {
        start_time = std::chrono::high_resolution_clock::now();
    }
    auto current_time = std::chrono::high_resolution_clock::now();
    double elapsed_time_since_start =
            std::chrono::duration<double, std::milli>(current_time - start_time).count() / 1000;

    // Global Helper variables
    fps = 1 / ImGui::GetIO().DeltaTime;

    // -------- Print interesting facts --- 
    ImGui::BulletText("Current FPS: %f", (fps));
    ImGui::BulletText("Elapsed time since beginning of plot: %i s", int(elapsed_time_since_start));
    //ImGui::BulletText("Event rate: %i ", (gaia_events.size()+loaded_gaia_events.size()));
    //ImGui::BulletText("Fifo usage : %i ", (data_count));
    ImGui::BulletText("Event time: %f ", tmp2);
    ImGui::BulletText("Delta time: %f ", tmp2 - tmp1);

    //ImGui::BulletText("Incoming number of events: %i ", int(event_counter));


    ImGui::InputInt("Number of events to show: ", &num_ev);

    if (ImGui::Button("Load Events"))
        ImGui::OpenPopup("Loading Events");

    if (ImGui::BeginPopupModal("Loading Events", NULL, ImGuiWindowFlags_MenuBar)) {
        ImGui::Text("Choose a your favourite set of events... ");
        std::string path = "Recordings/";
        int n_files = 0;
        for (const auto &dirEntry: fs::directory_iterator(path)) {
            string filename_with_path = dirEntry.path();
            n_files = n_files + 1;
        }
        char *filenames_no_path[n_files];
        char *filenames_with_path[n_files];
        int iter = 0;
        for (const auto &dirEntry: fs::directory_iterator(path)) {
            string filename_with_path = dirEntry.path();
            string filename_no_path = filename_with_path.substr(filename_with_path.find('/') + 1,
                                                                filename_with_path.length() -
                                                                filename_with_path.find('/'));

            filenames_with_path[iter] = new char[filename_with_path.length() + 1];
            strcpy(filenames_with_path[iter], filename_with_path.c_str());

            filenames_no_path[iter] = new char[filename_no_path.length() + 1];
            strcpy(filenames_no_path[iter], filename_no_path.c_str());
            iter++;
        }
        for (int i = 0; i < n_files; i++) {
            if (i % 3 != 0)
                ImGui::SameLine();
            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(4 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(4 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(4 / 7.0f, 0.8f, 0.8f));
            if (ImGui::Button(filenames_no_path[i])) {
                create_event_vector(filenames_with_path[i], num_ev);
                playback = true;
                live = false;
                tmp1, tmp2 = 0;
                ImGui::CloseCurrentPopup();
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }
        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0 / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(0 / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(0 / 7.0f, 0.8f, 0.8f));
        bool close_button = false;
        close_button = ImGui::Button("Close");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (close_button)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.1);
    ImGui::Checkbox("Stop", &stop_replay);
    ImGui::SameLine();
    ImGui::Checkbox("Pause", &pause_replay);
    ImGui::SameLine();
    ImGui::Checkbox("Next", &next_replay);
    ImGui::NewLine();
    ImGui::Checkbox("Background Noise", &bg_noise);
    ImGui::SameLine();
    ImGui::InputFloat("Distance cutoff: ", &distance_cutoff);
    ImGui::SameLine();
    ImGui::InputFloat("Time cutoff: ", &time_cutoff, 0.000001, 0.0001, "%06f");
    ImGui::NewLine();
    ImGui::Checkbox("Plot heatmap", &heatmap_plot);
    ImGui::SameLine();
    ImGui::Checkbox("ON events: ", &heatmap_on_ev);
    ImGui::SameLine();
    ImGui::Checkbox("OFF events: ", &heatmap_off_ev);
    ImGui::SameLine();
    ImGui::Checkbox("Hot pixels: ", &hot_pixels);
    ImGui::NewLine();
    ImGui::Text("Blacklist: ");
    for (int i = 0; i < blacklist.size(); i++) {
        ImGui::SameLine();
        ImGui::Text("%i-%i ", blacklist[i].x, blacklist[i].y);
    }
    ImGui::NewLine();
    ImGui::Checkbox("Event trace: ", &event_trace_plot);
    ImGui::SameLine();
    ImGui::SliderInt("Slow Down", &slow_down, 1, 20, " %.6f x");
    ImGui::SameLine();
    ImGui::Checkbox("Live", &live);
    ImGui::SameLine();
    ImGui::Checkbox("Playback", &playback);
    ImGui::SameLine();
    ImGui::Checkbox("Recording", &record);

    ImGui::NewLine();

    if (ImGui::Button("Save processed data"))
    {
        ofs = new std::ofstream(default_filename, std::ofstream::out | std::ofstream::binary);
        if (ofs->is_open()) {
            std::cout << "Saving \n" << std::endl;
        }
        for (int i = 0; i < loaded_gaia_events.size(); i++) {
            WriteEvent(ofs, loaded_gaia_events[i]);
        }
        cout << "logged all data: " << loaded_gaia_events.size() << endl;
        ofs->close();

        ImGui::OpenPopup("Save Processed Events");
    }

    if (ImGui::BeginPopupModal("Save Processed Events", NULL, ImGuiWindowFlags_MenuBar))
    {
        ImGui::Text("Name the file... ");
        static char filename[128] = "Recordings/1901_CM_electrodestimulation/__NAME__.dat";
        ImGui::InputText(" ", filename, IM_ARRAYSIZE(filename));
        ImGui::SameLine();
        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(3 / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(3 / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(3 / 7.0f, 0.8f, 0.8f));
        bool save_button = false;
        save_button = ImGui::Button("Save");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (save_button) {
            copy_events(default_filename, filename);
        }
        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0 / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(0 / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(0 / 7.0f, 0.8f, 0.8f));
        bool close_button = false;
        close_button = ImGui::Button("Close");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (close_button)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }


    if (ImGui::Button("Start Save")) {
        enable_record(true);
    }
    ImGui::SameLine();

    if (ImGui::Button("Stop Save Events")) {
        enable_record(false);
        ImGui::OpenPopup("Stop Save Events");
    }
    if (ImGui::BeginPopupModal("Stop Save Events", NULL, ImGuiWindowFlags_MenuBar)) {
        ImGui::Text("Name the file... ");
        static char filename[128] = "Recordings/1901_CM_electrodestimulation/__NAME__.dat";
        ImGui::InputText(" ", filename, IM_ARRAYSIZE(filename));
        ImGui::SameLine();
        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(3 / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(3 / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(3 / 7.0f, 0.8f, 0.8f));
        bool save_button = false;
        save_button = ImGui::Button("Save");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (save_button) {
            copy_events(default_filename, filename);
        }
        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4) ImColor::HSV(0 / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4) ImColor::HSV(0 / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4) ImColor::HSV(0 / 7.0f, 0.8f, 0.8f));
        bool close_button = false;
        close_button = ImGui::Button("Close");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (close_button)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }

    if (stop_replay) {
        live = true;
        playback = false;
    }

    if (live == false && playback == true) {
        populate_index_time_bin(itorator);
        if (next_replay) {
            while (index_time_plot == -1) {
                itorator++;
                populate_index_time_bin(itorator);
                next_replay = true;
            }
        }
        next_replay = false;
        if (index_time_plot > 0) {
            for (int i = 0; i < index_time_plot; i++) {
                float color = (loaded_gaia_events[i].ts * sampling_frequency - (tmp1)) / (tmp2 - tmp1);

                if (loaded_gaia_events[i].p == 1) {
                    GAIAEvents[(74 - loaded_gaia_events[i].y) * 75 + loaded_gaia_events[i].x] =
                            colormap_on_color_lowbound + color;
                } else if (loaded_gaia_events[i].p == 0) {
                    GAIAEvents[(74 - loaded_gaia_events[i].y) * 75 + loaded_gaia_events[i].x] =
                            colormap_off_color_lowbound + color;
                } else if (loaded_gaia_events[i].p == 3) {
                    GAIAEvents[(74 - loaded_gaia_events[i].y) * 75 + loaded_gaia_events[i].x] = dummycolor;
                }
            }
        }
        itorator = itorator + 1;
    }
    if(heatmap_plot) {
        live = false;
        playback = false;
        record = false;
        for (int i = 0; i < loaded_gaia_events.size(); i++) {
            if (heatmap_on_ev and loaded_gaia_events[i].p == 1)
            {
                GAIAEvents[(74 - loaded_gaia_events[i].y) * 75 + loaded_gaia_events[i].x] += 1;
            }
            if (heatmap_off_ev and loaded_gaia_events[i].p == 0)
            {
                GAIAEvents[(74 - loaded_gaia_events[i].y) * 75 + loaded_gaia_events[i].x] += 1;
            }
        }
        float max_count=GAIAEvents[0] ;

        for (int i = 0; i < LEN_GAIAEVENTS; i++) {
            if (GAIAEvents[i] > max_count) {
                max_count = GAIAEvents[i];
            }
        }
        for (int i = 0; i < LEN_GAIAEVENTS; i++) {
            GAIAEvents[i] /= max_count;
        }

    }

    if (live == true && playback == false) {
        for (unsigned int i = 0; i < gaia_events.size(); i++) {
            Event2d event = gaia_events[i];
            if (record) {
                WriteEvent(ofs, event);
            }
            if (event.p == 1) {
                GAIAEvents[(74 - event.y) * 75 + event.x] = colormap_on_color_upbound;
            } else if (event.p == 0) {
                GAIAEvents[(74 - event.y) * 75 + event.x] = colormap_off_color_upbound;
            }
        }
        gaia_events.clear();
        gaia_events.shrink_to_fit();
    }

    if (ImGui::Button("Save Plot")) {
        ushort GAIA_heat[75][75][3] = {0};
        for (int i = 0; i < loaded_gaia_events.size(); i++) {
            GAIA_heat[loaded_gaia_events[i].y][loaded_gaia_events[i].x][0] += 1;
            if (loaded_gaia_events[i].p==0)
            {
                GAIA_heat[loaded_gaia_events[i].y][loaded_gaia_events[i].x][1] += 1;
            }
            if (loaded_gaia_events[i].p==1)
            {
                GAIA_heat[loaded_gaia_events[i].y][loaded_gaia_events[i].x][2] += 1;
            }
        }
        cv::Mat mat(75, 75,  CV_16UC3);
        std::memcpy(mat.data, GAIA_heat, 75*75*3*sizeof(ushort));
        cv::imwrite("event_hist/Test.png", mat);
    }

    if (ImPlot::BeginPlot("GAIA 4096", ImVec2(ImGui::GetWindowWidth() * 0.6, ImGui::GetWindowHeight() * 0.6))) {
        ImGui::Text("X vertical - 0 on top; Y horizonal - 0 left");
        ImPlot::PlotHeatmap("GAIA HEATMAP ", GAIAEvents, 75, 75, 0, 5, NULL, ImPlotPoint(0, 0), ImPlotPoint(75, 75));

        if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0)) {
            ImPlotPoint pt = ImPlot::GetPlotMousePos();
            if (event_trace_plot) {
                X_plot.push_back(floor(pt.x));
                Y_plot.push_back(floor(pt.y));
            }
            if (hot_pixels) {
                Event2d ev;

                ev.x = floor(pt.x);
                ev.y = floor(pt.y);
                blacklist.push_back(ev);
            }
        }
        //todo: fix time axis of roi plot
        //todo:
        ImPlot::EndPlot();
    }



    if (ImGui::Begin("ROI visualization", &event_trace_plot))
    {
        ImGui::InputFloat("Time resolution ", &interval_time, 0.000001, 0.0001, "%06f" );
        ImGui::NewLine();
        ImGui::Text("Input X-Y address: " );
        ImGui::InputFloat("X address to add: ", &new_X_plot);
        ImGui::NewLine();

        ImGui::InputFloat("Y address to add: ",&new_Y_plot);
        ImGui::NewLine();



        if (ImGui::Checkbox("Add", &add_plot))
        {
            X_plot.push_back(int(new_X_plot));
            Y_plot.push_back(int(new_Y_plot));
            add_plot= false;
        }
        if (ImGui::Checkbox("Add all", &add_plot_all))
        {
            for (int i=0;i<75;i++)
            {
                X_plot.push_back(i);
                for (int k=0;k<75;k++) {
                    Y_plot.push_back(k);
                }
            }
            add_plot_all= false;
        }
        ImGui::NewLine();
        ImGui::Text("Remove X-Y address: " );
        ImGui::NewLine();
        ImGui::InputFloat("X address to remove: ", &rm_X_plot);
        ImGui::NewLine();
        ImGui::InputFloat("Y address to remove: ",&rm_Y_plot);
        if (ImGui::Checkbox("Remove", &rm_plot))
        {
            int rm_it= 0;
            for (int i=0; i<Y_plot.size();i++){if (Y_plot[i]==int(rm_Y_plot)){rm_it = i; break;}}
            Y_plot.erase(Y_plot.begin()+rm_it );
            for (int i=0; i<X_plot.size();i++){if (X_plot[i]==int(rm_X_plot)){rm_it = i; break;}}
            X_plot.erase(X_plot.begin()+rm_it);
            rm_plot= false;
        }
        if (ImGui::Checkbox("Remove all", &rm_plot_all))
        {

            Y_plot.erase(Y_plot.begin(), Y_plot.end());
            X_plot.erase(X_plot.begin(), X_plot.end());
            rm_plot_all= false;
        }

        ImGui::Text("X-Y addresses in ROI: " );
        for (int i=0; i<X_plot.size();i++){
            ImGui::SameLine();
            ImGui::Text("%i-%i ",X_plot[i],Y_plot[i] );
        }
        ImGui::NewLine();


        if ( ImPlot::BeginPlot("Event visualization",  ImVec2(ImGui::GetWindowWidth()*0.7, ImGui::GetWindowHeight()*0.7)))
        {
            ImPlot::SetupAxisLimits(ImAxis_X1,0,plt_ev.size());//
            ImPlot::SetupAxisLimits(ImAxis_Y1,0, 10);//max_y
            ImPlot::PlotLine("Channel X,Y", &plt_time[0],&plt_ev[0],(plt_ev.size()));
            ImPlot::EndPlot();
        }
        ImGui::End();
    }
    ImGui::End();
}
void Visualizer::populate_index_time_bin(int itorator)
{

    if (pause_replay == false)
    {
        tmp1 =  tmp2;
        tmp2 = tmp1 + ((1./fps)/ slow_down);
    }
    if (pause_replay == true && next_replay==true )
    {
        tmp1 =  tmp2;
        tmp2 = tmp1 + ((1./fps)/ slow_down);
    }


    if (itorator == 0) // to start the visualization at the right time
    {
        tmp1= (loaded_gaia_events[0].ts*sampling_frequency);
        tmp2 = tmp1 + ((1./fps)/ slow_down);
    }
    //Delete plotted elements from buffer
    if ((index_time_plot>0 and pause_replay==false) or (index_time_plot>0 and next_replay==true)  )
    {
        loaded_gaia_events.erase (loaded_gaia_events.begin(), loaded_gaia_events.begin()+index_time_plot);
        loaded_gaia_events.shrink_to_fit();

    }
    index_time_plot = -1;
    //Select which events to plot at this iteration
    for (int ff =0; ff < (loaded_gaia_events.size()); ++ff)
    {
        if (  loaded_gaia_events[ff].ts*sampling_frequency <= tmp2 ) 
        {
            index_time_plot = ff+1;
        }
    } 

    if (loaded_gaia_events.size() ==0) 
    {
        cout << "All out of events to plot" << endl;
        live=true;
        playback = false;
        loaded_gaia_events.clear();    
    }
}

float Visualizer::distance_calculator(Event2d p1,Event2d p2 )
{   //calculating Euclidean distance
    float distance=0;
    float p = abs(int(p1.x - p2.x));
    float l = abs(int(p1.y - p2.y));
    distance = pow(p, 2.) + pow(l, 2.);
    distance = sqrt(distance);
    return distance;
}

void Visualizer::background_filter( )
{
    float dist;
    float time;
    for (int i =0; i<loaded_gaia_events_all.size()-1; i++){

        for (int j =i; j<loaded_gaia_events_all.size()-1; j++){
            time = ((loaded_gaia_events_all[j].ts - loaded_gaia_events_all[i].ts))*sampling_frequency;

            if  ( time >= time_cutoff) {break;}
            if  ( time < time_cutoff and i !=j)
            {
                dist = distance_calculator(loaded_gaia_events_all[i], loaded_gaia_events_all[j]);

                if (dist <= distance_cutoff and dist > 0.1) {

                    loaded_gaia_events.push_back(loaded_gaia_events_all[i]);
                    loaded_gaia_events.push_back(loaded_gaia_events_all[j]);
                    break;
                }
            }
        }
    }
}

void Visualizer::hot_pixel_filter() {
    if (bg_noise) {
        for (auto it = loaded_gaia_events_all.begin(); it != loaded_gaia_events_all.end(); it++) {
            if (std::find_if(blacklist.begin(), blacklist.end(),
                             [&it](Event2d &ev) { return ev.x == it->x && ev.y == it->y; }) != blacklist.end()) {
                it = loaded_gaia_events_all.erase(it);
            }
        }
    } else {
        for (auto it = loaded_gaia_events.begin(); it != loaded_gaia_events.end(); it++) {
            if (std::find_if(blacklist.begin(), blacklist.end(),
                             [&it](Event2d &ev) { return ev.x == it->x && ev.y == it->y; }) != blacklist.end()) {
                it = loaded_gaia_events.erase(it);
            }
        }
    }

}
