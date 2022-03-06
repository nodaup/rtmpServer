#include <iostream>

int main() {
	uint8_t recv[5] = "hi";
	uint8_t temp[5];
	memcpy(temp, recv, 5);
	std::cout << "recv value: "<< *recv <<",recv add:"<< &recv<<",recv: "<< recv << std::endl;
	
	std::cout << "temp value: " << *temp << ",temp add:" << &temp << ",temp: " << temp << std::endl;

	uint8_t* data = temp;
	std::cout << "data value: " << *data << ",data add:" << &data << ",data: " << data << std::endl;

}