file(GLOB_RECURSE TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cpp)

# Iterate over all tests found. For each, declare an executable and add it to the tests list.
set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
    string(REGEX REPLACE "[./]" "_" NAME ${FILE}) # create exe name
    add_executable(${NAME} ${FILE})
    target_link_libraries(${NAME} core-imp pthread docopt fhe ${local-ntl} ${local-gmp} cryptopp) # link against our lib
    add_test(NAME ${NAME}
             COMMAND ${NAME})
             # WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
endforeach(FILE ${TEST_FILES})
include(CTest)
