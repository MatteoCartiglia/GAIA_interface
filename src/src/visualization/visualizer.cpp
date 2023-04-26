#include "../../imgui/imgui_backend/imgui_impl_glfw.h"
#include "../../imgui/imgui_backend/imgui_impl_opengl3.h"
#include "../../imgui/ImGuiFileDialog.h"
#include "../../imgui/Implot.h"

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

#include "../include/Filters.h"
#include "../visualization/Filters.cpp"

using namespace std;

long long first_ts;
extern const int begin_time;
Visualizer::Visualizer() : recording_filter(nullptr),  heatmap_filter(75, 75){
    prev_frame_time = std::chrono::high_resolution_clock::now();

}
Visualizer::~Visualizer(){
    if (recording_filter != nullptr) {
        delete recording_filter;
    }
}


void Visualizer::renderEvents(bool* live_visualization_open, bool reset, bool fifo_full) {
        ImGui::Begin("GAIA Live Visualization board", live_visualization_open);
        // time handling varibale
        current_frame_time = std::chrono::high_resolution_clock::now();
        frame_interval = current_frame_time - prev_frame_time;
        prev_frame_time = current_frame_time;
        fps =1 / ImGui::GetIO().DeltaTime;

    // Create checkboxes for filter activation states
        ImGui::Checkbox("Hotpixel Filter", &hotpixel_active);
        displaySelectedHotPixels();
        ImGui::Checkbox("Coincidence Filter", &coincidence_active);
        if(coincidence_active){AP_filter_Active = false;}
        ImGui::SameLine();
        ImGui::Checkbox("AP Filter", &AP_filter_Active);
        if(AP_filter_Active){coincidence_active = false;}
        ImGui::InputFloat("Coinc Distance cutoff: ", &CoincidenceFilter.distance_cutoff);
        ImGui::InputFloat("Coinc Time cutoff [in ms]: ", &CoincidenceFilter.time_cutoff, 0.000001, 1, "%06f");
        ImGui::InputFloat("AP Distance cutoff: ", &APFilter.distance_cutoff);
        ImGui::InputFloat("AP Time cutoff [in ms]: ", &APFilter.time_cutoff, 0.000001, 1, "%06f");
        ImGui::Checkbox("Recording Filter", &recording_active);
        ImGui::SameLine();
        ImGui::Checkbox("Live Mode", &isLive);
        ImGui::SameLine();
        ImGui::Checkbox("Playback Mode", &isPlayback);
        ImGui::SameLine();
        ImGui::Checkbox("Heatmap", &heatmap_active);
        ImGui::SameLine();
        ImGui::Checkbox("Pause ", &playback_paused);
        ImGui::SameLine();
        ImGui::Checkbox("Next Frame ", &nextFrame);
        ImGui::SameLine();
        ImGui::Checkbox("Previous Frame ", &prevFrame);
        ImGui::Text("Time Scale");
        ImGui::SameLine();
        ImGui::SliderFloat("##Time Scale", &time_scale, 0.0001f, 1.0f, "%.4f");
        ImGui::SameLine();
        ImGui::Text("0.03 for 500us intervals");
        float frame_duration = time_scale / (fps * sampling_frequency);
        ImGui::Text("Frame Duration: %.3f microseconds", frame_duration*10);


    filter_map = (hotpixel_active << Hotpixel) |
                (coincidence_active << Coincidence) |
                (recording_active << Recording);



        processEvents(gaia_events, filter_map);

        showSavingPopup(filename);
        filePicker();
        handleMouseClick();

        // Get the current window draw list
        ImVec2 top_left_corner(pixels_X_offset, pixels_Y_offset);
        ImVec2 bottom_right_corner(pixels_X_offset + 75 * circle_diameter, pixels_Y_offset + 75 * circle_diameter);
        ImU32 rectangle_color = ImColor(255, 255, 255,50); // White
        ImGui::GetWindowDrawList()->AddRect(top_left_corner, bottom_right_corner, rectangle_color);


    // Check if fifo_full is asserted
    if (checkFifoFull(fifo_full)) {
        ImVec2 center(ImGui::GetWindowSize().x / 2, ImGui::GetWindowSize().y / 2);
        ImU32 color = ImColor(0, 0, 255); // Blue
        ImGui::GetWindowDrawList()->AddCircleFilled(center, circle_diameter / 2.0f, color);
    }
    else {
        // Draw the canvas
        visualizeSelectedHotPixels();
        drawCanvas(live_visualization_open);
    }
        ImGui::End();
}
void Visualizer::visualizeSelectedHotPixels() {

    if (hotpixel_active) {
        const auto &hot_pixels = hotPixel_Filter.getSelectedHotPixels();
        ImU32 hot_pixel_color = ImColor(128, 128, 128, 200); // Grey
        for (const auto &pixel: hot_pixels) {
            ImVec2 hot_pixel_top_left(pixel.first * circle_diameter + pixels_X_offset,
                                      pixel.second * circle_diameter + pixels_Y_offset);
            ImVec2 hot_pixel_bottom_right(hot_pixel_top_left.x + circle_diameter,
                                          hot_pixel_top_left.y + circle_diameter);
            ImGui::GetWindowDrawList()->AddRectFilled(hot_pixel_top_left, hot_pixel_bottom_right, hot_pixel_color);
        }
    }
}


