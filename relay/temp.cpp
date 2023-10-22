#include <thread>
#include <barrier>
#include <iostream>
#include <map>
using namespace std;

class ConnectionManager
{
public:
	std::map<std::string, std::barrier<>*> identifier_to_barrier;
};

int main()
{
    ConnectionManager cm;
    cm.identifier_to_barrier["a"] = new std::barrier(2);

    thread([&cm](){
        cm.identifier_to_barrier["a"]->arrive_and_wait();
        cout << "Thread 1" << endl;
    }).detach();
    thread([&cm](){
        cm.identifier_to_barrier["a"]->arrive_and_wait();
        cout << "Thread 2" << endl;
    }).detach();
    sleep(1);
    return 0;
}