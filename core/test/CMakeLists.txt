INCLUDE_DIRECTORIES(../include/)
INCLUDE_DIRECTORIES(${NTL_INLCUDE_PATH})
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
TARGET_LINK_LIBRARIES(test_contingency core-imp fhe crypto ${local-ntl} ${local-gmp} pthread docopt)


find_package(OpenSSL)
include_directories(${OPENSSL_INCLUDE_DIR}/)
ADD_EXECUTABLE(test_aes TestAES.cpp)
ADD_TEST(NAME TestAES
        COMMAND $<TARGET_FILE:test_aes>
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
TARGET_LINK_LIBRARIES(test_aes core-imp fhe crypto ${local-ntl} ${local-gmp} pthread)
