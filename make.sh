g++ -o index index.cpp -lsqlite3 -std=c++0x
wait
g++ -o indexer indexer.cpp cmdparse.cpp -lsqlite3 -std=c++0x
echo All files compiled