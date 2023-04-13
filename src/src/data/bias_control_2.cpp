

//---------------------------------------------------------------------------------------------------------------------------------------
// parseCSV: Parses CSV files containing POR bias value for the DAC and Bias Generator
//---------------------------------------------------------------------------------------------------------------------------------------

std::vector<std::vector<std::string>> parseCSV(const std::string& path)
{
    std::ifstream input_file(path);
    std::string line;
    std::vector<std::vector<std::string>> parsedCSV;
    int noFileLines = getFileLines(path);

    // Iterating over all lines in the csv file
    for(int i = 0; i <noFileLines; i++)
    {
        std::getline(input_file, line);
        std::stringstream lineStream(line);
        std::string cellValue;
        std::vector<std::string> parsedRow;

        // Excluding the first line with column titles
        if(i > 0)
        {
            // Iterating over the values per row
            while(std::getline(lineStream, cellValue, ','))
            {
                parsedRow.push_back(cellValue);
            }
            parsedCSV.push_back(parsedRow);
        }
    }

    return parsedCSV;
}

//---------------------------------------------------------------------------------------------------------------------------------------
// getDACvalues: Initialises DAC_command array with values from CSV file
//---------------------------------------------------------------------------------------------------------------------------------------

void getDACValues(DAC_biases dac[], const std::string filename = DAC_BIASFILE )
{
    std::vector<std::vector<std::string>> parseCSVoutput = parseCSV(filename);

    for (int i = 0; i < (int) parseCSVoutput.size(); i++)
    {
        // Defining an string of spaces for consistent sizing in GUI
        std::string dacBiasName = "                       ";

        for(int j = 0; j < (int) parseCSVoutput[i][0].length(); j++)
        {
            dacBiasName[j] = parseCSVoutput[i][0][j];
        }
        dac[i].name = dacBiasName;
        dac[i].value = (uint16_t) std::stoi(parseCSVoutput[i][1]);
        dac[i].address = (uint16_t) std::stoi(parseCSVoutput[i][1]);
    }
}


//---------------------------------------------------------------------------------------------------------------------------------------
// setupDacWindow: Initialises and updates GUI window displaying DAC values to send
//---------------------------------------------------------------------------------------------------------------------------------------

int setupDacWindow(bool show_DAC_config, DAC_biases dac[],  bool updateValues)
{

    ImGui::Begin("Biases [DAC Values]", &show_DAC_config);

    for(int i=0; i<N_BIASES; i++)
    {
        // Using push/pop ID to create unique identifiers for widgets with the same label
        ImGui::PushID(i);

        // Labelling the DAC input channel
        ImGui::Text(dac[i].name.c_str());
        ImGui::SameLine();

        // Setting an invisible label for the input field
        std::string emptylabel_str = "##";
        const char *emptylabel = emptylabel_str.c_str(); 
        
        // Adding an input field for changing bias value
        ImGui::PushItemWidth(120);
        int inputField_BiasValue = static_cast <int>(dac[i].data);
        inputField_BiasValue, valueChange_DACbias[i] = ImGui::InputInt(emptylabel, &inputField_BiasValue);
        dac[i].data = static_cast <std::uint16_t>(checkLimits(inputField_BiasValue, MAXIMUM_BIAS_VOLTAGE)); 
        ImGui::SameLine();

        // Including units
        ImGui::Text("mV");
        ImGui::SameLine();

        // Adding a update button to write to serial port
        if((ImGui::Button("Update", ImVec2(BUTTON_UPDATE_WIDTH, BUTTON_HEIGHT))) || updateValues)
        {
            dev.set_bias(dac[i])
        }

        ImGui::PopID();
    }


    // Loading saved DAC biases

    if (ImGui::Button("Load", ImVec2(ImGui::GetWindowSize().x*0.48, BUTTON_HEIGHT)))
    {
        ImGui::OpenPopup(popupLoad);
    }

    ImGui::SameLine();

    // Save new biases values

    if (ImGui::Button("Save", ImVec2(ImGui::GetWindowSize().x*0.48, BUTTON_HEIGHT)))
    {
        ImGui::OpenPopup(popupSave);
    }

    savePopup(openSavePopup, popupSave, dac);
    loadPopup(openLoadPopup, popupLoad, dac);

    ImGui::End();
    
}
   

