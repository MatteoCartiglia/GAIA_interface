#ifndef ADC_CHANNEL_H
#define ADC_CHANNEL_H
class VisualizeAdcChannel{
    public:
        void ShowAdcChannelGraphs();
        void ShowAdcChannelSubplotconst(char* title, bool& show, float amplitude, float period, float offset, float &adc_value);
        void ShowAdcPlotControlMenuconst int n_adc_channels, const char* ADC_Names, bool show);
        void ShowDemo_Tables();
};
#endif