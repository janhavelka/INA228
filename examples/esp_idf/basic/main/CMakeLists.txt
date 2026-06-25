idf_component_register(
  SRCS "main.cpp" "Ina228IdfI2cTransport.cpp"
  INCLUDE_DIRS "."
  REQUIRES INA228 esp_driver_i2c esp_driver_gpio esp_timer freertos
)

target_compile_features(${COMPONENT_LIB} PUBLIC cxx_std_17)
