# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/Embedded/ESP/esp/v5.3/esp-idf/components/bootloader/subproject"
  "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader"
  "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader-prefix"
  "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader-prefix/tmp"
  "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader-prefix/src"
  "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Embedded/ESP/project/012bluetooth_wifi/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
