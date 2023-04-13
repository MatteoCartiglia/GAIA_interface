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
#include <iomanip>
#include <list>
#include "implot.h"
#include <vector>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../include/bias_control.h"

using namespace std;

#ifndef PI
#define PI 3.14159265358979323846
#endif

// // Encapsulates examples for customizing ImPlot.
// namespace MyImPlot {

// // Example for Custom Data and Getters section.
// struct Vector2f {
//     Vector2f(float _x, float _y) { x = _x; y = _y; }
//     float x, y;
// };

// // Example for Custom Data and Getters section.
// struct WaveData {
//     double X, Amp, Freq, Offset;
//     WaveData(double x, double amp, double freq, double offset) { X = x; Amp = amp; Freq = freq; Offset = offset; }
// };
// ImPlotPoint SineWave(void* wave_data, int idx);
// ImPlotPoint SawWave(void* wave_data, int idx);
// ImPlotPoint Spiral(void*, int idx);

// // Example for Tables section.
// void Sparkline(const char* id, const float* values, int count, float min_v, float max_v, int offset, const ImVec4& col, const ImVec2& size);

// // Example for Custom Plotters and Tooltips section.
// void PlotCandlestick(const char* label_id, const double* xs, const double* opens, const double* closes, const double* lows, const double* highs, int count, bool tooltip = true, float width_percent = 0.25f, ImVec4 bullCol = ImVec4(0,1,0,1), ImVec4 bearCol = ImVec4(1,0,0,1));

// // Example for Custom Styles section.
// void StyleSeaborn();

// } // namespace MyImPlot


// template <typename T>
// inline T RandomRange(T min, T max) {
//     T scale = rand() / (T) RAND_MAX;
//     return min + scale * ( max - min );
// }

// utility structure for realtime plot
struct ScrollingBufferADC {
    int MaxSize;
    int Offset;
    ImVector<ImVec2> Data;
    ScrollingBufferADC(int max_size = 2000) {
        MaxSize = max_size;
        Offset  = 0;
        Data.reserve(MaxSize);
    }
    void AddPoint(float x, float y) {
        if (Data.size() < MaxSize)
            Data.push_back(ImVec2(x,y));
        else {
            Data[Offset] = ImVec2(x,y);
            Offset =  (Offset + 1) % MaxSize;
        }
    }
    void Erase() {
        if (Data.size() > 0) {
            Data.shrink(0);
            Offset  = 0;
        }
    }
};

void ShowAdcChannelSubplot(const char* title, bool& show, float amplitude, float period, float offset, float history, ScrollingBufferADC &adcdata) {

    if (show){
    
        // static ScrollingBuffer adcdata, sdata1, sdata2;
        static float t = 0;
        t += ImGui::GetIO().DeltaTime;
        float data_point = amplitude * sin(period*t + offset);
        adcdata.AddPoint(t, data_point);
        
        static ImPlotAxisFlags flags_x = ImPlotAxisFlags_NoTickLabels;
        static ImPlotAxisFlags flags_y = ImPlotAxisFlags_AutoFit;

        if (ImPlot::BeginPlot(title, ImVec2(-1,150))) {
            ImPlot::SetupAxes("Time Scrolling Window", "Value", flags_x, flags_y);
            ImPlot::SetupAxisLimits(ImAxis_X1, t - history, t, ImGuiCond_Always);
            ImPlot::SetupAxisLimits(ImAxis_Y1, -amplitude, amplitude);
            ImPlot::SetNextFillStyle(IMPLOT_AUTO_COL,0.5f);
            ImPlot::PlotLine("", &adcdata.Data[0].x, &adcdata.Data[0].y, adcdata.Data.size(), adcdata.Offset, 2*sizeof(float));
            ImPlot::EndPlot();
        }
    }
}

void ShowAdcChannelGraphs(){

    const int n_adc_channels = 12;

    // Initializing necessary objects for the 12 ADC channels
    const char* ADC[n_adc_channels]  = {"CH 8", "CH 7", "CH 6", "CH 5", "CH 4", "CH 3", "CH 2", "CH 1", "CH 0", "G1: out", "G1: f_out", "NC"};
    static bool show[n_adc_channels] = {true,    true,    true,    true,    true,    true,    true,    true,    true,     true,    true,     true    }; 
    const float period[n_adc_channels] = {1.5, 2.1, 3.6, 4.8, 5, 6, 7, 8, 9, 10, 11};
    const float amplitude[n_adc_channels] = {1, 0, 7, 4, 5, 6, 7, 8, 9, 10, 11};
    const float offset[n_adc_channels] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    static ScrollingBufferADC adc[n_adc_channels] = {}; // Object where data is stored

    ImGui::Begin("Classical ADC Channel Window");

    int counter = 0;
    for (int r = 0; r < 2; r++){
        for (int c = 0; c < 6; c++){
            if (c > 0 && c%6 != 0) { ImGui::SameLine(); }
            ImGui::Checkbox(ADC[counter], &show[counter]);
            counter++;
        }
    }


    // Controling history of visualization
    static float history = 10.0f;
    ImGui::SliderFloat("History",&history,1,30,"%.1f s");

    static ImPlotSubplotFlags flags = ImPlotSubplotFlags_None;

    static int rows = 4;
    static int cols = 3;

    if (ImPlot::BeginSubplots("ADC Channel Subplots", rows, cols, ImVec2(-1,400), flags)){
        for (int i = 0; i < n_adc_channels; ++i){
            ShowAdcChannelSubplot(ADC[i],  show[i],  period[i],  amplitude[i],  offset[i],  history, adc[i]);
        }
        ImPlot::EndSubplots();
    }
    ImGui::End();

    // ImGui::Begin("Potential ADC or Spike Channel Window");
    // ShowDemo_Tables();
    // ImGui::End();
}

