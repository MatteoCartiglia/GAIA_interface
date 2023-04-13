#ifndef DATA_LOADER_H
#define DATA_LOADER_H

class DataLoader{
    public:
        struct BiasData;
        bool loadBiases(char *filename);
        bool loadBiasesOld(char *filename);
        bool saveBiases(char *filename);
        void showSaveLoadBiases();
        BiasData ShowBiasControlCenter(bool* bias_control_open);
};
#endif