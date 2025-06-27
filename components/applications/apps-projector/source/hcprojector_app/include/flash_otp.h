#include <stdint.h> 
#include "app_config.h"
#ifdef HUDI_FLASH_SUPPORT
#ifdef CVBSIN_SUPPORT
uint8_t cvbs_training_value_get(void);
void cvbs_training_value_set(uint8_t val);
#endif
void flash_otp_data_saved(void);
void flash_otp_data_init(void);
#endif