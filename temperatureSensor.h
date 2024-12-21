
//#include <driver/temp_sensor.h>

/*
temperature_sensor_handle_t temp_handle = NULL;
temperature_sensor_config_t temp_sensor = {
    .range_min = 20,
    .range_max = 50,
};
*/

void temp_init(){
  /*
  //ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor, &temp_handle));
  temp_sensor_config_t temp_sensor = TSENS_CONFIG_DEFAULT();
  temp_sensor.dac_offset = TSENS_DAC_L2;  // TSENS_DAC_L2 is default; L4(-40°C ~ 20°C), L2(-10°C ~ 80°C), L1(20°C ~ 100°C), L0(50°C ~ 125°C)
  temp_sensor_set_config(temp_sensor);
  temp_sensor_start();
  */
}

uint8_t lastTemperature;
void temp_sense(){
  lastTemperature = temperatureRead();
  //printf("Temperature %i\n", lastTemperature);
  //float result = 0;
  //temp_sensor_read_celsius(&result);
  /*
  // Enable temperature sensor
  ESP_ERROR_CHECK(temperature_sensor_enable(temp_handle));
  // Get converted sensor data
  float tsens_out;
  ESP_ERROR_CHECK(temperature_sensor_get_celsius(temp_handle, &tsens_out));
  printf("Temperature in %f °C\n", tsens_out);
  // Disable the temperature sensor if it is not needed and save the power
  ESP_ERROR_CHECK(temperature_sensor_disable(temp_handle));
  */
}