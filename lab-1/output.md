```sh
cyber-hpc@cclab105:~/Documents/hpc-lab/lab-1$ tree
.
├── CMakeLists.txt
├── include
│   ├── sort.hpp
│   └── timeit.hpp
└── src
    ├── bogosort.cpp
    ├── main.cpp
    ├── sleep_sort.cpp
    ├── stalin_sort.cpp
    └── timeit.cpp

3 directories, 8 files
cyber-hpc@cclab105:~/Documents/hpc-lab/lab-1$ cmake .
-- The CXX compiler identification is GNU 13.3.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (0.2s)
-- Generating done (0.0s)
-- Build files have been written to: /home/cyber-hpc/Documents/hpc-lab/lab-1
cyber-hpc@cclab105:~/Documents/hpc-lab/lab-1$ make
[ 16%] Building CXX object CMakeFiles/main.dir/src/main.cpp.o
[ 33%] Building CXX object CMakeFiles/main.dir/src/timeit.cpp.o
[ 50%] Building CXX object CMakeFiles/main.dir/src/stalin_sort.cpp.o
[ 66%] Building CXX object CMakeFiles/main.dir/src/bogosort.cpp.o
[ 83%] Building CXX object CMakeFiles/main.dir/src/sleep_sort.cpp.o
[100%] Linking CXX executable main
[100%] Built target main
cyber-hpc@cclab105:~/Documents/hpc-lab/lab-1$ ./main
Vec size: 10
Runs: 5

bogosort
Avg time: 208804270 ns

stalin_sort
Avg time: 107 ns

sleep_sort
Avg time: 911091259 ns
```