#include "../../imgui/imgui_backend/imgui_impl_glfw.h"
#include "../../imgui/imgui_backend/imgui_impl_opengl3.h"
#include <stdio.h>
#include <iostream>

#if defined(IMGUI_IMPL_OPENGL_ES2)
#include <GLES2/gl2.h>
#endif
#include <GLFW/glfw3.h> // Will drag system OpenGL headers
#include <tuple>
#include <unistd.h>  /* UNIX standard function definitions */
#include <fcntl.h>   /* File control definitions */
#include <errno.h>   /* Error number definitions */
#include <termios.h> /* POSIX terminal control definitions */
#include <sstream>
#include <iomanip>
#include <list>
#include <chrono>
#include <signal.h>
#include <stdlib.h>
#include <ctime>
#include <thread>
#include <fstream>
#include "../src/include/bioamp_biases.h"
#include "../src/concurrentqueue.h"

#include "../src/include/dev.h"
#include "../src/data/events.cpp"
#include "../src/data/dev.cpp"
#include "../src/data/bias_control.cpp"
#include "../src/visualization/visualizer.cpp"
//#include "../src/visualization/visualize_adc_channel.cpp"

# define overflow_event_time_equivalent 65535.0f * sampling_frequency;
// top_sv_led as good as it gets

// top_sv_drf_invisible doesnt make events go through
// top_sv_drf doesnt make events go through


//top_sv_old works, but weird

//top_sv_led!
// top_sv_0504_nodelay_FSMFAST == error10
//top_sv_0504_nodelay_notrig
#define XILINX_CONFIGURATION_FILE  "bitfiles/top_sv_0504_nodelay_FSMFAST.bit"
volatile sig_atomic_t done = 0;

void my_handler(int a){ 
  done = 1;
}

