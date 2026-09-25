cmake_minimum_required(VERSION 3.10)
project(EtherCatServoProject)

# Tentukan standar C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Daftarkan folder include utama
include_directories(${CMAKE_SOURCE_DIR}/include)

# (Opsional tapi aman) Tambahkan sub-direktori osal dan oshw agar compiler tidak meleset
include_directories(${CMAKE_SOURCE_DIR}/include/osal)
include_directories(${CMAKE_SOURCE_DIR}/include/oshw)

# Gunakan paket SOEM yang dipasang lokal, atau lokasi dari -Dsoem_DIR.
find_package(soem CONFIG REQUIRED
    HINTS "${CMAKE_CURRENT_SOURCE_DIR}/.deps/soem/cmake")

# ── Executable 1: ethercat_servo (main program) ──
add_executable(ethercat_servo src/main.cpp src/pdo_check.cpp src/sdo_trace.cpp src/lc10e_esi.cpp src/brake_confirm.cpp src/motion_trial.cpp)

# Hubungkan (link) library soem serta library pendukung Linux (pthread, rt)
target_link_libraries(ethercat_servo soem pthread rt)
target_link_libraries(ethercat_servo "-Wl,--wrap=ecx_mbxsend" "-Wl,--wrap=ecx_mbxreceive"
    "-Wl,--wrap=ecx_readPDOmap" "-Wl,--wrap=ecx_readPDOmapCA")

# ── Executable 2: diagnostic (diagnostic tool) ──
add_executable(diagnostic src/diagnostic.cpp src/brake_confirm.cpp)

# Link library yang sama
target_link_libraries(diagnostic soem pthread rt)
