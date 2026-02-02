# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "qwt/CMakeFiles/qwt_autogen.dir/AutogenUsed.txt"
  "qwt/CMakeFiles/qwt_autogen.dir/ParseCache.txt"
  "qwt/qwt_autogen"
  "src/CMakeFiles/GoldenCheetah_autogen.dir/AutogenUsed.txt"
  "src/CMakeFiles/GoldenCheetah_autogen.dir/ParseCache.txt"
  "src/GoldenCheetah_autogen"
  )
endif()
