#include "utest_client.hpp"

namespace UTC {
    int test() {
        std::cout << "test." << std::endl;
        std::vector<std::string> test_data;
        test_data.push_back("ses_01");
        test_data.push_back("ana_01");
        test_data.push_back("PROT_CI2");
        test_data.push_back("schema.csv");
        test_data.push_back("u01");
        test_data.push_back("u02");
        FileSystemClient fsc_test;
        fsc_test.make_analyst_info(test_data);
        return 0;
    }
}

