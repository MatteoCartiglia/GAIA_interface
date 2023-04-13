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
#include <sys/types.h>
#include <tuple>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <experimental/filesystem>
#include <iostream>


#include "../include/bias_control.h"

#include "bioamp_biases.cpp"

using namespace std;
namespace fs = std::experimental::filesystem;

#define BIAS_FILE (char*)"GAIA.bias"

struct BiasData{
    int  ANALOG_BIASES [N_BIASES] = {0};
    bool DIG_CONTROL [N_DIG_CONTROL] = {0};
    
    bool load_new_set_of_analog_biases = false;
    bool load_new_set_of_digital_biases = false;
    
    bool changed_analog_bias = false;
    bool changed_digital_control = false;

    BioampBiases::Bias analog_bias_to_set;
    int analog_new_val;
    
    BioampBiases::Bias digital_control_changed;
    int digital_read_pin; 
    int digital_write_pin;

    bool initialized_digital_bias = false;
    bool reset_device = false;
} BIASES_STRUCT;


// Initiate Biases
BioampBiases biases(BIAS_FILE);

bool loadBiases(char *filename){
    char buf[256];
    ifstream file;
    file.open(filename);
    if(!file.good()) {std::cout << "Error loading biases" << std::endl;return false;}
    // Discard header
    for (;file.peek() == '%'; file.getline(buf, sizeof(buf)));
    // Read biases
    int counter = 0;
    for(int i = 0; i < N_BIASES; ++i)
    { 
        file.getline(buf, sizeof(buf));
        BIASES_STRUCT.ANALOG_BIASES[i] =  atoi(buf);
        counter += 1;
    }
    for(int i = 0; i < N_DIG_CONTROL; ++i)
    {
        file.getline(buf, sizeof(buf));
        BIASES_STRUCT.DIG_CONTROL[i] =  atoi(buf);
        counter += 1;
    }
    // Verify if the file had enough data in it
    if (counter < !N_BIASES) {
        std::cout << "Error loading biases" << std::endl;
        return false;
    }
    // Close file
    else{file.close();return true;};
}

bool loadBiasesOld(char *filename){
    ifstream inFile;
    inFile.open(filename);
    if (inFile.is_open()) {
        for(int i = 0; i < N_BIASES; ++i)
        { 
            inFile >> BIASES_STRUCT.ANALOG_BIASES[i];
        }
        for(int i = 0; i < N_DIG_CONTROL; ++i)
        {
            inFile >> BIASES_STRUCT.DIG_CONTROL[i];
        }
        return true;
    }
    else{
        return false;
    };  
}

bool saveBiases(char *filename){
    ofstream fout(filename);
    if(fout.is_open())
	{
        for (int i = 0; i < N_BIASES; i++) 
		{
            fout << BIASES_STRUCT.ANALOG_BIASES[i] << '\n'; 
		}
        for (int i = 0; i < N_DIG_CONTROL; i++) 
		{
            fout << BIASES_STRUCT.DIG_CONTROL[i] << '\n'; 
		}
        return true;
	}
	else 
	    return false;
}

