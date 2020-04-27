#------------------------------------------------------------------------------
# making an EXE from this script: 
# https://gallery.technet.microsoft.com/scriptcenter/PS2EXE-GUI-Convert-e7cb69d5
#
# https://devcentral.f5.com/s/articles/powershell-abcs-f-is-for-format-operator
#
# Serial methods and properties:
# https://docs.microsoft.com/en-us/dotnet/api/system.io.ports.serialport.datareceived?view=netframework-4.8
#
#------------------------------------------------------------------------------

if ($args.length -eq 0) {
	write-output "ERROR: No input parameter, expected is the binary file to upload."
	return
}
#write "Input params: $args"
# define global variables
[byte[]] $myheader = @(0x41, 0xBE, 0, 0, 0, 0, 0, 0)
[int] $hLen = 8
$myCom = ""
$myVid = ""
$myPid = ""

#------------------------------------------------------------------------------
function Send-Header
{ param($port, [byte]$headId, [byte]$pNum, [uint16]$bufSize)

	$myheader[2] = $headId
	$myheader[3] = $pNum
	$myheader[4] = ($bufSize%256) # lobyte
	$myheader[5] = [Math]::Truncate($bufSize/256) # hibyte
	# calculate CRC
	[uint16] $crc = 0
	for ($j=0; $j -lt ($hLen-2); $j++)
	{
		[byte] $val = $myheader[$j]
		#write "element nr: $j, crc: $crc, val: $val"
		$crc += $val
	}
	$crc = $crc -bxor 0xFFFF
	#'0x{0:X4},' -f $crc # check CRC
	$myheader[6] = ($crc%256) #lo_byte
	$myheader[7] = [Math]::Truncate($crc/256) #hi_byte
	# write the header
	#write "Header: $myheader"
	$port.Write($myheader, 0, $hLen)
}

#------------------------------------------------------------------------------
function Wait-For-Answer
{ param ( $port)
	#write "<wait for answer from the device>"
	$counter = 0
	[int] $aLen = 0
	[byte[]] $rdArray = [byte[]]::new(8)
	while($counter -lt 10)
	{
		while ( ($rd = $port.bytestoread) -gt 0)
		{
			#$rd = $port.ReadByte() # ReadChar() will not work beause of Encoding settings after writing byte 
			#$rdArray[$aLen++] = $rd
			
			$rd = $port.Read($rdArray, $aLen, 8)
			$aLen += $rd
		
			if ($aLen -ge 8) { break }
		}
		if ( $aLen -gt 0 ) { break }
		$counter += 1
		#write-output "trying counter $counter"
		Start-Sleep -milliseconds 10
	}
	#$aLen
	# check if response was an error
	if ( $aLen -eq 1 )
	{
		# it is probably an error message. handle it here
		$myByte = $rdArray[0]
		if ( $myByte -eq 0 ) {
			write "ERROR: unexpected reply 0 before page $i"
		} elseif ( $myByte -eq 1 ) {
			write "ERROR: missing setup packet."
		} elseif ( $myByte -eq 2 ) {
			# no free buffer, means that we usually have to wait a bit
			# however, if it is page 0 then it is an error
			if ( $i -eq 0 ) {
				write "ERROR: no free buffer for page $i ???"
			} else {
				# try again the same header
				write "> trying again page $i header..."
				$i --
				Start-Sleep -milliseconds 10
				continue
			}
		} elseif ( $myByte -eq 3 ) {
			write "ERROR: data overflow at page $i. Try to upload again"
		} elseif ( $myByte -eq 4 ) {
			write "ERROR: data underflow at page $i. Try to upload again"
		} elseif ( $myByte -eq 5 ) {
			write "ERROR: wrong command length at page $i"
		} elseif ( $myByte -eq 6 ) {
			write "ERROR: wrong command CRC at page $i"
		} else {
			write "ERROR: unexpected byte $myByte at page $i"
		}
		write "Close 1"
		$port.Close()
		#return
	}
	elseif ( $aLen -ne $hLen ) # check for header length
	{
		write "ERROR: unexpected reply of $aLen bytes after page $i"
		$port.Close()
		#return
	}
	elseif ( ($rdArray[0..7] -join "") -ne ($myheader -join "") )
	{
		$rep = $rdArray[0..7]
		write "ERROR: wrong echo after page $i containing $aLen bytes: $rep"
		$port.Close()
		#return
	}
	#$aLen
}

