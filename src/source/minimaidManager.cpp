// minimaidManager.h implements HID support for a certain custom IO board
// source file created by Allen Seitz 02/07/2016

#include "minimaidManager.h"
#include "inputManager.h"
#include "lightsManager.h"

extern InputManager im;
extern LightsManager lm;

struct mm_input {                                                              
    uint8_t report_id;                                                         
    uint8_t dip_switches;                                                      
    uint32_t jamma;                                                            
    uint8_t ext_in;                                                            
};                                                                             
                                                                               
struct mm_output {                                                             
    uint8_t report_id;                                                         
    uint8_t ext_output;                                                        
    uint32_t lights;                                                           
    uint8_t blue_led;                                                          
    uint8_t kbd_enable;                                                        
    uint8_t aux_flags;                                                         
}; 

// four molexes fit on the minimaid, each with up to 8 pins
// I guess the menu buttons just don't fit?
int minimaidLampOrder[32] = {
	lampBlue0, lampRed0, lampBlue1, lampRed1, lampBlue0+1, lampRed0+1, lampBlue1+1, lampRed1+1,
	lampBlue2a, lampBlue2b, lampRed2a, lampRed2b, lampBlue3a, lampRed3b, lampRed3a, lampRed3b,
	lampBlue2a+1, lampBlue2b+1, lampRed2a+1, lampRed2b+1, lampBlue3a+1, lampRed3b+1, lampRed3a+1, lampRed3b+1,
	spotlightA, spotlightB, spotlightC, NULL, NULL, NULL, NULL, NULL
}; 

minimaidManager::minimaidManager()
{
	handle = NULL;
	powerOnTime = 0;
	timeSinceLastLampUpdate = 0;
}

void minimaidManager::initialize()
{
	wchar_t wstr[256];

	// Open the device using the VID, PID, and optionally the Serial number.
	handle = hid_open(0x5730, 0x0000, NULL);

	// Read the Manufacturer String - sample code, not used for anything
	int res = hid_get_manufacturer_string(handle, wstr, 256);
	wprintf(L"Manufacturer String: %s\n", wstr);

	// Read the Product String - sample code, not used for anything
	res = hid_get_product_string(handle, wstr, 256);
	wprintf(L"Product String: %s\n", wstr);

	// Read the Serial Number String - sample code, not used for anything
	res = hid_get_serial_number_string(handle, wstr, 256);
	wprintf(L"Serial Number String: (%d) %s\n", wstr[0], wstr);
}

bool minimaidManager::updateInitialize(UTIME dt)
{
	// easy. if initialize didn't find it right away, then it's not plugged in
	if ( handle == NULL )
	{
		return false;
	}
	return true;
}

bool minimaidManager::isReady()
{
	return handle != NULL;
}

void minimaidManager::update(UTIME dt)
{
	if ( !isReady() )
	{
		return;
	}
	powerOnTime += dt;
	timeSinceLastLampUpdate += dt;

	// read input from the hid
}

void minimaidManager::updateLamps()
{
	char b0 = 0, b1 = 0, b2 = 0;

	if ( timeSinceLastLampUpdate < 20 )
	{
		//al_trace("--STIFLED LAMP UPDATE-- too soon of an update.\r\n");
		return; // don't do that
	}
	timeSinceLastLampUpdate = 0;

	// prepare the output structure - I'm not sure what most of thse do to be honest
	struct mm_output out;
	out.report_id = 0;
	out.ext_output = 0;
	out.blue_led = 0;
	out.aux_flags = 0;
	out.kbd_enable = 0; // might be a good idea to see what the bindings are, and if that would be easier?
	out.lights = 0; // set in the next code block

	for ( int i = 0; i < 32; i++ )
	{
		if ( minimaidLampOrder[i] != 0 )
		{
			out.lights |= lm.getLamp(minimaidLampOrder[i]) ? 1 << i : 0;
		}
	}

	al_trace("LAMP %d\n", out.lights);

	// move the mm_output struct into an array of chars for hid_write
	unsigned char outstream[128];
	memcpy_s(&outstream, 128, &out, sizeof(mm_output));
	hid_write(handle, outstream, sizeof(mm_output));
}