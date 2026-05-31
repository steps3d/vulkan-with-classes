//
// Basic log class
//
// Author: Alexey V. Boreskov, <steps3d@gmail.com>, <steps3d@narod.ru>
//

#include	<iostream>
#include	<fstream>
#include	<iomanip>
#include	<ctime>
#include	"Log.h"

#ifdef	_WIN32
#include	<windows.h>
#endif

static	Log	appLog ( "" );		// create application log
static	Log	debLog ( "", true );	// create debug log
Log::endl__	Log::endl;			// creat end-of-line marker

Log& Log::flush ()
{
	if ( skip )
		return *this;

	if ( isWarning && skipWarning )
		return *this;

	if ( isInfo && skipInfo )
		return *this;

	if ( showTime )
	{
		auto t  = std::time(nullptr);
		auto tm = *std::localtime(&t);

		std::cout << std::put_time(&tm, "%d-%m-%Y %H-%M-%S ");
	}

	std::string	temp = s.str ();	// get string from stream
		
	s.str ( std::string () );		// clear stream
	
	std::cout << temp;
	
	if ( !logName.empty () )
	{
		std::ofstream file;

		file.open ( logName, std::ios::app );
		file << temp << std::endl;
	}
		
#ifdef	_WIN32
	OutputDebugString ( temp.c_str () );
	OutputDebugString ( "\n" );
#endif
	return *this;
}

Log& log ( int level )
{
	return appLog;
}

Log& fatal ()
{
	return appLog << Log::fatal__ ();
}

Log&	debug ()
{
	return debLog;
}

Log& info ()
{
	return appLog << Log::info__ ();
}

Log& warning ()
{
	return appLog << Log::warning__ ();
}

