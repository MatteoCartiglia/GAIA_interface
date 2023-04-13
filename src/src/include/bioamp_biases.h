
#ifndef __BIOAMP_BIASES_H
#define __BIOAMP_BIASES_H

#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <stdlib.h>
#include <iostream>

// Number of biases
#define N_BIASES 16
#define N_DIG_CONTROL 7

#define MAXIMUM_BIAS_VOLTAGE 1800. // 1800mV max
#define __OFFSET__ 0 //mV
#define BIT_PRECISION  65535. // 2^16

class BioampBiases {
 public:
  // Enumeration describing the different biases, there should be N_BIASES + N_DIG_CONTROL
  // different values in here
  enum Bias {
    vref = 1,
    lp_v = 2,
    ibn = 3,
    tail_comp_dn = 8,
    tail_comp_up = 9,
    vthup = 10,
    vthdn = 11,
    ref_p = 12,
    tail_stage_2 = 13,
    ref_electrode = 14,
    vbn = 15,
    };


  enum DIG_BIAS_WRITE {
    w_C0 = 0x01,
    w_C1 = 0x02,
    w_C2 = 0x03,
    w_control_imp = 0x04,
    w_EN_G9 = 0x05,
    w_EN_G1 = 0x06,
    w_EN_G4096 = 0x07
  };

  enum DIG_BIAS_READ {
    r_C0 = 0x20,
    r_C1 = 0x21,
    r_C2 = 0x22,
    r_control_imp = 0x23,
    r_EN_G9 = 0x24,
    r_EN_G1 = 0x25,
    r_EN_G4096 = 0x26
  };


  BioampBiases(std::string filename);
  ~BioampBiases();

  void set(Bias bias, unsigned int value);

  unsigned int get(Bias bias);

  unsigned int get_encoded(Bias bias);

  // Protected Stuff
 protected:
  unsigned int values[N_BIASES + N_DIG_CONTROL];

};


#endif /* __BIOAMP_BIASES_H */
