#include "../include/dev.h"
#include <chrono>
#include <thread>
#include "../concurrentqueue.h"


Dev::Dev(std::string config_filename) {
  char dll_date[32], dll_time[32];
  printf("---- GAIA application v1.0 ----\n");
  if (FALSE == okFrontPanelDLL_LoadLib(NULL)) {
    printf("FrontPanel DLL could not be loaded.\n");
  }
  okFrontPanelDLL_GetVersion(dll_date, dll_time);
  printf("FrontPanel DLL loaded.  Built: %s  %s\n", dll_date, dll_time);

  is_listening = true;
  event_counter = 0;
  of_counter = 0;
  time_ref = 0;


  struct stat buffer;
  if(stat (config_filename.c_str(), &buffer) != 0) {
      printf("ERROR : Bitfile '%s' not found!\n",config_filename.c_str());
      exit(2);
  }
  // Configure FPGA
  initializeFPGA(config_filename);

  // Configure DACs
  if (dev != NULL){
    init_DACs();
    reset();
  }

  // Check FPGA configuration
  if (NULL == dev) {
    printf("\nFPGA could not be initialized.\n\n");
    is_opened = false;
  }
  else {
    printf("\nFPGA initialized.\n\n");
    is_opened = true;
  }
}



Dev::~Dev() {

  std::cout << "Terminate poolling thread" << std::endl;
  // Detach from thread
  listener.detach();

  delete g_devInfo;
  delete dev;
  delete g_devInfo;
  delete dev;
  printf("All done, Bye Bye.\n");
}

void Dev::initializeFPGA(std::string config_filename) {
    // Open the first XEM
    dev = new okCFrontPanel;

    printf("serial: %d \n", dev->OpenBySerial());

    if (okCFrontPanel::NoError != dev->OpenBySerial()) {
        printf("Device could not be opened. Is one connected?\n");
        dev = NULL;
        // exit(1);
        return ;
    }
    printf("Found a device: %s (%s)\n", dev->GetBoardModelString(dev->GetBoardModel()).c_str(),dev->GetDeviceID().c_str());
    g_devInfo = new okTDeviceInfo;
    dev->GetDeviceInfo(g_devInfo);
    if(dev->GetBoardModel() != okCFrontPanel::brdXEM7310A75) {

        printf("Unsupported device.\n");
        dev = NULL;
        exit(1);
        return ;
    }
    // Configure the PLL appropriately
    dev->LoadDefaultPLLConfiguration();

    // Get some general information about the XEM.
    printf("Device firmware version: %d.%d\n", dev->GetDeviceMajorVersion(), dev->GetDeviceMinorVersion());
    printf("Device serial number: %s\n", dev->GetSerialNumber().c_str());

    if (okCFrontPanel::NoError != dev->ConfigureFPGA(config_filename)) {
        printf("FPGA configuration failed.\n");
        dev = NULL;
        exit(1);
        return;
    }

    // Check for FrontPanel support in the FPGA configuration.
    if (false == dev->IsFrontPanelEnabled()) {
        printf("FrontPanel support is not enabled.\n");
        dev = NULL;
        exit(1);
        return;
    }
}

void Dev::reset() {
  time_ref = 0;

//PRst and SRst low and reset high

  dev->SetWireInValue(0x08, 0x000);
  dev->UpdateWireIns();
  dev->SetWireInValue(0x09, 0x000);
  dev->UpdateWireIns();

  dev->SetWireInValue(0x000, 0x0001);
  dev->UpdateWireIns();
  std::this_thread::sleep_for(std::chrono::nanoseconds(100));

// init DAC
  dev->ActivateTriggerIn(0x40, 0);
// activate DAC trigger
  dev->ActivateTriggerIn(0x40, 1);
// activate ADC trigger
  dev->ActivateTriggerIn(0x40, 2);
  // Reset AER FIFOs
  dev->ActivateTriggerIn(0x40, 3);

  //Reset Low and PRst and SRst high
  dev->SetWireInValue(0x000, 0x0000);
  dev->UpdateWireIns();
  dev->SetWireInValue(0x08, 0x001);
  dev->UpdateWireIns();
  dev->SetWireInValue(0x09, 0x001);
  dev->UpdateWireIns();

}

