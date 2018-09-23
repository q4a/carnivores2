
#ifndef H_AFSTRING_H
#define H_AFSTRING_H

#include <string.h>
#include <stdint.h>

template <typename T = char> class string
{
protected:
	T* 			m_data;
	uint32_t	m_length; // Number of characters/elements (includes NULL)

public:

	typedef T char_type;

	// Default Constructor
	string()
	{
		m_data = 0;
		m_length = 0;
	}

	// Copy Constructor
	string( const string& str )
	{
		m_data = new T [ str.m_length ];
		memcpy( m_data, str.m_data, str.m_length * sizeof(T) );
		m_length = str.m_length;
	}
	// Copy C-Style String Constructor
	string( const char* str )
	{
		m_length = strlen(str) + 1;
		m_data = new T[ m_length ];
		memcpy( m_data, str, m_length * sizeof(T) );
	}

	// Destructor
	~string()
	{
		if (m_data != 0)
		delete [] m_data;
	}

	inline operator T* (void)
	{
		return m_data;
	}

	inline string& operator = (const T *str) // done
	{
		if ( !str ) return *this;

		if ( m_data )
		{
			delete [] m_data;
		}

		m_length = strlen(str) + 1;
		m_data = new T[ m_length ];
		memcpy( m_data, str, m_length * sizeof(T) );
		return *this;
	}

	inline string& operator = (const string &str) // done
	{
		this->string( str ); // copy construct
		return *this;
	}

	inline string& operator + ( const string &str ) // done
	{
		if ( !m_data )
		{
			T *temp = 0;

			temp = new T [ m_length + str.m_length - 1 ]; // allocate a new string

			memcpy( temp, m_data, ( m_length - 1 ) * sizeof( T ) ); // copy the original into the new

			delete [] m_data;
			m_data = temp;

			memcpy( *((T*)m_data + ( m_length-1 ), str.m_data, str.m_length * sizeof( T ) ); // Copy the second half in

			m_length = m_length + str.m_length - 1;
		}
		return *this;
	}

	inline string& operator + ( const T *str )
	{
		if ( !m_data )
		{
			T* temp = new T [ m_length + strlen( str ) ];
			memcpy( temp, m_data, m_length * sizeof( T ) ); // Copy old string in
			delete [] this->m_data; // Free the old memory

			memcpy(

			this->m_length += strlen( str );
			this->m_data = temp; // Transfer the pointer over
		}
		return *this;
	}

	inline void operator += (const char *str)
	{
		if ( !m_data )
		{
			char *s2;
			s2 = new char[strlen(this->s)];
			strcpy(s2,s);

			delete [] s;
			s = new char[strlen(str)+strlen(s2)];

			strcpy(this->s,s2);
			strcat(this->s,str);
			delete [] s2;
		}
		return;
	}
	inline void operator += (string &str)
	{
		if (s != NULL)
		{
			char *s2;
			s2 = new char[strlen(s)];
			strcpy(s2,s);

			delete [] s;
			s = new char[strlen(str.s)+strlen(s2)];

			strcpy(s,s2);
			strcat(s,str.s);
			delete [] s2;
		}
		return;
	}
	//--OPERATORS -- EVALUATIONS
	inline bool operator == (const char *str)
	{
		if (strcmp(this->s,str)==0)
			return true;
		else
			return false;
	}
	inline bool operator == (string &str)
	{
		if ( !memcmp( m_data, str.m_data, m_length * sizeof(T) ) )
			return true;
		else
			return false;
	}
	inline bool operator != (const char *str)
	{
		if ( strlen(str) != (m_length-1) ) return false;

		if ( !memcmp( m_data, str, m_length * sizeof(T) ) )
			return false;
		else
			return true;
	}
	inline bool operator != (string &str)
	{
		if ( m_length != str.m_length ) return false;

		if ( !memcmp( m_data, str.m_data, m_length * sizeof(T) ) )
			return false;
		else
			return true;
	}

	/*
		Slice the string up to one of the characters, copy the sliced portion into the output buffer
		then resize this string
	*/
	T* 			slice( T* p_str, T* p_chars );

	/*
		Size of the string in bytes, including NULL
	*/
	uint32_t	size( void );

	/*
		Length of the string in characters
	*/
	uint32_t	length( void );
};

T* string::slice( T* p_str, T* p_chars )
{

}

#endif
