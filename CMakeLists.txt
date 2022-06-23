
cmake_minimum_required (VERSION 3.8)

set(projectname "idatrace2tree")
project (${projectname})

set(SOURCES
  src/idatrace2tree.h
  src/idatrace2tree.cpp
)

add_executable (${projectname} ${SOURCES})

target_compile_features(${projectname} PRIVATE cxx_std_17)