// Exits on critical failure
// Returns true if there is no error
// Returns false on non-critical error
bool Dev::exitOnError(okCFrontPanel::ErrorCode error)
{
	switch (error) {
	case okCFrontPanel::DeviceNotOpen:
		printf("Device no longer available.\n");
		exit(EXIT_FAILURE);
	case okCFrontPanel::Failed:
		printf("Transfer failed.\n");
		exit(EXIT_FAILURE);
	case okCFrontPanel::Timeout:
		printf("   ERROR: Timeout\n");
		return false;
	case okCFrontPanel::TransferError:
		std::cout << "   ERROR: TransferError" << '\n';
		return false;
	case okCFrontPanel::UnsupportedFeature:
		printf("   ERROR: UnsupportedFeature\n");
		return false;
	case okCFrontPanel::DoneNotHigh:
	case okCFrontPanel::CommunicationError:
	case okCFrontPanel::InvalidBitstream:
	case okCFrontPanel::FileError:
	case okCFrontPanel::InvalidEndpoint:
	case okCFrontPanel::InvalidBlockSize:
	case okCFrontPanel::I2CRestrictedAddress:
	case okCFrontPanel::I2CBitError:
	case okCFrontPanel::I2CUnknownStatus:
	case okCFrontPanel::I2CNack:
	case okCFrontPanel::FIFOUnderflow:
	case okCFrontPanel::FIFOOverflow:
	case okCFrontPanel::DataAlignmentError:
	case okCFrontPanel::InvalidParameter:
		std::cout << "   ERROR: " << error << '\n';
		return false;
	default:
		return true;
	}
}


bool Dev::read() {
  read(NULL);
  return(false);
}


//bool Dev::read(std::queue<Event2d>* ev_buffer)
bool Dev::read(moodycamel::ConcurrentQueue<Event2d>* ev_buffer) 

{
    long ret;
    dev->UpdateWireOuts();
    data_count = dev->GetWireOutValue(READ_fifo_count);
    //std::cout << "data_count: "<< data_count <<std::endl;
    uint32_t fifo_full = dev->GetWireOutValue(READ_fifo_full);

  /*if (false == exitOnError((okCFrontPanel::ErrorCode)ret))
  {
    std::cout << "Error" << std::endl;
  }*/

  if (fifo_full == 1)
  {
    data_count = LEN_FIFO;
    std::cout << "Fifo full" <<std::endl;
  }
  if (data_count < BLOCK_SIZE/8) {
    return(true);
  }

  for (unsigned int k = 0 ; k < (data_count*4)/BLOCK_SIZE ; k++) {

       ret = dev->ReadFromBlockPipeOut(READ_ADDR, BLOCK_SIZE, BLOCK_SIZE, &g_rbuf[0]);
       //std::cout << "ret: "<< ret << std::endl;

       if (false == exitOnError((okCFrontPanel::ErrorCode) ret)) {
           std::cout << "Error" << std::endl;
       }

       for (unsigned int i = 0; i < (int)(BLOCK_SIZE / 4); i++) {

          uint32_t event_data;
          memcpy(&event_data, &g_rbuf[4 * i], sizeof(uint32_t));
          unsigned short timestamp = (event_data >> 16) & 0xFFFF;
          unsigned short data = event_data & 0xFFFF;
            
          // unsigned short timestamp = g_rbuf[4 * i + 2] + (g_rbuf[4 * i + 3] << 8);
          // unsigned short data = g_rbuf[4 * i] + (g_rbuf[4 * i + 1] << 8);
         //  std::cout << "ts: " << timestamp << std::endl;

           if (data == OF_CODE && timestamp == OF_CODE) {
               of_counter++;
               time_ref += OF_CODE;
           }
           else {
               Event2d ev;
               EventRaw tt;
               event_counter++;
               int data_old, ts_old;
                 switch (data)
                {
                case (0xC000):
                    ev = Event2d(time_ref + timestamp, 74, 67, 0);
                    break;
                case (0xC001):
                    ev = Event2d(time_ref + timestamp, 67, 74, 1);
                    break;
                case (0xC002):
                    ev = Event2d(time_ref + timestamp, 70, 70, 0);
                    break;
                case (0xC003):
                    ev = Event2d(time_ref + timestamp, 70, 71, 0);
                    break;
                case (0xC004):
                    ev = Event2d(time_ref + timestamp, 70, 72, 0);
                    break;
                case (0xC005):
                    ev = Event2d(time_ref + timestamp, 71, 70, 0);
                    break;
                case (0xC006):
                    ev = Event2d(time_ref + timestamp, 71, 71, 0);
                    break;
                case (0xC007):
                    ev = Event2d(time_ref + timestamp, 71, 72, 0);
                    break;
                case (0xC008):
                    ev = Event2d(time_ref + timestamp, 72, 70, 0);
                    break;
                case (0xC009):
                    ev = Event2d(time_ref + timestamp, 72, 71, 0);
                    break;
                case (0xC00A):
                    ev = Event2d(time_ref + timestamp, 72, 72, 0);
                    break;
                default:
                    unsigned int x = (data >> address_shift) & address_mask;
                    unsigned int y = (data) & address_mask;
                    unsigned int p = (data >> pol_shift) & pol_mask;
                    ev = Event2d(time_ref + timestamp, x, y, p);
                }

            data_old = data;
            ts_old = timestamp;
           // std::cout << " x,y:  " <<ev.x << " " << ev.y << " ts: " <<ev.ts <<std::endl;
           event_counter++;
           //ev_buffer_lock.lock();
           //ev_buffer->push(ev);
           //ev_buffer_lock.unlock();
           ev_buffer->enqueue(ev);

           }

       }
   }
   if (false == dev->IsOpen()){
     exitOnError(okCFrontPanel::DeviceNotOpen);
   }
   return(false);

}

