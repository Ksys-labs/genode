#include <base/printf.h>
#include <timer_session/connection.h>

int main(int argc, char **argv)
{
	using namespace Genode;
	
	printf("--- hackme ---\n");
	static Timer::Connection timer;
	
	while(1) {
		printf("hello\n");
		timer.msleep(1000);
	}
}