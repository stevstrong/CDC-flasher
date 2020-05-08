/*
 * Created by stevestrong
 * 2020.05.03
 *
 * To setup Eclipse with MinGW:
 * 		https://www.codeguru.com/cpp/cpp/getting-started-with-c-for-eclipse.html
 *
 * Various sources of inspiration:
 * 		https://aticleworld.com/get-com-port-of-usb-serial-device/
 * 		https://aticleworld.com/serial-port-programming-using-win32-api/
 */

#include <windows.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <devguid.h>
#include <Setupapi.h>


typedef ULONG DEVPROPTYPE, *PDEVPROPTYPE;

using namespace std;

//-----------------------------------------------------------------------------
void delay(int millis)
{
	// Converting time into milli_seconds

	// Storing start time
	clock_t start_time = clock();

	// looping till required time is not achieved
	while (clock() < start_time + millis)
		;
}
//-----------------------------------------------------------------------------
bool GetComPort(const char * vid, const char * pid, char *comPort)
{
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    const TCHAR * devEnum = "USB";
    char buffer[1024] = {0};
    int retVal = false;

    //SetupDiGetClassDevs returns a handle to a device information set
    deviceInfoSet = SetupDiGetClassDevs(
                        NULL,
                        devEnum,
                        NULL,
                        DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
		printf(" ERROR: SetupDiGetClassDevs returned INVALID_HANDLE_VALUE !!!\n");
        return false;
    }

	string vidpidstr = "VID_";
	vidpidstr += vid;
	vidpidstr += ("&PID_");
	vidpidstr += pid;

	for (auto & c: vidpidstr) c = toupper(c);

    DWORD deviceIndex =0;
	//Fills a block of memory with zeros
    ZeroMemory(&deviceInfoData, sizeof(SP_DEVINFO_DATA));
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    //Receive information about an enumerated device
    while (1)
    {
    	bool ret = SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex++, &deviceInfoData);
    	if (ret==0)
    	{
			//printf(" ERROR: Could not get device info !\n");
    		break;
    	}
        //Retrieves a specified Plug and Play device property
		DWORD dwSize = 0;
#if 1
		ret = SetupDiGetDeviceInstanceId (deviceInfoSet, &deviceInfoData, buffer, sizeof(buffer), &dwSize);
		if (ret==false)
		{
			printf(" ERROR: Could not get device instance Id !");
    		break;
    	}
#else
		DEVPROPTYPE ulPropertyType;
        ret = SetupDiGetDeviceRegistryProperty (deviceInfoSet, &deviceInfoData, SPDRP_HARDWAREID,
                                              &ulPropertyType, (BYTE*)buffer, sizeof(buffer), &dwSize))
		if (ret==false)
		{
			printf(" ERROR: Could not get SetupDiGetDeviceRegistryProperty !");
    		break;
    	}