void Dev::listen(moodycamel::ConcurrentQueue<Event2d>* ev_buffer) {
    while (is_listening) {
        read(ev_buffer);
        std::this_thread::sleep_for(std::chrono::milliseconds(READ_DELAY));
    }
}

/*void Dev::listen(std::queue<Event2d>* ev_buffer) {
  while(is_listening) {
    read(ev_buffer);
   // std::this_thread::sleep_for(std::chrono::milliseconds(READ_DELAY));
  }
}

void Dev::join_thread(std::queue<Event2d>* ev_buffer) {
  is_listening = true;
  listener = std::thread([this, ev_buffer]{listen(ev_buffer);});
}
*/
void Dev::join_thread(moodycamel::ConcurrentQueue<Event2d>* ev_buffer) {
    is_listening = true;
    listener = std::thread([this, ev_buffer] { listen(ev_buffer); });
}

bool Dev::ok() {
    return dev->IsOpen();
}




// ********************************
// ***** BIASES GENERATION ********
// ********************************

void Dev::set_individual_bias(BioampBiases *biases, BioampBiases::Bias bias, int value){
  printf("== Configuring Biases ==\n");
  biases->set((BioampBiases::Bias)bias, value);
  unsigned int encoded = biases->get_encoded((BioampBiases::Bias)bias);
  std::cout << encoded << std::endl;
  unsigned int val = biases->get(bias);
  printf("Set bias #%d to %d mV",val,encoded);
  dac_write(encoded);

}

void Dev::set_biases(BioampBiases *biases) {
  printf("== Configuring Biases ==\n");
  for (unsigned int i=0; i < N_BIASES; i++)
  {
    unsigned int encoded = biases->get_encoded((BioampBiases::Bias)i);
    unsigned int val = biases->get((BioampBiases::Bias)i);
    printf("Set bias #%d to %d mV [%06X]\n",i,val,encoded);
    dac_write(encoded);
  }
}

void Dev::init_DACs() {

    dev->ActivateTriggerIn(0x40, 0);
    dac_write(SOFT_RST);
    usleep(1500);
    printf("\tpower up\n");
    dac_write(PWRUP);
    usleep(1500);
    dac_write(PWRUP_1);
    usleep(1500);

}

void Dev::dac_write(unsigned int val) {
  // Update WireIn values DAC_IN endpoint 0x0C

  dev->SetWireInValue(0x0C, val );
  dev->UpdateWireIns();
  // Activate Trigger to notify FPGA of new values
  dev->ActivateTriggerIn(0x40, 1);

}

int Dev::read_wire(unsigned int pin_read){
  dev->UpdateWireOuts();
  return dev->GetWireOutValue (pin_read);
}

void Dev::set_wire(unsigned int pin_in, bool val)
{
  dev->SetWireInValue(pin_in, (int) val);
  dev->UpdateWireIns();
}

void Dev::toggle_wire(int pin_read, int pin_write){
  bool val = read_wire(pin_read);
  set_wire(pin_write, !val);
  std::cout << "Digital control set from " << val << " to " << !val << std::endl;
}

