cmake .
make
valgrind --leak-check=full  ./yjson_test