void Visualizer::displaySelectedHotPixels() {
    if (hotpixel_active) {
        ImGui::SameLine();
        const auto& hot_pixels = hotPixel_Filter.getSelectedHotPixels();
        // Print hot pixels on screen
        ImGui::Text("Selected Hot Pixels:");

        int counter = 0;
        for (const auto& pixel : hot_pixels) {
            if (counter % 8 == 0 && counter != 0) {
                ImGui::NewLine();
            }
            ImGui::SameLine();
            ImGui::Text("X-Y: %d-%d ", pixel.first, pixel.second);
            counter++;
        }
    }
}


void Visualizer::filePicker() {
    if (ImGui::Button("Load Events")) {
        isLive=false;
        show_file_picker = true;
        ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose File", ".dat,.cpp,.h,.hpp",
                                                ".",1);
    }
        if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey")) {
            if (ImGuiFileDialog::Instance()->IsOk()) {
                std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
                loadEvents(filePathName);
                std::cout << filePathName << std::endl;
            }
            ImGuiFileDialog::Instance()->Close();
            show_file_picker = false;

        }

}

void Visualizer::loadEvents(const std::string& filename) {
    std::ifstream file(filename, std::ifstream::binary);
    Event2d ev;

    gaia_loaded_events.clear();

    while (file.good()) {
        file.read((char *)&ev, sizeof(ev));
        gaia_loaded_events.push_back(ev);

    }
    file.close();

    std::cout << "Loaded event : " << gaia_loaded_events.size() << std::endl;

    //  All processing that needs to happen on loaded events.
    itorator = 0;
    removeTsOffset();
    isPlayback = true;
    sortEvents();
    std::cout << "Loaded event after sorting: " << gaia_loaded_events.size() << std::endl;

    if( hotpixel_active) {
        hotPixel_Filter.process(gaia_loaded_events);
    }
    std::cout << "Loaded event af hotpix: " << gaia_loaded_events.size() << std::endl;

    if (coincidence_active) {
        CoincidenceFilter.processing(gaia_loaded_events);
    }
    if (AP_filter_Active) {
        APFilter.processing(gaia_loaded_events);
    }
    std::cout << "Loaded event af coincidence: " << gaia_loaded_events.size() << std::endl;

    if(heatmap_active){
        heatmap_filter.process(gaia_loaded_events);
        heatmap_filter.calculateEventCounts(gaia_loaded_events, 100);
    }

}

void Visualizer::filterEventsByTimestamp(){
//    std::cout<< "Dim gaia_loaded_events " << gaia_loaded_events.size() << std::endl;
//    std::cout<< "First ts " << gaia_loaded_events.begin()->ts << std::endl;
//    std::cout<< "Last ts " << gaia_loaded_events.end()->ts << std::endl;
//    std::cout<< "ts slice start: " << start_ts << std::endl;

    // 10 frames as buffer
    buffer_time = start_ts - (end_ts - start_ts)*4;
    if (buffer_time<0) {
        buffer_time = 0;
    }
    int ind=0;
    for (int i = 0; i < gaia_loaded_events.size(); i++) {
        if (gaia_loaded_events[i].ts <= (buffer_time)) {
            gaia_loaded_events.erase(gaia_loaded_events.begin() + ind);
            ind++;
        } else { break; }
    }
    if (gaia_loaded_events.empty()){
        std::cout << "Out of events!!" << std::endl;
        isPlayback = false;
        isLive = false;
    }
}

void Visualizer::updateEventTimeframe() {
    if (playback_paused && nextFrame) {
        handleNextFrame();
    } else if (playback_paused&& prevFrame) {
        handlePrevFrame();
    } else if (playback_paused) {
        updateGaiaEvents();
    } else if (!playback_paused && isPlayback) {
        handlePlayback();
    }
}

void Visualizer::handleNextFrame() {
    start_ts = end_ts;
    end_ts = start_ts + ((1. / fps) * time_scale) * 1 / sampling_frequency;
    playback_paused = true;
    updateGaiaEvents();
    nextFrame = false;
}

void Visualizer::handlePrevFrame() {
    end_ts = start_ts;
    start_ts = end_ts - 2 * ((1. / fps) * time_scale) * 1 / sampling_frequency;
    playback_paused = true;
    updateGaiaEvents();
    prevFrame = false;
}

