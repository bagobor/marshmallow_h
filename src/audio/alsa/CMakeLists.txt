find_package(ALSA)

if(NOT ALSA_FOUND)
	message(FATAL_ERROR "ALSA was not found!")
endif()

set(MARSHMALLOW_AUDIO_BACKEND_SRCS "backend.cpp")

set(MARSHMALLOW_AUDIO_BACKEND_LIBS ${ALSA_LIBRARY})

include_directories(${ALSA_INCLUDE_DIR})

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUXX)
	if(MARSHMALLOW_SHARED)
		set(CMAKE_CXX_FLAGS "-fPIC -DPIC ${CMAKE_CXX_FLAGS}")
		set(CMAKE_C_FLAGS "-fPIC -DPIC ${CMAKE_C_FLAGS}")
	endif()
endif()

################################################################################

add_library(marshmallow_audio_backend STATIC ${MARSHMALLOW_AUDIO_BACKEND_SRCS})

target_link_libraries(marshmallow_audio_backend ${MARSHMALLOW_AUDIO_BACKEND_LIBS})