#endif

		// pos 0 = "USB", pos 1 = "VID_01234&PID=ABCD", pos 2 = serial number
		char* word = strtok( buffer, "\\" );
		if ( strstr(word, "USB") == NULL ) { // no match
			continue;
		}
		// USB matched. check next token
		word = strtok( NULL, "\\" );
		if ( strstr( word, vidpidstr.c_str() ) == NULL )
		{
			continue; // it was not our VID:PID, continue to search next item
		}

		// Get the key where to extract the port number from
		HKEY hDeviceRegistryKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
		if (hDeviceRegistryKey == INVALID_HANDLE_VALUE)
		{
			printf(" ERROR: SetupDiOpenDevRegKey returned %lu", GetLastError());
			break;
		}
		// Read in the name of the port
		TCHAR portName[40];
		dwSize = sizeof(portName);
		DWORD dwType = REG_SZ;
		if ( (RegQueryValueEx(hDeviceRegistryKey, "PortName", NULL, &dwType, (LPBYTE) portName, &dwSize) == ERROR_SUCCESS) && (dwSize>0) )
		{
			strcpy(comPort, portName);
			retVal = true;
		}
		else
		{
			printf(" ERROR: could not get port name with RegQueryValueEx for device [%s:%s] !\n", vid, pid);
		}
		// Close the key now that we are finished with it
		RegCloseKey(hDeviceRegistryKey);
		break;
	}

	if (deviceInfoSet)
	{
		SetupDiDestroyDeviceInfoList(deviceInfoSet);
	}
	return retVal;
}
//-----------------------------------------------------------------------------
bool OpenComPort(HANDLE* comm, char * portName)
{
    // translate com port to windows required friendly name
     char portNo[20] = { 0 };
    sprintf(portNo, "\\\\.\\%s", portName);
    // Open the serial com port
    HANDLE hComm = CreateFile(portNo, //friendly name
                       GENERIC_READ | GENERIC_WRITE,      // Read/Write Access
                       0,                                 // No Sharing, ports can't be shared
                       NULL,                              // No Security
                       OPEN_EXISTING,                     // Open existing port only
                       0,                                 // Non Overlapped I/O
                       NULL);                             // Null for Comm Devices
    if (hComm == INVALID_HANDLE_VALUE)
    {
        //printf("\n ERROR: cannot open port %s\n", portName);
        return false;
    }

	*comm = hComm; // set valid handler

	return true;
}
//-----------------------------------------------------------------------------
bool WaitForAnswer(HANDLE hComm, int i, unsigned char* header)
{
	// wait for answer from the device
	unsigned char rdArray[8];
	DWORD readBytes = 0;

	clock_t start = clock();
	while( (readBytes==0) && (clock()-start)<1000 )
	{
		//Read data and store in a buffer
		bool status = ReadFile(hComm, &rdArray, sizeof(rdArray), &readBytes, NULL);
		if (status==false)
		{
			printf(" ERROR: could not read from port.\n");
			CloseHandle(hComm);
			return false;
		}
	}

	// check if response was an error
	if ( readBytes==1 )
	{
		// it is probably an error message. handle it here
		unsigned char myByte = rdArray[0];
		if ( myByte == 0 ) {
			printf(" ERROR: unexpected reply 0 after page %d\n", i);
		} else if ( myByte == 1 ) {
			printf(" ERROR: missing setup packet.\n");
		} else if ( myByte == 3 ) {
			printf("ERROR: data overflow at page %d. Try to upload again\n", i);
		} else if ( myByte == 4 ) {
			printf("ERROR: data underflow at page %d. Try to upload again\n", i);
		} else if ( myByte == 5 ) {
			printf("ERROR: wrong command length at page %d\n", i);
		} else if ( myByte == 6 ) {
			printf("ERROR: wrong command CRC at page %d\n", i);
		} else {
			printf("ERROR: unexpected reply 0x%02X at page %d\n", myByte, i);
		}
	}
	else if ( readBytes != sizeof(rdArray) ) // check for header length
	{
		printf(" ERROR: unexpected reply of %ld bytes after page %d\n", readBytes, i);
	}
	else
	{
		int cmp = memcmp(rdArray, header, sizeof(rdArray));
		if (cmp==0)
			return true;
		printf(" ERROR: wrong echo content after page %d\n", i);
	}
	CloseHandle(hComm);
	return false;
}
//-----------------------------------------------------------------------------
bool SendHeader(HANDLE hComm, unsigned char cmd, char pack_num, int len)
{
	unsigned char header[] = {0x41, 0xBE, 0, 0, 0, 0, 0, 0};
	header[2] = cmd;
	header[3] = pack_num;
	header[4] = (len%256);
	header[5] = (len/256);

	// calculate and add CRC to the array
	unsigned int crc = 0;
	for (unsigned int i=0; i<(sizeof(header)-2); i++)
	{
		crc += header[i];
	}
	crc = crc ^ 0xFFFF;
	header[6] = (crc%256);	// lo_byte
	header[7] = (crc/256);	// hi_byte

    // Writing data to Serial Port
	DWORD bytesWritten = 0;
    if ( ( WriteFile(hComm, header, sizeof(header), &bytesWritten, NULL) == false ) || (bytesWritten != sizeof(header)) )
    {
        printf(" ERROR: header could not be sent\n");
        return false;
    }
	// wait for answer
	WaitForAnswer(hComm, pack_num, header);

    return true;
}
//-----------------------------------------------------------------------------
bool SendMagic(char * portName)
{
    HANDLE hComm = NULL;
	bool status = OpenComPort(&hComm, portName);
	if (status==false)
	{
		//printf(" ERROR: could not open port %s", portName);
smRet:
		CloseHandle(hComm);
		return status;
	}
	printf("\nOpened port %s, triggering reset...", portName);

	// generate DTR negedge trigger
	status = EscapeCommFunction(hComm, CLRDTR); // clear DTR
	if (status==false)
	{
smRet1:
		printf(" ERROR: could not control DTR.\n");
		goto smRet;
	}
	delay(10);

	status = EscapeCommFunction(hComm, SETDTR); // set DTR
	if (status==false)
		goto smRet1;
	delay(10);

	status = EscapeCommFunction(hComm, CLRDTR); // clear DTR
	if (status==false)
		goto smRet1;
	delay(1);

	// send the magic sequence to trigger reset
	DWORD bytesWritten = 0;          // No of bytes written to the port
	const char* magic = "1EAF";
	status = WriteFile(hComm, magic, 4, &bytesWritten, NULL);
    if (status==false)
    	printf(" ERROR: could not open port %s\n", portName);
    else
		printf("done.\n");

    goto smRet;
}
//-----------------------------------------------------------------------------
bool SendFile(char * portName, char* file_buffer, long file_size, int page_size)
{
	HANDLE hComm;
	bool ret = OpenComPort(&hComm, portName);
	if (ret==false)
	{
		//printf(" ERROR: could not open port %s\n", portName);
		return false;
	}

	printf ("\nFound bootloader on %s. Opening file...", portName);

	// count how many packets have to be sent
	page_size *= 1024; // convert to bytes
	int num_packets = file_size/page_size + 1;
	printf("done.\nFile size is %ld bytes, page size is %d bytes, %d pages to flash\n", file_size, page_size, num_packets);
/*
	_COMMTIMEOUTS tout = {
		.ReadIntervalTimeout = MAXDWORD, // the read operation is to return immediately with the bytes that have already been received, even if no bytes have been received.
		.ReadTotalTimeoutMultiplier = 0,
		.ReadTotalTimeoutConstant = 0,
		.WriteTotalTimeoutMultiplier = 10,
		.WriteTotalTimeoutConstant = 50,
	};
	SetCommTimeouts(hComm, &tout);
*/
	// send the setup header
	if ( SendHeader(hComm, 0x20, num_packets, 0)==false )
	{
		printf(" ERROR: could not send setup header.\n");
		CloseHandle(hComm);
		return false;
	}

	// send the file in multiple chunks of page size
	printf("Flashing binary file\n\n");
//	printf("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n");
	printf("0%% ----- 10%% ----- 20%% ----- 30%% ----- 40%% ----- 50%% ----- 60%% ----- 70%% ----- 80%% ----- 90%% -- 100%%\n");

	int progress = 0;
	int pack_num = 0;
	int offset = 0, pack_size = 0;
	while ( pack_num<num_packets )
	{
		// get the payload size
		int remaining = file_size - offset;
		if (remaining <= 0)
		{ // finished sending all the data
			break;
		}
		pack_size = (remaining > page_size) ? page_size : remaining;
		// send header
		if ( SendHeader(hComm, 0x21, pack_num, pack_size)==false ) // and wait for answer
		{
			printf(" ERROR: could not send payload header for packet %d.\n", pack_num);
			break;
		}
		// send data payload
		DWORD bytesWritten = 0;          // No of bytes written to the port
		ret = WriteFile(hComm, &file_buffer[offset], pack_size, &bytesWritten, NULL);
		if ( ret==false || (int)bytesWritten!=pack_size)
		{
			printf(" ERROR: could not send payload for packet %d.\n", pack_num);
			break;
		}
		// prepare to send the next packet number
		offset += pack_size;
		pack_num ++;
		int section_progress = (100*pack_num/num_packets);
		//printf("%d", section_progress);
		while ( progress < section_progress )
		{
			//printf("%d", progress);
			printf("#"); // show progress
			progress ++;
		}
	}
	printf("\n\n");

	free(file_buffer);	// free up the reserved buffer

	CloseHandle(hComm); // Closing the Serial Port

	return ret;
}
//-----------------------------------------------------------------------------
bool OpenReadFile(const char* file, char** file_data, long* fSize)
{
	// Open the firmware file in binary mode
	FILE * binFile;
	binFile = fopen(file, "rb");
	if( !binFile )
	{
		printf(" ERROR: cannot open file %s. Check the path and file name.\n", file);
orfRet:
		if (binFile) fclose(binFile);
		return false;
	}

	// Get the file size
	fseek(binFile, 0L, SEEK_END);
	long file_size = ftell(binFile);
	rewind(binFile);
	// allocate buffer for the whole file to fit in
	char* file_buffer = (char*) malloc(sizeof(char)*file_size);
	if (file_buffer == NULL)
	{
		printf(" ERROR: memory error\n");
		goto orfRet;
	}
	// read the whole file into the buffer
	long read_bytes = fread (file_buffer, 1, file_size, binFile);
	if (read_bytes != file_size)
	{
		printf(" ERROR: Reading error");
		free(file_buffer);
		goto orfRet;
	}
	fclose(binFile);
	*fSize = file_size;
	*file_data = file_buffer;
	return true;
}
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int main(int argc, _TCHAR* argv[])
{
	// find out the number of input parameters
	int i = 0;
	for (; i<128; ++i)
		if (argv[i+1]==NULL) break;

	if (i!=3)
	{
		printf (" ERROR: wrong number %d of input parameters!\n"
				"Expected usage:\n"
				"\t%s <binary_file> <com_port> <page_size_in_kb>\n", i, argv[0]);
		printf ("Example:\n"
				"\t%s <absolute_path>/blinky.bin COM14 1\n", argv[0]);
		return -1;
	}

#define binFile		argv[1]
#define portName	argv[2]
#define pageSize	atoi(argv[3])

#define TIME_OUT	5	// seconds to wait for the bootloader device to appear

	// read file
	long f_size;
	char* filePtr; // pointer to buffer containing binary data read from file
	if ( OpenReadFile(binFile, &filePtr, &f_size) == false )
	{
		return -1;
	}

	bool send_magic = true;
	bool send_file = true;

	int clk_cnt = TIME_OUT;
	clock_t start = clock();

	setbuf(stdout, NULL);
	printf ("\nTrying to reset port %s and search bootloader on [1eaf:0002] for 5 seconds\n", portName);
	do
	{
		if ( send_file && GetComPort("1eaf", "0002", portName) == true )
		{
			delay(20); // essential for the port to be ready

			if ( SendFile(portName, filePtr, f_size, pageSize) == false )
			{
				printf(" ERROR: could not write file to port %s\n", portName);
				free(filePtr);
				return -1;
			}
			printf("done.\n");
			send_file = false;
			break;
		}

		if ( send_magic && SendMagic(portName) == true )
		{
			send_magic = false;
			// reset timeout
			clk_cnt = TIME_OUT;
			start = clock();
		}

		if (clock()>=(start+1000))
		{
			start = clock();
			printf(".");
			clk_cnt--;
		}
	}
	while (clk_cnt); // try x seconds long

	if ( clk_cnt==0 )
	{
		printf ("\n ERROR: Could not establish communication with the device. Giving up.\n");
		return -1;
	}

	printf ("Upload finished.\n");
    return 0;
}
