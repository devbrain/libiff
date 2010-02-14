cmake_minimum_required(VERSION 2.6)

option (optHAS_OPTIMIZED "Turn Optimizations ON" OFF)
option (optHAS_SYMBOLS   "Build with debug Symbols" ON)

if (optHAS_OPTIMIZED)
  if (optHAS_SYMBOLS)
    set (CMAKE_BUILD_TYPE relwithdebinfo)
  else (optHAS_SYMBOLS)
    set (CMAKE_BUILD_TYPE release)
  endif (optHAS_SYMBOLS)
else (optHAS_OPTIMIZED)
  if (optHAS_SYMBOLS)
    set (CMAKE_BUILD_TYPE Debug)
  else (optHAS_SYMBOLS)
    set (CMAKE_BUILD_TYPE Debug)
  endif (optHAS_SYMBOLS)
endif (optHAS_OPTIMIZED)



set (BK_SRC_ROOT src)

set (myQualification "${CMAKE_SYSTEM_NAME}.${CMAKE_BUILD_TYPE}")

set (myLIB_OUTPUT "${CMAKE_SOURCE_DIR}/lib/${myQualification}" )
set (myBIN_OUTPUT "${CMAKE_SOURCE_DIR}/bin/${myQualification}")
set (myOBJ_OUTPUT obj)

set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${myLIB_OUTPUT})
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${myLIB_OUTPUT})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${myBIN_OUTPUT})


include_directories (${BK_SRC_ROOT} ${BK_INCLUDE_PATH})

if (${CMAKE_SYSTEM_NAME} STREQUAL Linux)
set (TE_SYS_LIBS pthread dl)
else ()
set (TE_SYS_LIBS)
endif ()

set (build_modules 
core
test
)

foreach (mdl ${build_modules})
  set (MDL_SRC "${BK_SRC_ROOT}/${mdl}")
  set (MDL_OBJ "${myOBJ_OUTPUT}/${myQualification}/${mdl}")
  add_subdirectory (${MDL_SRC} ${MDL_OBJ})
endforeach ()
