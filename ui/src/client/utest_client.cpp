#include "utest_client.hpp"


namespace UTC {
    int test(int argc, char *argv[]) {
        int flg = -1;
        std::cout << "--------------------------------------------------------------" <<  std::endl;
        std::cout << "test file system : start"  << std::endl;
        std::cout << "--------------------------------------------------------------" <<  std::endl;
        flg = test_filesystem_1();
        std::cout << "--------------------------------------------------------------" <<  std::endl;
        std::cout << "test file system : end with return code = "  << flg << std::endl;
        std::cout << "--------------------------------------------------------------" <<  std::endl;
        std::cout << "test driver : start" << std::endl;
        std::cout << "--------------------------------------------------------------" <<  std::endl;
        flg = test_driver(argc, argv);
        std::cout << "--------------------------------------------------------------" <<  std::endl;
        std::cout << "test driver : end with return code = "  << flg << std::endl;
        std::cout << "--------------------------------------------------------------" <<  std::endl;
        return 0;
    }

    int test_filesystem_1() {
        ///////////////////////////////////////////////////////////////////////////////////////////
        // prepare
        ///////////////////////////////////////////////////////////////////////////////////////////
        std::string session_name = "test_session";
        std::string analyst_name = "test_analyst";
        std::string user_name = "test_user";
        FileSystemClient fs1(session_name, analyst_name, user_name);
        // test
        ///////////////////////////////////////////////////////////////////////////////////////////
        if(fs1.make_session_directory() == 0) {
            std::cout << "[OK] test filesystem 1 : make session directory" << std::endl;
        } else {
            std::cout << "[NG] test filesystem 1 : make session directory" << std::endl;
            fs1.remove_directory(session_name);
            return -1;
        }
        if(fs1.make_directory(fs1.path_enc_uploading_dir("categ")) == 0) {
            std::cout << "[OK] test filesystem 1 : make directory enc upload dir" << std::endl;
        } else {
            std::cout << "[NG] test filesystem 1 : make directory enc upload dir" << std::endl;
            fs1.remove_directory(session_name);
            return -1;
        }
        if(fs1.make_directory(fs1.path_enc_result_dir("categ")) == 0) {
            std::cout << "[OK] test filesystem 1 : make directory enc result dir" << std::endl;
        } else {
            std::cout << "[NG] test filesystem 1 : make directory enc result dir" << std::endl;
            fs1.remove_directory(session_name);
            return -1;
        }
        fs1.remove_directory(session_name);
        return 0;
    }

    int test_driver(int argc, char *argv[]) {
        std::string config_file_path = argv[2];
        std::vector<std::string> vbuf;
        Utils::get_items(config_file_path, CConst::CFG_KEY_HOST, &vbuf);
        std::string hostname = vbuf[0];
        vbuf.clear();
        vbuf.shrink_to_fit();
        Utils::get_items(config_file_path, CConst::CFG_KEY_PORT, &vbuf);
        std::string port_no = vbuf[0];
        vbuf.clear();
        vbuf.shrink_to_fit();
        Utils::get_items(config_file_path, CConst::CFG_KEY_UNAME, &vbuf);
        std::string user_name = vbuf[0];
        vbuf.clear();
        vbuf.shrink_to_fit();
        std::cout << "hostname : " << hostname << std::endl;
        std::cout << "port_no : " << port_no << std::endl;
        std::cout << "username : " << user_name << std::endl;
        return 0;
    }

}

