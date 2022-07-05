#!/bin/bash

cd /nfs/site/disks/sc_aepdeD02/user/nfacciol/indexer/indexer

g++ -o fresh_index fresh_index.cpp -lsqlite3 -std=c++0x

# ./fresh_index sc_aepdeD01 --async &
./fresh_index sc_aepdeD02 --async &
# ./fresh_index sc_aepdeD03 --async &
# ./fresh_index sc_aepdeD04 --async &
# ./fresh_index sc_aepdeD05 --async &
# ./fresh_index sc_aepdeD06 --async &
# ./fresh_index sc_aepdeD07 --async &
# ./fresh_index sc_aepdeD08 --async &
# ./fresh_index sc_aepdeD09 --async &
# ./fresh_index sc_aepdeD10 --async &
./fresh_index sc_aepdeD11 --async &
# ./fresh_index sc_aepdeD12 --async &
# ./fresh_index sc_aepdeD13 --async &
# ./fresh_index sc_aepdeD14 --async &
# ./fresh_index sc_aepdeD15 --async &