static void glfw_error_callback(int error, const char* description){
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Decide GL+GLSL versions
 #if defined(IMGUI_IMPL_OPENGL_ES2)
    // GL ES 2.0 + GLSL 100
    const char* glsl_version = "#version 100";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
 #elif defined(__APPLE__)
    // GL 3.2 + GLSL 150
    const char* glsl_version = "#version 150";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // Required on Mac
 #else
    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);  // 3.2+ only
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);            // 3.0+ only
 #endif

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(1280, 720, "GAIA GUI", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    /////////////////// OPALKELLY INIT //////////////////////

    std::chrono::time_point<std::chrono::high_resolution_clock> start, end, origin;
    signal (SIGINT, my_handler); // catch ctrl-C ---- comes from signal library
    //Sigint: (Signal Interrupt) Interactive attention signal. Generally generated by the application user.

    // Instantiate BioAmp device
    Dev dev(XILINX_CONFIGURATION_FILE);

    bool open_live_visualization = true;
    bool open_replay_visualization = true;
    bool open_bias_control = true;
   // write_dummy_data();
   // create_event_vector("New_scheme.dat");
   // if FPGA is instatiated open live plots
    if (dev.is_opened)
    {
        bool open_live_visualization = true;
        bool open_replay_visualization = true;
        bool open_bias_control = true;
        // reset the device
        dev.reset();
        // join pooling thread
        dev.join_thread(&ev_buffer);
    }

    origin = std::chrono::high_resolution_clock::now();
    bool initialized_analog = false;
    bool initialized_digital = false;
    BiasData UpdatedBiasData;
    Visualizer Visualizer;
    // Main loop
    while (!glfwWindowShouldClose(window) && done == 0)
    {
        
        glfwPollEvents();
        // Start the Dear ImGui



        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        io.DeltaTime = 1.0f/60.0f;
        ImGui::NewFrame();
        
        //ImPlot::ShowDemoWindow();
        //ImGui::ShowDemoWindow();

            // If we want to visualize live events, then handle events and show the live visualization.
        if (dev.is_opened)
        {
            UpdatedBiasData = ShowBiasControlCenter(&open_bias_control);

            // If we haven't initialized the biases, we should initialized. 
            // If we want to load from a file (which is handled in bias_control.cpp), we must also set all biases again. 
            // This goes through the same logic as the initialization, for both analog biases and digital control. 
            if (initialized_analog == false || BIASES_STRUCT.load_new_set_of_analog_biases == true)
            {
                for (unsigned int i=0; i< N_BIASES; i++)
                {
                    biases.set((BioampBiases::Bias)i, UpdatedBiasData.ANALOG_BIASES[i]);
                }
                dev.set_biases(&biases);
                initialized_analog = true;
                cout << "Initialized Analog Biases" << endl;
            }
            if (initialized_digital == false || BIASES_STRUCT.load_new_set_of_digital_biases == true)
            {
                int init_write[7] = {WRITE_C0, WRITE_C1, WRITE_C2, WRITE_control_imp, WRITE_EN_G9, WRITE_EN_G1, WRITE_EN_G4096};
                for (unsigned int i=0; i < N_DIG_CONTROL; i++){
                    dev.set_wire(init_write[i], UpdatedBiasData.DIG_CONTROL[i]);
                    cout << "Set digital control #" << i << " to " << UpdatedBiasData.DIG_CONTROL[i] << endl;}
                initialized_digital = true;
                cout << "Initialized Digital Control" << endl;
            }

            // If change is detected in the analog or digital toggles/switches on the GUI, the "changed_bias/changed_control"  
            // bools get updated on bias_control.cpp. We can then modify the biases/control individually.
            if (UpdatedBiasData.changed_analog_bias == true)
            {
                dev.set_individual_bias(&biases, UpdatedBiasData.analog_bias_to_set, UpdatedBiasData.analog_new_val);
            }
            
            if (UpdatedBiasData.changed_digital_control == true)
            {
                cout << "Reading from: " << UpdatedBiasData.digital_read_pin << "--" << UpdatedBiasData.digital_write_pin << endl;
                dev.toggle_wire(UpdatedBiasData.digital_read_pin, UpdatedBiasData.digital_write_pin);
            }

            // If button "reset device" is clicked on the Bias Control center, reset_device return true, and we should reset 
            // the device.
            if (UpdatedBiasData.reset_device == true)
            {
                dev.reset();
                // Setting the following bools to false in order to initialize again at the next iteration. 
                // The value of the biases will be the same as on the previous iteration. 
                initialized_analog = false;
                initialized_digital = false;
                std::cout << "Device was reset! Bias and Digital controls will also be reset" << endl;
            }
        }
        
        if (dev.is_opened && dev.ok())
        {
            ///// EVENT HANDLER /////
            start = std::chrono::high_resolution_clock::now();
            
            // empty gaia_events when reset!
            if (UpdatedBiasData.reset_device == true)
            {
                Visualizer.gaia_events.clear();
                std::cout << UpdatedBiasData.reset_device << endl;

            }    
            // Check input data, emptying buffer
            /*while(ev_buffer.size() > 0)
            {
                dev.ev_buffer_lock.lock();
                Event2d ev = ev_buffer.front();
                Visualizer.gaia_events.push_back(ev);
                ev_buffer.pop();
                dev.ev_buffer_lock.unlock();
            }*/
            Event2d ev;

            while(ev_buffer.try_dequeue(ev))
            {
            Visualizer.gaia_events.push_back(ev);
            }
        }



        Visualizer.ShowGaiaVisualization(&open_live_visualization, UpdatedBiasData.reset_device);

       // if (open_replay_visualization == true)
       //     PlayGaiaVisualizationFromFile(&open_replay_visualization);
   
        /*
         {

            //ImGui::Begin("Custom plotting");
            float offset = 500;
            ImGui::Text("size = %d x %d", image_width, image_width);
            const ImVec2 min = ImVec2(500.0f, 500.0f);
            const ImVec2 max = ImVec2(50.0f, 10.0f);
            const ImVec2 mid = ImVec2(10.0f, 500.0f);

            ImDrawList* draw_list = ImGui::GetWindowDrawList();;
           // draw_list->AddRectFilled(min, max, IM_COL32(64, 64, 64, 200));
           // draw_list->AddCircle(min, 10., IM_COL32(90, 90, 120, 255));
           // draw_list->AddTriangleFilled(min, max, mid,IM_COL32(90, 90, 120, 255));
            draw_list ->AddRectFilled(ImVec2(500.0f, 200.0f), ImVec2(505.0f, 205.0f),IM_COL32(255, 0, 0, 100));
            draw_list ->AddRectFilled(ImVec2(400.0f, 200.0f), ImVec2(405.0f, 205.0f),IM_COL32(255, 0, 0, 100));
            draw_list ->AddRectFilled(ImVec2(400.0f, 200.0f), ImVec2(405.0f, 205.0f),IM_COL32(255, 0, 0, 100));
            draw_list ->AddRectFilled(ImVec2(300.0f, 200.0f), ImVec2(305.0f, 205.0f),IM_COL32(0, 0, 255, 100));

           // ImGui::End();
            }*/
        //// ADC VISUALIZATION 
         // ShowAdcChannelGraphs();
        
        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
