PROJECT(ais_reader)


INCLUDE_DIRECTORIES(${BEAST_INCLUDE_DIRS})
LINK_DIRECTORIES(${BEAST_LIB_DIRS})

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
		TARGET_LINK_LIBRARIES(${targetname} ais_decoder -ltiff)
        TARGET_LINK_LIBRARIES(${targetname} "-stdlib=libc++")

ENDIF(MAC)

