# Try to find I2C library.
# It can be used as :
#  
# find_package(I2C REQUIRED)
# target_link_libraries(program I2C::device)
#

set(I2C_FOUND OFF)
set(I2C_DEVICE_FOUND OFF)

# avahi-common & avahi-client
find_library(I2C_DEVICE_LIBRARY NAMES i2c)
find_path(I2C_DEVICE_INCLUDE_DIRS i2c/smbus.h linux/i2c-dev.h)

if (I2C_DEVICE_LIBRARY AND I2C_DEVICE_INCLUDE_DIRS)
  set(I2C_FOUND ON)
  set(I2C_DEVICE_FOUND ON)

  if (NOT TARGET "I2C::device")

    add_library("I2C::device" UNKNOWN IMPORTED)
    set_target_properties("I2C::device"
      PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${I2C_DEVICE_INCLUDE_DIRS}"
      IMPORTED_LOCATION "${I2C_DEVICE_LIBRARY}"
    )

    get_filename_component(AVAHI_LIB_DIR "${I2C_DEVICE_LIBRARY}" DIRECTORY CACHE)

  endif()

endif()

set(_I2C_MISSING_COMPONENTS "")
foreach(COMPONENT ${I2C_FIND_COMPONENTS})
  string(TOUPPER ${COMPONENT} COMPONENT)

  if(NOT I2C_${COMPONENT}_FOUND)
    string(TOLOWER ${COMPONENT} COMPONENT)
    list(APPEND _I2C_MISSING_COMPONENTS ${COMPONENT})
  endif()
endforeach()

if (_I2C_MISSING_COMPONENTS)
  set(I2C_FOUND OFF)

  if (I2C_FIND_REQUIRED)
    message(SEND_ERROR "Unable to find the requested I2C libraries.\n")
  else()
    message(STATUS "Unable to find the requested but not required I2C libraries.\n")
  endif()

endif()


if(I2C_FOUND)
  set(I2C_LIBRARIES ${I2C_DEVICE_LIBRARY})
  set(I2C_INCLUDE_DIRS ${I2C_DEVICE_INCLUDE_DIRS})
  
  message(STATUS "Found I2C: ${I2C_LIBRARIES}")
endif()
