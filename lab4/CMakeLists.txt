# CMakeList.txt: projekt CMake dla elementu lab2, dołącz źródło i zdefiniuj
# tutaj logikę specyficzną dla projektu.
#
cmake_minimum_required (VERSION 3.8)

# Dodaj źródło do pliku wykonywalnego tego projektu.
add_executable (lab4 "lab4.cpp" "lab4.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET lab4 PROPERTY CXX_STANDARD 20)
endif()

# Szukanie biblioteki Direct2D (d2d1.lib)
find_library(DIRECT2D d2d1)
if (NOT DIRECT2D)
    message(FATAL_ERROR "Could not find Direct2D.")
endif ()

# Dołączenie biblioteki Direct2D
target_link_libraries(lab4 ${DIRECT2D})


# TODO: Dodaj testy i zainstaluj elementy docelowe w razie potrzeby.
