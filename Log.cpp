//
// Basic log class
//
// Author: Alexey V. Boreskov, <steps3d@gmail.com>, <steps3d@narod.ru>
//

#include	<iostream>
#include	<fstream>
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

	std::string	temp = s.str ();	// get string from stream
		
	s.str ( std::string () );		// clear stream
	
	puts ( temp.c_str () );
	
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