void showSaveLoadBiases(){

    if (ImGui::Button("Save biases"))
        ImGui::OpenPopup("Saving Biases Menu");

    ImGui::SameLine();
    
    if (ImGui::Button("Load biases"))
        ImGui::OpenPopup("Loading Biases Menu");

    //////////////// SAVE

    if (ImGui::BeginPopupModal("Saving Biases Menu", NULL, ImGuiWindowFlags_MenuBar))
        {
            ImGui::Text("Settings: gain_thresholds_band_chip_etc");
            static char filename[128] = "Biases/__NAME__.bias";
            ImGui::InputText(" ", filename, IM_ARRAYSIZE(filename));
            ImGui::SameLine();

            ImGui::PushID(0);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(3 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(3 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(3 / 7.0f, 0.8f, 0.8f));
            bool save_button = false;
            save_button = ImGui::Button("Save");
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            if (save_button){
                int saved_biases = saveBiases(filename);
                if (saved_biases == true) {
                    ImGui::CloseCurrentPopup();
                    } 
                else{
                    ImGui::Text("Failed to save the biases! :(");
                    }
            }
            ImGui::PushID(0);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0 / 7.0f, 0.8f, 0.8f));
            bool close_button = false;
            close_button = ImGui::Button("Close");
            ImGui::PopStyleColor(3);
            ImGui::PopID();
            if (close_button)
                ImGui::CloseCurrentPopup();
            ImGui::EndPopup();
        }

    //////////////// LOAD

    if (ImGui::BeginPopupModal("Loading Biases Menu", NULL, ImGuiWindowFlags_MenuBar)){
        
        ImGui::Text("Choose a your favourite set of biases... ");  
        std::string path = "Biases/";


        //// Obtaining the list of files in the Biases/ Directory. This will allow us to display buttons 
        //// And allow the user to choose which file to load. 
        
        /// See how many files we have first
        int n_files = 0; 
        for (const auto& dirEntry: fs::directory_iterator(path)){
            string filename_with_path = dirEntry.path();
            n_files = n_files + 1;
        }

        //// Put filenames into array of chars.
        char* biases_filenames_no_path [n_files];
        char* biases_filenames_with_path [n_files];
        int iter = 0;
        for (const auto& dirEntry: fs::directory_iterator(path)){

            string filename_with_path = dirEntry.path();
            string filename_no_path = filename_with_path.substr(filename_with_path.find('/') + 1, filename_with_path.length() - filename_with_path.find('/'));
            
            biases_filenames_with_path[iter] = new char[filename_with_path.length() + 1];
            strcpy(biases_filenames_with_path[iter], filename_with_path.c_str());
            
            biases_filenames_no_path[iter] = new char[filename_no_path.length() + 1];
            strcpy(biases_filenames_no_path[iter], filename_no_path.c_str());

            iter++;
        }

        for (int i = 0; i < n_files; i++)
        {
            if (i % 3 != 0)
                ImGui::SameLine();

            ImGui::PushID(i);
            ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(4 / 7.0f, 0.6f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(4 / 7.0f, 0.7f, 0.7f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(4 / 7.0f, 0.8f, 0.8f));

            if (ImGui::Button(biases_filenames_no_path[i])){
                int loaded_biases = loadBiases(biases_filenames_with_path[i]);
                if (loaded_biases == true) {
                    // If biases successfully loaded into our BIASES_STRUCT which will triger the update on chip in the main. 
                    BIASES_STRUCT.load_new_set_of_analog_biases = true;
                    BIASES_STRUCT.load_new_set_of_digital_biases = true;
                    ImGui::CloseCurrentPopup();
                    } 
                else{
                    ImGui::Text("Failed to load the biases! :(");
                    }
            }
            ImGui::PopStyleColor(3);
            ImGui::PopID();
        }

        ImGui::PushID(0);
        ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)ImColor::HSV(0 / 7.0f, 0.6f, 0.6f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)ImColor::HSV(0 / 7.0f, 0.7f, 0.7f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)ImColor::HSV(0 / 7.0f, 0.8f, 0.8f));
        bool close_button = false;
        close_button = ImGui::Button("Close");
        ImGui::PopStyleColor(3);
        ImGui::PopID();
        if (close_button)
            ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

// This simply takes care of loading the default BIAS_FILE
bool initial_load = loadBiases(BIAS_FILE);

