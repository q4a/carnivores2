
#ifndef H_CONSOLE_H
#define H_CONSOLE_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

class AFLog
{
private:
	FILE*	_log;
public:
	AFLog( ) : _log(stderr) {}
	AFLog( FILE* f ) : _log(f) {}
	~AFLog() {}

	void indent()
	{
		++m_indents;
	}

	void undent()
	{
		if ( m_indents > 0 ) --m_indents;
	}

	size_t	log( const char* p_str )
	{
		if ( !p_str ) return 0;

		__writeIndents();
		return fprintf( _log, "%s", p_str );
	}

	size_t	error( const char* p_error, uint32_t p_line, const char* p_file )
	{
		if ( !p_error || !p_file || !p_line ) return 0;
		size_t len = 0;

		__writeIndents();
		len += fprintf( _log, "ERROR!\n" );
		__writeIndents();
		len += fprintf( _log, "\t%s\nOn Line: %u\nIn File: %s\n", p_error, p_line, p_file );

		return len;
	}

	size_t	warning( const char* p_warn, uint32_t p_line, const char* p_file )
	{
		if ( !p_warn || !p_file || !p_line ) return 0;
		size_t len = 0;

		__writeIndents();
		len += fprintf( _log, "WARNING!\n" );
		__writeIndents();
		len += fprintf( _log, "\t%s\nOn Line: %u\nIn File: %s\n", p_warn, p_line, p_file );

		return len;
	}

protected:
	uint32_t	m_indents;

	void __writeIndents()
	{
		for ( uint32_t i=0; i<m_indents; ++i )
		{
			fputc( '\t', _log ); //Faster than a bloody fprintf call
		}
	}
};

#endif
