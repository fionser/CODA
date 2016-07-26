INCLUDE_DIRECTORIES(../include/)
ADD_EXECUTABLE(test_gt TestGreaterThan.cpp)
ADD_TEST(NAME TestGreaterThan
         COMMAND $<TARGET_FILE:test_gt>
         WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
TARGET_LINK_LIBRARIES(test_gt core-imp fhe ${local-ntl} ${local-gmp} pthread docopt)

ADD_EXECUTABLE(test_repeat TestRepeat.cpp)
ADD_TEST(NAME TestRepeat
         COMMAND $<TARGET_FILE:test_repeat>
         WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
TARGET_LINK_LIBRARIES(test_repeat core-imp fhe ${local-ntl} ${local-gmp} pthread docopt)

ADD_EXECUTABLE(test_contingency TestContingencyTable.cpp)
ADD_TEST(NAME TestContingency
        COMMAND $<TARGET_FILE:test_contingency>
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
TARGET_LINK_LIBRARIES(test_contingency core-imp fhe ${local-ntl} ${local-gmp} pthread docopt)
