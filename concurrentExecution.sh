#!/bin/sh

cd /nfs/site/disks/sc_aepdeD16/user/nfacciol/indexer
wait
g++ -o index index.cpp -lsqlite3 -std=c++0x
wait

./index sc_aepdeD01 &
./index sc_aepdeD02 &
./index sc_aepdeD03 &
./index sc_aepdeD04 &
./index sc_aepdeD05 &
./index sc_aepdeD06 &
./index sc_aepdeD07 &
./index sc_aepdeD08 &
./index sc_aepdeD09 &
./index sc_aepdeD10 &
./index sc_aepdeD11 &
./index sc_aepdeD12 &
./index sc_aepdeD13 &
./index sc_aepdeD14 &
./index sc_aepdeD15 &
./index sc_aepdeD16 &