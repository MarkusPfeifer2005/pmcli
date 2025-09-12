g++ -std=c++17 cli.cpp str_lib.cpp -lpqxx -lpq -o build/pmcli
if [ $? == 0 ]; then
    clear
    ./build/pmcli
fi
