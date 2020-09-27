PROJECT(ais_decoder)

SET(INCL_SRC
    chunk.h
    decoder.h
    mem_pool.h
    processing.h
    strutils.h
    queue.h
)

SET(LIB_SRC
    ${INCL_SRC}
    decoder.cpp
    processing.cpp
)

set(targetname "ais_decoder")
ADD_LIBRARY(${targetname} STATIC ${LIB_SRC})
set_property(TARGET ${targetname} PROPERTY FOLDER libs)

