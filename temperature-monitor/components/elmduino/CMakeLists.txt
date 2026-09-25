idf_component_register(
  SRCS "src/ELMduino.cpp"
  INCLUDE_DIRS "src"
  PRIV_REQUIRES espressif__arduino-esp32
)

target_compile_options(${COMPONENT_LIB} PRIVATE
  -Wno-error=format-truncation
)