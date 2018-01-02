#include <vector>
#include <armadillo>
#include <boost/program_options.hpp>
#include <iostream>
#include <glog/logging.h>

#include <h5cpp/core>
	#include <tests/struct.h>
#include <h5cpp/io>

using namespace std;
namespace po = boost::program_options;

/** 
 * 
 *
 */

template <class T> struct harness {
};


int main(int argc, char **argv) {
	std::string input,output,source,destination,glog_dir;
	bool glog_stderr;
    unsigned int glog_minloglevel, time_interval, signal_delay;
    po::options_description desc("Allowed options",120);

    desc.add_options()
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
            cout << "bandwidth profiler harness... "<<endl;
            cout <<endl;

            cout << desc << endl <<endl;

            cout <<"example:" <<endl;
            cout <<"   "<< argv[0] <<"   \n\n" <<endl;
            return 0;
        }
	} catch( ... ){
        cout << endl << "Error parsing arguments!!! "<<endl<<endl;
        cout << desc <<endl<<endl;
    }

    return 0;
}