BiasData ShowBiasControlCenter(bool* bias_control_open){

    // Helper variable to detect if the user would like to reset the device.
    BIASES_STRUCT.reset_device = false;

    // Helper variables to detect if the user would like to load a new set of bias from file. 
    BIASES_STRUCT.load_new_set_of_analog_biases = false;
    BIASES_STRUCT.load_new_set_of_digital_biases = false;

    // Helper variables to detect changes to values. This is sent back to the main and triggers an update when necessary.
    // This gets updated if the Load Bias button is pressed *and* a file is selected. The bool is then sent back to main 
    // for loading the biases onto the chip.   
    BIASES_STRUCT.changed_analog_bias = false;
    BIASES_STRUCT.changed_digital_control = false;
    
    // Begin visualization
    ImGui::Begin("Bias configuration", bias_control_open);    
    ImGui::Text("Biases (mV)");

    ImGui::SliderInt(  "vref (Vout1)" ,  &BIASES_STRUCT.ANALOG_BIASES[1] ,  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.vref;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[1];}

    ImGui::SliderInt(  "lp_v (Vout2)" ,  &BIASES_STRUCT.ANALOG_BIASES[2] ,  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.lp_v;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[2];}

    ImGui::SliderInt(  "ibn  (Vout3)" ,  &BIASES_STRUCT.ANALOG_BIASES[3] ,  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.ibn;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[3];}
    
    ImGui::SliderInt(  "tail_comp_dn" ,  &BIASES_STRUCT.ANALOG_BIASES[8] ,  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.tail_comp_dn;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[8];}

    ImGui::SliderInt(  "tail_comp_up" ,  &BIASES_STRUCT.ANALOG_BIASES[9] ,  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.tail_comp_up;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[9];}

    ImGui::SliderInt(  "vthup"        ,  &BIASES_STRUCT.ANALOG_BIASES[10],  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.vthup;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[10];}

    ImGui::SliderInt(  "vthdn"        ,  &BIASES_STRUCT.ANALOG_BIASES[11],  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.vthdn;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[11];}

    ImGui::SliderInt(  "ref_p"        ,  &BIASES_STRUCT.ANALOG_BIASES[12],  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.ref_p;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[12];}

    ImGui::SliderInt(  "tail_stage_2" ,  &BIASES_STRUCT.ANALOG_BIASES[13],  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.tail_stage_2;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[13];}

    ImGui::SliderInt(  "ref_electrode",  &BIASES_STRUCT.ANALOG_BIASES[14],  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.ref_electrode;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[14];}

    ImGui::SliderInt(  "vbn"          ,  &BIASES_STRUCT.ANALOG_BIASES[15],  0,  1800, "%d");
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_analog_bias = true;
        BIASES_STRUCT.analog_bias_to_set = biases.vbn;
        BIASES_STRUCT.analog_new_val = BIASES_STRUCT.ANALOG_BIASES[15];}
 
    
    ImGui::Text("Digital Biases (mV)");

    ImGui::Checkbox( "C0" ,&BIASES_STRUCT.DIG_CONTROL[0]);
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_digital_control = true;
        BIASES_STRUCT.digital_read_pin = biases.r_C0;
        BIASES_STRUCT.digital_write_pin = biases.w_C0;}

    ImGui::Checkbox( "C1" ,&BIASES_STRUCT.DIG_CONTROL[1]);
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_digital_control = true;
        BIASES_STRUCT.digital_read_pin = biases.r_C1;
        BIASES_STRUCT.digital_write_pin = biases.w_C1;}

   ImGui::Checkbox( "C2" ,&BIASES_STRUCT.DIG_CONTROL[2]);
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_digital_control = true;
        BIASES_STRUCT.digital_read_pin = biases.r_C2;
        BIASES_STRUCT.digital_write_pin = biases.w_C2;}

    ImGui::Checkbox( "control_imp", &BIASES_STRUCT.DIG_CONTROL[3]);
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_digital_control = true;
        BIASES_STRUCT.digital_read_pin = biases.r_control_imp;
        BIASES_STRUCT.digital_write_pin = biases.w_control_imp;}

    ImGui::Checkbox( "EN_G9", &BIASES_STRUCT.DIG_CONTROL[4]);
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_digital_control = true;
        BIASES_STRUCT.digital_read_pin = biases.r_EN_G9;
        BIASES_STRUCT.digital_write_pin = biases.w_EN_G9;}
    
    ImGui::Checkbox( "EN_G1", &BIASES_STRUCT.DIG_CONTROL[5]);
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_digital_control = true;
        BIASES_STRUCT.digital_read_pin = biases.r_EN_G1;
        BIASES_STRUCT.digital_write_pin = biases.w_EN_G1;}

    ImGui::Checkbox( "EN_G4096", &BIASES_STRUCT.DIG_CONTROL[6]);
    if (ImGui::IsItemEdited()){
        BIASES_STRUCT.changed_digital_control = true;
        BIASES_STRUCT.digital_read_pin = biases.r_EN_G4096;
        BIASES_STRUCT.digital_write_pin = biases.w_EN_G4096;}

    showSaveLoadBiases();
    bool Reset_Button = ImGui::Button("Reset Device");
    if (Reset_Button){BIASES_STRUCT.reset_device = true;}
    ImGui::End();

    return BIASES_STRUCT;
}





