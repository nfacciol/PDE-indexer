#!/bin/sh

cd /nfs/site/disks/sc_aepdeD16/user/nfacciol/indexer
wait

timeout 23h ./index sc_aepdeD01odd &
timeout 23h ./index sc_aepdeD02odd &
timeout 23h ./index sc_aepdeD03odd &
timeout 23h ./index sc_aepdeD04odd &
timeout 23h ./index sc_aepdeD05odd &
timeout 23h ./index sc_aepdeD06odd &
timeout 23h ./index sc_aepdeD07odd &
timeout 23h ./index sc_aepdeD08odd &
timeout 23h ./index sc_aepdeD09odd &
timeout 23h ./index sc_aepdeD10odd &
timeout 23h ./index sc_aepdeD11odd &
timeout 23h ./index sc_aepdeD12odd &
timeout 23h ./index sc_aepdeD13odd &
timeout 23h ./index sc_aepdeD14odd &
timeout 23h ./index sc_aepdeD15odd &
timeout 23h. /index sc_aepdeD16odd &
timeout 23h. /index sc_aepdeD17odd &