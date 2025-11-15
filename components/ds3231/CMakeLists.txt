require_idf_targets()

idf_component_register(SRCS "ds3231-module.c"
        INCLUDE_DIRS "include"
        REQUIRES driver)