#include "who_am_I.hpp"

using namespace std;

atomic<bool> i_am_server;

void init_who_am_I() {
	string my_addr;
	
	ifstream mac_file;
	mac_file.open(PROGRAM_ROOT_DIR + "mac_addr");
	
	mac_file >> my_addr;
	mac_file.close();
	
	cout << "server mac addr -> " << BT_SERVER_MAC << endl;
	cout << "my mac addr -> " << my_addr << endl;
	
	if (!BT_SERVER_MAC.compare(my_addr)) {
		i_am_server = true;
		cout << "I AM SERVER" << endl;
	} else {
		i_am_server = false;
		cout << "I AM CLIENT" << endl;
	}
}

