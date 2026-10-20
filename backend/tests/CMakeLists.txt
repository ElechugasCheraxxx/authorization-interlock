# Backend Tests
cmake_minimum_required(VERSION 3.24)

add_executable(core_tests core_tests.cpp)
target_link_libraries(core_tests PRIVATE interlock_core)

add_executable(benchmark_rdt benchmark_rdt.cpp)
target_link_libraries(benchmark_rdt PRIVATE interlock_core)

add_executable(test_data_surgery_stress test_data_surgery_stress.cpp)
target_link_libraries(test_data_surgery_stress PRIVATE interlock_core)

add_executable(formal_pbt_tests formal_pbt_tests.cpp)
target_link_libraries(formal_pbt_tests PRIVATE interlock_core)

add_executable(math_compression_test math_compression_test.cpp)
target_link_libraries(math_compression_test PRIVATE interlock_core)

add_executable(pool_stress_test pool_stress_test.cpp)
target_link_libraries(pool_stress_test PRIVATE interlock_core)

enable_testing()
add_test(NAME CoreTests COMMAND core_tests)
add_test(NAME MathCompressionTest COMMAND math_compression_test)
add_test(NAME DataSurgeryStress COMMAND test_data_surgery_stress)
add_test(NAME PoolStressTest COMMAND pool_stress_test)
add_test(NAME FormalPBT COMMAND formal_pbt_tests)
