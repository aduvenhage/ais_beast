PROJECT(ais_decoder)

SET(INCL_SRC
    aisutils.h
    chunk.h
    decoder.h
    mem_pool.h
    processing.h
    strutils.h
    queue.h
    tiff.h
)

SET(LIB_SRC
    ${INCL_SRC}
    aisutils.cpp
    decoder.cpp
    processing.cpp
)

set(targetname "ais_decoder")
ADD_LIBRARY(${targetname} STATIC ${LIB_SRC})
set_property(TARGET ${targetname} PROPERTY FOLDER libs)

