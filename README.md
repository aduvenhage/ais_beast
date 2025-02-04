# ais_beast
Next (Experimental) version of AIS decoder (multi-threaded and hopefully GPU enabled)
- Previous version of decoder: https://github.com/aduvenhage/ais-decoder
  (much more mature version with python bindings)
- Ais reference: https://gpsd.gitlab.io/gpsd/AIVDM.html

While the previous version focused on extensibility and also on support for different formats, the intent with this verion is to focus on speed.
The decoder is broken into stages:
- input (nmea raw sentence decoding)
- fragment CRC checks and multi-line support
- payload de-armouring
- message decoding

Features:
- each stage is connected to the next through a thread safe queue
- stages can essentially run in parallel
- each stage also consumes and produces data in chunks (groups of messages)
- WIP support for processing on GPU

The goal is to get to 20M+ messages per second (NMEA processed from disk, decoded and saved back to disk in a application/binary format).

Current stats:
- Debug build: 5M+ messages per second
- Release build: 17M+ messages per second

DONE:
- decoding structures (fragments, messages, payloads)
- basic nmea sentence decoding
- crc checks
- payload de-armouring
- lock-free producer/consumer queue
- overloaded memory operators for memory structures, so that we can reuse objects
- multi-sentence fragment decoding
- replace lock-free queue with a locking and blocking queue (using condition variables)

TODO:
- support cuda