#------------------------------------------------------------------------------
function Get-Com-Port
{ param ( $inParam )
	$tok = $inParam.split(":")
	#$len = $tok.length
	#write-output "Arguments: $len"
	$myVid = $tok[0]
	$myPid = $tok[1]
	write-host "Searching for device having VID: $myVid, PID: $myPid..."

	$counter = 50
	while($counter -gt 0)
	{
		# detect available serial ports
		$pList = Get-WmiObject win32_serialport | Select-Object DeviceID, PNPDeviceID | Out-String

		# parse the right VID:PID
		$pattern = "(COM[1-9]+).+VID_$myVid([0-9A-F]*)\&PID_$myPid([0-9A-F]*)*"
		#single match
		$result = $pList -match $pattern
		$matchCnt = $matches.length
		if ( ($result) -and ( $matchCnt -gt 0 ) ) {
			#write-output "Cnt: $matchCnt, 0: $match0, 1: $match1, 2: $match2, 3: $match3"
			$myCom = $matches[1]
			break
		}
		$counter -= 1
		#write-output "trying counter $counter"
		Start-Sleep -milliseconds 100
	}
	if ( $counter -eq 0 ) {
	write-host "No COM port found having VID:$myVid, PID:$myPid. Check your connections."
	return
	}
	write-host "Found as $myCom. Trying to open it..."
	$myCom
}
#------------------------------------------------------------------------------
function Do-Main
{ param ( $inParam1, $inParam2 )

if ( 1 ) {
#	$myCom = Get-Com-Port $inParam1
	$myCom = Get-Com-Port "1eaf:0004"

	if ($myCom -eq "") {
		return
	}
	# if we got so far, then COM port was succesfuly identified. Let's establish connections
	try {
		$port = new-Object System.IO.Ports.SerialPort $myCom,230400,None,8,one
		$port.DtrEnable = $true
		$port.open()
	}
	catch {
		write "ERROR: Serial port $myCom could not be opened!"
		return
	}
	write "$myCom opened succesfully. Reseting..."
	# generate negative DTR edge

#	write-host "DTR -"
	$port.RtsEnable = true
#	Start-Sleep -milliseconds 3000
	Start-Sleep -milliseconds 10

#	write-host "DTR +"
	$port.DtrEnable = $False
	Start-Sleep -milliseconds 10

	# write the magic sequence
	$port.Write("1EAF")
#	$port.DtrEnable = $False
	Start-Sleep -milliseconds 10
	try {
		$port.Close()
	}
	catch {
		write "Port closed."
	}
	
	Start-Sleep -milliseconds 100
}
	$myCom = Get-Com-Port "1eaf:0002"

	if ($myCom -le "") {
		return
	}
	# if we got so far, then COM port was succesfuly identified. Let's establish connections
	if ($port = new-Object System.IO.Ports.SerialPort $myCom,230400,None,8,one) {
		$port.open()
	}
	else {
		write "ERROR: Serial port could not be opened!"
		return
	}
	write "$myCom opened succesfully."
	#flushing any available data
	while ($port.bytestoread -gt 0) {
		$rd = $port.ReadByte() # ReadChar() will not work beause of Encoding settings after writing byte
	}

	# TODO: take the file from input arguments
	$inFile = $inParam1
	# read the file
	write-output "Opening file $inFile..."
	#$rawFile = Get-Content -Path  -Raw
	$rawFile = [System.IO.File]::ReadAllBytes($inFile)

	$fileLen = $rawFile.length
	write "File length = $fileLen"

	$beginPtr = 0
	[int] $bufSize =0
	[uint16] $bufferSize = 1024

	[byte] $pages = [Math]::Ceiling($fileLen/$bufferSize)
	write "We have $pages pages of max. $bufferSize bytes."
	
	# send the header to provide the number of pages to the device
	Send-Header $port 0x20 $pages 0

	Wait-For-Answer $port

	if ( -not $port.IsOpen ) {
		write "Port closed."
		return
	}

	write-host -NoNewLine "Writing pages - "

	[byte] $i = 0
	# send pages iteratively
	for ($i=0; $i -lt $pages; $i++)
	{
		# remaining size ?
		if ( ($beginPtr+$bufferSize) -le $fileLen ) {
			$bufSize = $bufferSize
		} else {
			$bufSize = $fileLen - $beginPtr
		}

		Send-Header $port 0x21 $i $bufSize

		Wait-For-Answer $port

		if ( -not $port.IsOpen ) {
			write "Port closed."
			return
		}

		# go on with page upload
		[byte[]] $halad = @(0..50)
		foreach ( $elem in $halad ) {
			if ($elem -le $i) { $elem = "*" }
			else { $elem = "_" }
		}
		write-host -NoNewline "#"

		$endPtr = $beginPtr + $bufSize - 1
		$subStr = $rawFile[$beginPtr..$endPtr]
		$port.Write($subStr, 0, $bufSize)
		# update next slice pointer
		$beginPtr += $bufSize

#		Start-Sleep -milliseconds 10 # to separate the header from the rest of data
	}
	
	write-output "`nDone."

	if ( $port.IsOpen ) {
		$port.Close()
	}
	#write "Port closed."
}

Do-Main $args[0] $args[1]
