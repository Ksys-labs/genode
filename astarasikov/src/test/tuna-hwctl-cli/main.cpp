/*
 * \brief   Tuna board hardware control app
 * \author  Alexander Tarasikov <tarasikov@ksyslabs.org>
 * \date    2013-02-15
 */

#include <base/snprintf.h>

#include <regulator_session/connection.h>
#include <timer_session/connection.h>
#include <drivers/mfd/twl6030/regulator.h>

using namespace Genode;

enum {
	VLCD = Regulator::TWL6030::VAUX3,
	VLCD_IOVCC = Regulator::TWL6030::VUSIM,
	VDSS_DSI = Regulator::TWL6030::VCXIO,
	HDMI_VREF = Regulator::TWL6030::VDAC,
	VOTG = Regulator::TWL6030::VUSB,
	VMMC = Regulator::TWL6030::VMMC,
};

int main(int argc, char *argv[])
{
	printf("--- Regulator test started ---\n");
	static Regulator::Connection regulator("twl6030");
	static Timer::Connection timer;

	#define TEST(x) if (!x) PERR("failed to " #x)

	TEST(regulator.enable(VDSS_DSI));
	
	TEST(regulator.enable(VLCD_IOVCC));
	TEST(regulator.set_level(VLCD_IOVCC, 2200000, 2200000));
	
	TEST(regulator.enable(VLCD));
	TEST(regulator.set_level(VLCD, 3100000, 3100000));

	TEST(regulator.enable(VMMC));
	TEST(regulator.set_level(VMMC, 1800000, 1800000));

	TEST(regulator.disable(Regulator::TWL6030::CLK32KG));
	TEST(regulator.disable(Regulator::TWL6030::CLK32KAUDIO));

	return 0;
}