void Visualizer::handlePlayback() {
    if (itorator == 0) {
        start_ts = 0;
        end_ts = start_ts + ((1. / fps) * time_scale) * 1 / sampling_frequency;
    } else {
        start_ts = end_ts;
        end_ts = start_ts + ((1. / fps) * time_scale) * 1 / sampling_frequency;
    }
    itorator++;
    updateGaiaEvents();
    filterEventsByTimestamp();
}

void Visualizer::updateGaiaEvents() {
    for (int i = 0; i < gaia_loaded_events.size(); i++) {
        if (gaia_loaded_events[i].ts > start_ts && gaia_loaded_events[i].ts <= end_ts) {
            gaia_events.push_back(gaia_loaded_events[i]);
        }
        else if (gaia_loaded_events[i].ts > end_ts){break;}
    }
}


void Visualizer::removeTsOffset() {
    if (gaia_loaded_events.empty()) {
        return;
    }
    uint64_t first_timestamp = gaia_loaded_events[0].ts;
    for (Event2d& event : gaia_loaded_events) {
        event.ts -= first_timestamp;
    }
}
void Visualizer::drawHeatmap() {

    if(ImGui::Begin("Heatmap Window", &show_heatmap_window)){
    //mImGui::SetNextWindowSize(ImVec2(ImGui::GetWindowWidth()*1,  ImGui::GetWindowHeight() * 1));
    ImGui::Checkbox("ON Events ", &heatmap_filter.visualize_on);
    ImGui::Checkbox("OFF Events ", &heatmap_filter.visualize_off);

    ImPlot::BeginPlot("GAIA 4096", ImVec2(ImGui::GetWindowWidth() * 0.6, ImGui::GetWindowHeight() * 0.6)) ;


    float max_value = *std::max_element(std::begin(heatmap_filter.GAIAEvents), std::end(heatmap_filter.GAIAEvents));
    float min_value = *std::min_element(std::begin(heatmap_filter.GAIAEvents), std::end(heatmap_filter.GAIAEvents));

        ImPlot::PlotHeatmap("GAIA HEATMAP ", heatmap_filter.GAIAEvents, 75, 75, min_value, max_value, NULL, ImPlotPoint(0, 0), ImPlotPoint(75, 75));

        if (ImPlot::IsPlotHovered() && ImGui::IsMouseClicked(0)) {
            ImPlotPoint pt = ImPlot::GetPlotMousePos();
            if (hotpixel_active) {
                Event2d ev;
                ev.x = floor(pt.x);
                ev.y = floor(pt.y);
                heatmap_filter.blacklist.push_back(ev);
            }
        }

        ImPlot::EndPlot();

        ImGui::SameLine();
    static ImPlotColormap map = ImPlotColormap_Jet;
    ImPlot::PushColormap(map);
    ImPlot::ColormapScale("##ID",min_value, max_value,ImVec2(60,225));

    ImGui::NewLine();
    ImGui::Text("Number of events: %d",heatmap_filter.event_number);
    ImGui::Text("Number of ON events: %d",heatmap_filter.event_number_on);
    ImGui::Text("Number of OFF events: %d",heatmap_filter.event_number_off);
    ImGui::Text("Blacklist: ");
    for (int i=0;i<heatmap_filter.blacklist.size();i++){
        ImGui::Text(" %d-%d ",heatmap_filter.blacklist[i].x ,heatmap_filter.blacklist[i].y);
    }
    // Draw the event plot over time
        if (ImPlot::BeginPlot("Event Counts", "Time (us)", "Event Count", ImVec2(ImGui::GetWindowWidth() * 0.6, ImGui::GetWindowHeight() * 0.5))) {
            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green for ON events
            ImPlot::PlotLine("ON Events", &heatmap_filter.onEventCounts[0], heatmap_filter.onEventCounts.size(), 0,
                             sizeof(int));
            ImPlot::PopStyleColor();

            ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red for OFF events
            ImPlot::PlotLine("OFF Events", &heatmap_filter.offEventCounts[0], heatmap_filter.offEventCounts.size(), 0,
                             sizeof(int));
            ImPlot::PopStyleColor();

            ImPlot::EndPlot();
        }
    ImGui::End();
    }
}

void Visualizer::processEvents(std::vector<Event2d>& input_events, uint8_t filter_map) {

    // playback of pre-recorded events
    if(heatmap_active){
        drawHeatmap();
    }

    if (isPlayback){
        updateEventTimeframe();
    }
    // Hotpixel filter
    if ((filter_map & (1 << Hotpixel))){
        hotPixel_Filter.process(input_events);
    }

    // SAVING EVENTS!
    if (recording_active != prev_recording_active && (filter_map & (1 << Recording))) {
        recordingInitializer();
    }
    if (recording_active != prev_recording_active&& !(filter_map & (1 << Recording)) ) {
        recordingFinisher();
    }
    if (recording_filter) {
        recording_filter->process(input_events);
    }

    prev_recording_active = recording_active;
    if(isLive && coincidence_active){

      //  CoincidenceFilter.liveFiltering(gaia_events);
    }

}
bool Visualizer::checkFifoFull(bool fifo_full) {
    return fifo_full;
}

