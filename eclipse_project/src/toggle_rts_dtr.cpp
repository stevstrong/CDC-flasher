
	//====================================================================================================//
	// Program to open the Serial port and control the RTS and DTR pins (SET and CLEAR ) using Win32 API  //
	// EscapeCommFunction() is used to SET and CLEAR the RTS and DTR pins                                 //
	// Baud Rate,Data Rate etc are not set since no serial Communication takes place                      //
	//====================================================================================================//

	//====================================================================================================//
	// www.xanthium.in										                                              //
	// Copyright (C) 2015 Rahul.S                                                                         //
	//====================================================================================================//

	//====================================================================================================//
	// Compiler/IDE  :	Microsoft Visual Studio Express 2013 for Windows Desktop(Version 12.0)            //
	//               :  gcc 4.8.1 (MinGW)                                                                 //
	// Library       :  Win32 API,windows.h,                                                              //
	// Commands      :  gcc -o SerialPort_RTS_DTR SerialPort_RTS_DTR.c                                    //
	// OS            :	Windows(Windows 7)                                                                //
	// Programmer    :	Rahul.S                                                                           //
	// Date	         :	20-January-2015                                                                   //
	//====================================================================================================//

	//====================================================================================================//
	// Sellecting the COM port Number                                                                     //
    //----------------------------------------------------------------------------------------------------//
    // Use "Device Manager" in Windows to find out the COM Port number allotted to USB2SERIAL converter-  //
    // -in your Computer and substitute in the  CreateFile()                                              //
	//                                                                                                    //
	// for eg:-                                                                                           //
	// If your COM port number is COM32 in device manager(will change according to system)                //
	// then                                                                                               //
	//			    CreateFile("\\\\.\\COM32", , , , );                                                   //
	//====================================================================================================//

	#include <Windows.h>
	#include <stdio.h>
	#include <string.h>
#include <conio.h>

	int main(int argc, _TCHAR* argv[])
	{
		HANDLE hComm;                          // Handle to the Serial port
		char portName[20];
		sprintf(portName, "\\\\.\\%s", argv[1]);

		printf("\n\n +------------------------------------------------------------------+");
		printf("\n | Program to Set and Clear the RTS and DTR lines of PC Serial Port |");
		printf("\n | Using Win 32 API                                                 |");
		printf("\n | Please input the Serial port as argument, eg COM32               |");
	 	printf("\n +------------------------------------------------------------------+");


		//-------------------------- Opening the Serial Port------------------------------------//
		hComm = CreateFile( portName,                  // Name of the Port to be Opened
							GENERIC_READ | GENERIC_WRITE, // Read/Write Access
							0,                            // No Sharing, ports cant be shared
							NULL,                         // No Security
						    OPEN_EXISTING,                // Open existing port only
							0,                            // Non Overlapped I/O
							NULL);                        // Null for Comm Devices

		if (hComm == INVALID_HANDLE_VALUE)
			printf("\n   Error! - Port %s can't be opened", portName);
		else
			printf("\n   Port %s Opened \n", portName);

		printf("\nPress 'r' for RTS=0, 'R' for RTS=1, 'd' for DTR=0, 'D' for DTR=1, any other to quit.\n");
		//-------------------------Manipulating RTS/DTR lines of  PC Serial Port-------------------------//
		DWORD cmd;
		int c = 0;
		char * msg;
		while (1)
		{
			c = getch(); // Wait for user input
			switch (c) {
			case 'r': cmd = CLRRTS; msg = (char*)"RTS = 0"; break;
			case 'R': cmd = SETRTS; msg = (char*)"RTS = 1"; break;
			case 'd': cmd = CLRDTR; msg = (char*)"DTR = 0"; break;
			case 'D': cmd = SETDTR; msg = (char*)"DTR = 1"; break;
			default: c = 'x'; break;
			}
			if (c=='x')
				break;

			if ( EscapeCommFunction(hComm, cmd) == false )
			{
				printf("\n   Error! \n");
				break;
			}
			else
				printf("%s\n", msg);
		}

		CloseHandle(hComm);//Closing the Serial Port

		printf("\n\n   Serial Port Closed, Press Any Key to Exit");
		printf("\n +------------------------------------------------------------------+");
		_getch();
		return 0;
	}
