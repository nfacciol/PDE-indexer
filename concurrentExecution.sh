#!/bin/sh

cd /nfs/site/disks/sc_aepdeD16/user/nfacciol/indexer
wait

timeout 23h ./index sc_aepdeD01 &
timeout 23h ./index sc_aepdeD02 &
timeout 23h ./index sc_aepdeD03 &
timeout 23h ./index sc_aepdeD04 &
timeout 23h ./index sc_aepdeD05 &
timeout 23h ./index sc_aepdeD06 &
timeout 23h ./index sc_aepdeD07 &
timeout 23h ./index sc_aepdeD08 &
timeout 23h ./index sc_aepdeD09 &
timeout 23h ./index sc_aepdeD10 &
timeout 23h ./index sc_aepdeD11 &
timeout 23h ./index sc_aepdeD12 &
timeout 23h ./index sc_aepdeD13 &
timeout 23h ./index sc_aepdeD14 &
timeout 23h ./index sc_aepdeD15 &
timeout 23h ./index sc_aepdeD16 &
timeout 23h ./index sc_aepdeD17 &