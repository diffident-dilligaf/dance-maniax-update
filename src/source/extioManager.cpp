// extioManager.cpp implements programming the serial port for DMX
// source file created by Allen Seitz 11/14/2013

#include "extioManager.h"

#define ARDUINO_WAIT_TIME 2000
#define ARDUINO_TIMEOUT 4000

extioManager::extioManager()
{
	hSerial = INVALID_HANDLE_VALUE;
	comPort = -1;
	isConnected = false;
	isTalking = false;
	powerOnTime = 0;
	connectionState = 0;
}

void extioManager::initialize()
{
	for ( int i = 0; i < PACKET_SIZE; i++ )
	{
		inputBuffer[i] = 0;
	}

#ifdef DMXDEBUG
	comPort = TRUSTED_COM_PORT - 1;
#endif
}

bool extioManager::updateInitialize(UTIME dt)
{
	if ( !isConnected )
	{
		comPort++;
		if ( comPort < 10 )
		{
			char port[] = "\\\\.\\COM0";

			port[7] = comPort + '0';
			attemptConnection(port);
			connectionState = 1;
		}
		else
		{
			al_trace("Can't find the IO board on any com port.\r\n");
			comPort = -1;
			connectionState = 0;
			return false;
		}

		powerOnTime = 0;
	}
	else
	{
		powerOnTime += dt;

		if ( connectionState == 1 )
		{
			if ( powerOnTime >= ARDUINO_WAIT_TIME )
			{
				WriteData("DMX", PACKET_SIZE);
				connectionState = 2;
			}
		}
		else if ( connectionState == 2 )
		{
			if ( isPacketReady() )
			{
				ReadData(inputBuffer, PACKET_SIZE);
				if ( strcmp(inputBuffer, "OK!") == 0 )
				{
					isTalking = true;
					connectionState = 3;
					al_trace("Handshake accepted! Found the IO board on port %d.\r\n", comPort);
				}
				else
				{
					// abort - try another port
					powerOnTime = 0;
					isConnected = false;
					connectionState = 0;
					al_trace("The handshake was incorrect on port %d. Checking other ports.\r\n", comPort);
				}
			}
			else if ( powerOnTime >= ARDUINO_TIMEOUT )
			{
				// abort - try another port
				powerOnTime = 0;
				isConnected = false;
				connectionState = 0;
				al_trace("IO board timeout on port %d. Checking other ports.\r\n", comPort);
			}
		}
		else if ( connectionState == 3 )
		{
			al_trace("Do not call updateInitialize() after success!\r\n");
		}
	}

	return true;
}

bool extioManager::isReady()
{
	return isConnected && isTalking;
}

void extioManager::update(UTIME dt)
{
	static UTIME benchmarkTime = 0;

	if ( isConnected )
	{
		powerOnTime += dt;
	}
	if ( !isReady() )
	{
		return;
	}

	// actually do work here
	//*
	if ( connectionState == 3 )
	{
		char temp[4] = "000";
		for ( int i = 0; i < PACKET_SIZE; i++ )
		{
			temp[i] = rand()%5 + '0';
		}
		//WriteData("PING__", PACKET_SIZE);
		WriteData(temp, PACKET_SIZE);
		al_trace("%s ->", temp);
		benchmarkTime = powerOnTime;
		connectionState = 4;
	}
	if ( isPacketReady() )
	{
		ReadData(inputBuffer, PACKET_SIZE);
		al_trace("%c%c%c in %dms\r\n", inputBuffer[0], inputBuffer[1], inputBuffer[2], powerOnTime-benchmarkTime);
		if ( strcmp(inputBuffer, "PONG__") == 0 )
		{
		}
		connectionState = 3;
	}
	//*/
}

bool extioManager::attemptConnection(const char* port)
{
	isConnected = false;

	hSerial = CreateFile(port,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if(hSerial==INVALID_HANDLE_VALUE)
	{
		//al_trace("The IO board isn't on %s\n", port);
		return false; // it's okay - we'll try another port
	}

	//If connected we try to set the comm parameters
	DCB dcbSerialParams = {0};

	if ( !GetCommState(hSerial, &dcbSerialParams) )
	{
		al_trace("extioManager: failed to GET serial parameters!");
		return false;
	}

	//Define serial connection parameters for the arduino board
	dcbSerialParams.BaudRate = CBR_9600;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;

	if( !SetCommState(hSerial, &dcbSerialParams) )
	{
		al_trace("extioManager: failed to SET serial parameters!");
		return false;
	}

	isConnected = true;
	return true;
}

//////////////////////////////////////////////////////////////////////////////
// LOW LEVEL SERIAL FUNCTIONS
//////////////////////////////////////////////////////////////////////////////

int extioManager::ReadData(char *buffer, unsigned int nbChar)
{
    //Number of bytes we'll have read
    DWORD bytesRead;
    //Number of bytes we'll really ask to read
    unsigned int toRead;

    //Use the ClearCommError function to get status info on the Serial port
    ClearCommError(this->hSerial, &this->errors, &this->status);

    //Check if there is something to read
    if(this->status.cbInQue>0)
    {
        //If there is we check if there is enough data to read the required number
        //of characters, if not we'll read only the available characters to prevent
        //locking of the application.
        if(this->status.cbInQue>nbChar)
        {
            toRead = nbChar;
        }
        else
        {
            toRead = this->status.cbInQue;
        }

        //Try to read the require number of chars, and return the number of read bytes on success
        if(ReadFile(this->hSerial, buffer, toRead, &bytesRead, NULL) && bytesRead != 0)
        {
            return bytesRead;
        }
    }

    //If nothing has been read, or that an error was detected return -1
    return -1;
}

bool extioManager::WriteData(char *buffer, unsigned int nbChar)
{
    DWORD bytesSend;

    //Try to write the buffer on the Serial port
    if(!WriteFile(this->hSerial, (void *)buffer, nbChar, &bytesSend, 0))
    {
        //In case it don't work get comm error and return false
        ClearCommError(this->hSerial, &this->errors, &this->status);

        return false;
    }
    else
        return true;
}

bool extioManager::isPacketReady()
{
    ClearCommError(this->hSerial, &this->errors, &this->status);

    return this->status.cbInQue >= PACKET_SIZE;
}