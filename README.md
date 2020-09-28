# ais_beast
Faster AIS decoder (multi-threaded and hopefully GPU enabled)
Ais reference: https://gpsd.gitlab.io/gpsd/AIVDM.html
Previous version of decoder: https://github.com/aduvenhage/ais-decoder

Features:
- Super fast C++ AIS decoding.
- Multi-stage and support for multi-threading and multi-processing


DONE:
- fixed decoding structures (fragments, messages, payloads)
- basic nmea sentence decoding
- crc checks
- payload de-armouring
- lock-free producer/consumer queue


TODO:
- overloaded memory operators for memory structures, so that we can reuse objects
- multi-sentence fragment decoding
- support cuda


