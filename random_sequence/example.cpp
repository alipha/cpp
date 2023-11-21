#include "random_sequence.h"
#include <iostream>
#include <map>
#include <iomanip>

int main() {
	std::map<int, int> m;
	random_sequence seq(900000);
	for(int i = 0; i <= 900001; ++i) {	// intentionally make the last loop be a dup to test the dup detection logic
		if(i % 40 == 0)
			std::cout << '\n';
		int result = seq.next();
		std::cout << '\t' << result;
		if(m.find(result) != m.end()) {
			std::cout << "dup! " << i << " = " << std::hex << result << '\n';
			return 1;
		}
		m[result] = i;
	}
	std::cout << '\n';
}
