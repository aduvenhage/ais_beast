PROJECT(ais_reader)

# linker flags
LINK_DIRECTORIES(${PATH_LIB_OUTPUT} /usr/local/lib /opt/local/lib)

# source files
SET(APP_SRC
	main.cpp
)

# linker settings
set(targetname "ais_reader")
ADD_EXECUTABLE(${targetname} ${APP_SRC})

IF(MAC)
        TARGET_LINK_LIBRARIES(${targetname} "-framework CoreFoundation -framework Foundation")
        TARGET_LINK_LIBRARIES(${targetname} "-Wl,-export_dynamic,-force_flat_namespace,-F/Library/Frameworks")
		TARGET_LINK_LIBRARIES(${targetname} ais_decoder)
        TARGET_LINK_LIBRARIES(${targetname} "-stdlib=libc++")

ENDIF(MAC)

