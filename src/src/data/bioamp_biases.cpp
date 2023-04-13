#include "../include/bioamp_biases.h"

BioampBiases::BioampBiases(std::string filename) {
  std::ifstream file;
  char buf[256];
  unsigned int i;

  // Open file
  file.open(filename.c_str());
  if(!file.good()) {
      printf("Bias file '%s' not found\n",filename.c_str());
      exit(1);
  }
  // Discard header
  for (;file.peek() == '%'; file.getline(buf, sizeof(buf)));

  // Read biases
  for (i=0; i < N_BIASES; i++) {
    file.getline(buf, sizeof(buf));
    if (file.gcount() == 0)
      break;
    values[i] = atoi(buf);
  }

  // Close file
  file.close();

  // Verify if the file had enough data in it
  if (i < N_BIASES) {
    std::cout << "Error loading biases" << std::endl;
  }
}

BioampBiases::~BioampBiases() {
}

void BioampBiases::set(Bias bias, unsigned int value) {
  if(value > MAXIMUM_BIAS_VOLTAGE) {
    values[bias] = MAXIMUM_BIAS_VOLTAGE;
  }
  else {
    values[bias] = value;
  }
}

unsigned int BioampBiases::get(Bias bias) {
  return values[bias];
}

// See datasheet for data formatting
unsigned int BioampBiases::get_encoded(Bias bias) {
  
  int val = (BIT_PRECISION/MAXIMUM_BIAS_VOLTAGE) * (values[bias] + __OFFSET__);
  unsigned int encoded = (((unsigned int) 3 << 20 ) | ( (unsigned int)bias << 16 ) | val );
  return encoded;
}
