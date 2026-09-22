cmake_minimum_required(VERSION 3.16)

if (DEFINED ESP_PLATFORM)

    # ESP-IDF component
    set(AAT_REQUIRES
        esp_common
        freertos
        hal
        log
        nvs_flash
        driver
        esp_adc
        esp_http_client
        esp_driver_gpio
        esp_driver_ledc
        espressif__arduino-esp32
    )

    # esp_driver_i2s / esp_driver_dac needed starting IDF v6.0
    if(IDF_VERSION_MAJOR GREATER_EQUAL 6)
        list(APPEND AAT_REQUIRES
            esp_driver_i2s
            esp_driver_dac
        )
    endif()

    # Optional arduino-audio-driver component
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../arduino-audio-driver/CMakeLists.txt")
        list(APPEND AAT_REQUIRES arduino-audio-driver)
    endif()

    idf_component_register(
        INCLUDE_DIRS src
        REQUIRES ${AAT_REQUIRES}
    )

    # IMPORTANT:
    # Do NOT define ESP32_CMAKE here.
    # That makes AudioTools use its fake Arduino emulation layer.
    target_compile_options(${COMPONENT_LIB} INTERFACE
        -Wno-format
    )

else()

    project(arduino-audio-tools)

    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")
    set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

    include(FetchContent)

    add_library(arduino-audio-tools INTERFACE)

    target_compile_options(arduino-audio-tools INTERFACE
        -Wno-format
    )

    option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
    option(ADD_PORTAUDIO "Add Portaudio Library" OFF)
    option(ADD_ARDUINO_EMULATOR "Add Arduino Emulator Library" ON)

    target_include_directories(arduino-audio-tools INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )

    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/"
        DESTINATION "include/arduino-audio-tools"
        FILES_MATCHING
        PATTERN "*.h"
    )

    if (ADD_PORTAUDIO)
        FetchContent_Declare(
            portaudio
            GIT_REPOSITORY "https://github.com/PortAudio/portaudio.git"
            GIT_TAG v19.7.0
        )

        FetchContent_GetProperties(portaudio)

        if(NOT portaudio_POPULATED)
            FetchContent_Populate(portaudio)
            add_subdirectory(
                ${portaudio_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/portaudio
            )
        endif()
    endif()

    if (ADD_ARDUINO_EMULATOR)
        FetchContent_Declare(
            arduino_emulator
            GIT_REPOSITORY "https://github.com/pschatzmann/Arduino-Emulator.git"
            GIT_TAG main
        )

        FetchContent_GetProperties(arduino_emulator)

        if(NOT arduino_emulator_POPULATED)
            FetchContent_Populate(arduino_emulator)
            add_subdirectory(
                ${arduino_emulator_SOURCE_DIR}
                ${CMAKE_CURRENT_BINARY_DIR}/emulator
            )
        endif()
    endif()

endif()