void Visualizer::drawCanvas(bool* live_visualization_open) {

      for (const auto& event : gaia_events) {
          ImVec2 center(event.x * circle_diameter + circle_diameter / 2.0f + pixels_X_offset,
                        event.y * circle_diameter + circle_diameter / 2.0f + pixels_Y_offset);
          ImU32 color;
          if (event.p == 0) {
              color = ImColor(255, 0, 0, 150); // Red
          } else {
              color = ImColor(0, 255, 0, 150); // Green
          }
          // Draw the dot
          ImGui::GetWindowDrawList()->AddCircleFilled(center, circle_diameter / 2.0f, color);
      }
      popIncomingEvents();
}
void Visualizer::popIncomingEvents() {
    if (coincidence_active && isLive) {
        int64_t time_threshold = CoincidenceFilter.time_cutoff * 1e2; //
        // Remove events older than the specified time threshold
        gaia_events.erase(std::remove_if(gaia_events.begin(), gaia_events.end(),
                                         [this, time_threshold](const Event2d &event) {
                                             return (current_timestamp - event.ts) > time_threshold;
                                         }),
                          gaia_events.end());
    }
    else {
        gaia_events.clear();
        gaia_events.shrink_to_fit();
    }
}


pixel_click Visualizer::onMouseClick(int mouse_x, int mouse_y) {
    pixel_click clicked_pixel;
    clicked_pixel.x = (mouse_x - pixels_X_offset) / circle_diameter;
    clicked_pixel.y = (mouse_y - pixels_Y_offset) / circle_diameter;
   // std::cout << "Sensor pixel selected: (" << clicked_pixel.x << ", " << clicked_pixel.y << ")\n";
return clicked_pixel;
}

void Visualizer::handleMouseClick() {
    if (!show_file_picker) {
        if (ImGui::IsMouseClicked(0)) { // Check for left mouse button click
            ImVec2 mouse_pos = ImGui::GetMousePos(); // Get mouse position in screen coordinates
            int mouse_x = static_cast<int>(mouse_pos.x);
            int mouse_y = static_cast<int>(mouse_pos.y);

            // Ensure that the click is within the canvas area
            if (mouse_x >= pixels_X_offset && mouse_x < pixels_X_offset + 75 * circle_diameter &&
                mouse_y >= pixels_Y_offset && mouse_y < pixels_Y_offset + 75 * circle_diameter) {
                pixel_click clicked_pixel = onMouseClick(mouse_x, mouse_y);
                if (hotpixel_active) {
                    hotPixel_Filter.checkHotpixel(clicked_pixel.x, clicked_pixel.y);
                }
            }
        }
    }
}
void Visualizer::recordingInitializer() {
    if (recording_active) {
        if (recording_filter == nullptr) {
            recording_filter = new RecordingFilter(filename);
        }
        recording_filter->enable_record();

    }
}
void Visualizer::recordingFinisher() {
    recording_filter->enable_record();
    show_rename_popup = true;

    if (recording_filter != nullptr) {
        delete recording_filter;
        recording_filter = nullptr;
    }
}

void Visualizer::showSavingPopup(const std::string& filename) {

    if (show_rename_popup) {
        ImGui::OpenPopup("Rename File");
    }
    if (ImGui::BeginPopupModal("Rename File", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {

        ImGui::Text("Name the file... ");

        static char new_filename[128] = "NAME__.dat\" ";
        ImGui::InputText(" ", new_filename, IM_ARRAYSIZE(new_filename));

        if (ImGui::Button("Save")) {
            std::string new_path = "" + std::string(new_filename);
            std::rename(filename.c_str(), new_path.c_str());
            show_rename_popup = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
            show_rename_popup = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::EndPopup();
    }
}

void Visualizer::sortEvents() {
    // Sort the events based on their timestamps
    std::sort(gaia_loaded_events.begin(), gaia_loaded_events.end(), [](const Event2d& a, const Event2d& b) {
        return a.ts < b.ts;
    });
    // Remove duplicates from the sorted array
    auto last_unique = std::unique(gaia_loaded_events.begin(), gaia_loaded_events.end(),
                                   [](const Event2d& a, const Event2d& b) {
        return a.x == b.x && a.y == b.y && a.ts == b.ts && a.p == b.p;
    });

    // Resize the vector to remove the duplicate elements
    gaia_loaded_events.resize(std::distance(gaia_loaded_events.begin(), last_unique));

}
