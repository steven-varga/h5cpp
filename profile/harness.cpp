#include <vector>
#include <boost/program_options.hpp>
#include <iostream>
#include <glog/logging.h>
#include <cstdlib>

#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>


#include "runner.hpp"

using namespace std;
namespace po = boost::program_options;

// used when capturing /proc/... system info 
std::string exec(const char* cmd) {
	const int buf_size = 1024;
    std::array<char, buf_size> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw std::runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), buf_size, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result;
}

int main(int argc, char **argv) {
	std::string input,output,source,destination,glog_dir, test_case;
	size_t grid_row,grid_col;
	bool glog_stderr;
    unsigned int glog_minloglevel;
    po::options_description desc("Allowed options",120);

	std::map<std::string,std::function<void(const std::string& name, size_t grid_rows,size_t grid_cols)>> dispatch;

	dispatch["arma-rowvec-write"]  = harness<VectorWriteTest<arma::rowvec>>;
	//dispatch["arma-rowvec-read"]   = harness<VectorReadTest<arma::rowvec>>;
	//dispatch["arma-stlvec-write"]  = harness<VectorWriteTest<std::vector<double>>>;
	//dispatch["arma-stlvec-read"]   = harness<VectorReadTest<std::vector<double>>>;


    desc.add_options()
            ("test-case", po::value<std::string>(),"one of the test cases queried by --list option")
            ("grid-row", po::value<size_t>()->default_value(1),"grid search row parameter")
			("grid-col", po::value<size_t>()->default_value(0), "grid search column parameter")
            ("list,l","list test cases...\n")

            ("glog-dir", po::value<string>()->default_value("./"),"glog output directory") 
            ("glog-stderr", po::value<bool>()->default_value(true),"glog output to stderr if true") 
            ("glog-minloglevel", po::value<unsigned int>()->default_value(0),"glog log level:  INFO=0 WARNING=1 ERROR=2 FATAL=3\n")
            ("help,h", "produce help message")
            ;

    try {
        po::variables_map vm;
        po::store(po::parse_command_line(argc, argv, desc), vm);
        po::notify(vm);

        if (vm.count("help")) {
            cout << "profiler harness with grid search "<<endl;
            cout <<endl;
            cout << desc << endl <<endl;

            cout <<"example:" <<endl;
            cout <<"   "<< argv[0] <<"   \n\n" <<endl;
            return 0;
        }
		if( vm.count("list") ){
			for( auto i:dispatch )
				std::cout << i.first << std::endl;
			return 0;
		}

	std::string cpu_freq = exec("cat /proc/cpuinfo | grep 'cpu MHz'| head -n 1|cut -d':' -f2|tr -d '\n \t'");
	std::string cpu_type = exec("cat /proc/cpuinfo | grep 'model name'| head -n 1| cut -d':' -f2|cut -d' ' -f4 | tr -d '\n \t'");
	std::string load_avg = exec("cat /proc/loadavg | cut -d' ' -f1,2,3 | tr -d '\n' | tr ' ' '\t'");

	grid_row  = vm["grid-row"].as<size_t>();
	grid_col  = vm["grid-col"].as<size_t>();
   	test_case = vm["test-case"].as<std::string>();

	dispatch[test_case](test_case,grid_row,grid_col);

	std::cout<< cpu_freq <<" " << cpu_type << " " << load_avg  << std::endl;

	} catch( ... ){
        cout << endl << "Error parsing arguments!!! "<<endl<<endl;
        cout << desc <<endl<<endl;
    }

    return 0;
}

