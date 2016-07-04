INCLUDE_DIRECTORIES(../include)
ADD_EXECUTABLE(test_gt TestGreaterThan.cpp)
ADD_TEST(NAME TestGreaterThan
         COMMAND $<TARGET_FILE:test_gt>
         WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
TARGET_LINK_LIBRARIES(test_gt core-imp fhe ${local-ntl} ${local-gmp} pthread docopt)
