idf_component_register(
    SRCS "energy_meter.cpp"
    INCLUDE_DIRS "include"
    REQUIRES esp_common
    PRIV_REQUIRES esp_timer
)
