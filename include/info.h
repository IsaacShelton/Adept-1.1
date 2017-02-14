
#ifndef INFO_H_INCLUDED
#define INFO_H_INCLUDED

// Defines macros depending on the build type

// ADEPT_BUILD_TYPE - A string containing the name of the build type

#ifdef ADEPT_DEBUG_32
  #define ADEPT_BUILD_NAME "Debug 32-bit"
#endif // ADEPT_DEBUG_32

#ifdef ADEPT_RELEASE_32
  #define ADEPT_BUILD_NAME "Release 32-bit"
#endif // ADEPT_RELEASE_32

#ifdef ADEPT_DEBUG_64
  #define ADEPT_BUILD_NAME "Debug 64-bit"
#endif // ADEPT_DEBUG_64

#ifdef ADEPT_RELEASE_64
  #define ADEPT_BUILD_NAME "Release 64-bit"
#endif // ADEPT_RELEASE_64

#endif // INFO_H_INCLUDED
