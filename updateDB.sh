#!/bin/bash

cd /nfs/site/disks/sc_aepdeD16/user/nfacciol/indexer
wait
g++ -o index index.cpp -lsqlite3 -std=c++0x
wait

######################################
# DO NOT MODIFY AS crtime is NOT SAVED 
# IN THIS VERSION OF UNIX CHANGING
# THIS CAN BREAK THE ENTIRE DATABASE
# CONTROL SYSTEM
#####################################
dateOfCreation=$(date +2022-07-13)
now=$(date '+%Y-%m-%d')

#get days since creation
daysSinceCreation=$((($(date +%s --date "$now")-$(date +%s --date "$dateOfCreation"))/(3600*24)))

if [ $(($daysSinceCreation%2)) -eq 0  ];
then
    echo indexing even
    ./concurrentExecution.sh
else
    echo indexing odd
    ./concurrentExecutionOdd.sh
fi