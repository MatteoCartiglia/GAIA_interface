#ifndef __DEV_H
#define __DEV_H

#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include "boost/thread.hpp"
#include <vector>
#include <thread>
#include <queue>
#include <sys/stat.h>

#include "events.h"
#include "bioamp_biases.h"
#include "okFrontPanelDLL.h"
#include "okFrontPanel.h"





// Read and write digital endpoints 
#define READ_C0       0x20
#define READ_C1       0x21
#define READ_C2       0x22
#define READ_control_imp 0x23
#define READ_EN_G9       0x24
#define READ_EN_G1       0x25
#define READ_EN_G4096    0x26


#define WRITE_C0      0x01
#define WRITE_C1      0x02
#define WRITE_C2      0x03
#define WRITE_control_imp 0x04
#define WRITE_EN_G9      0x05
#define WRITE_EN_G1      0x06
#define WRITE_EN_G4096   0x07

#define PRst 0x08
#define SRst 0x09


#define BLOCK_SIZE      1024//2048              // bytes
#define OF_CODE         0xFFFF          // Overflow code
#define LEN_FIFO        65000


#define READ_ADDR       0xA3  // Address for reading from AER device
#define READ_fifo_empty 0x2A
#define READ_fifo_full  0x2B
#define READ_fifo_count 0x2C
#define READ_ADDR_ADC   0xa4  // Address for reading from ADC device

#define READ_DELAY 1 // period of device's pooling

// DAC Specific commands
#define PWRUP 0x480000   // Soft power up
#define PWRUP_1 0x400000   // soft power down
#define SOFT_RST 0X600000     // Soft reset

class 
Dev
{
public:

    Dev(std::string config_filename);
    ~Dev();

  bool is_opened;
  bool exitOnError(okCFrontPanel::ErrorCode error);
  bool read();
  //bool read(std::queue<Event2d>* ev_buffer);
  void reset();
 // void listen(std::queue<Event2d>* ev_buffer);
  //void join_thread(std::queue<Event2d>* ev_buffer);
  void set_biases(BioampBiases *biases);
  void set_individual_bias(BioampBiases *biases, BioampBiases::Bias, int value);

  bool read(moodycamel::ConcurrentQueue<Event2d>* ev_buffer);
  void listen(moodycamel::ConcurrentQueue<Event2d>* ev_buffer);
  void join_thread(moodycamel::ConcurrentQueue<Event2d>* ev_buffer);

  void set_wire(unsigned int pin_in, bool val);
  int read_wire(unsigned int pin_read);
  void toggle_wire(int pin_read, int pin_write);

  bool ok();
  bool is_listening;
  unsigned long long event_counter;
  unsigned long long of_counter;
  uint64_t data_count;
  unsigned char g_rbuf[BLOCK_SIZE];
  std::mutex ev_buffer_lock;


private:

  okCFrontPanel *dev;
  okTDeviceInfo *g_devInfo;
  std::thread listener;
  unsigned long long time_ref;

  void initializeFPGA(std::string config_filename);
  void init_DACs();
  void dac_write(unsigned int val);

};

#endif /* DEV */

