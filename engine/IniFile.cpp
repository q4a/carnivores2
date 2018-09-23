
#include "IniFile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

IniFile::IniFile()
{

}

IniFile::~IniFile()
{
	this->mGroups.clear();
}

IniGroup::IniGroup()
{

}

IniGroup::IniGroup( std::string Name )
{
	this->mName = Name;
}

IniGroup::~IniGroup()
{
	this->mKeys.clear();
}

IniKey::IniKey()
{

}

IniKey::IniKey( std::string Name, std::string Value )
{
	this->mName = Name;
	this->mValue = Value;
}

IniKey::~IniKey()
{

}


bool IniFile::File( std::string FileName )
{
	FILE* hFile = 0;
	//errno_t eFile = 0;

	this->mFileName = FileName;

	hFile = fopen( FileName.c_str(), "rb" );

	if ( hFile == NULL )
	{
		return false;
	}

	std::string current_group = "";

	while ( !feof(hFile) )
	{
		char line[256];
		fgets( line, 255, hFile );

		if ( line[0] == '\r' ) continue;
		if ( line[0] == '\n' ) continue;
		if ( line[0] == '\0' ) continue;
		if ( line[0] == ';' ) continue;

		if ( line[0] == '[' )
		{
			char *group = strtok( &line[1], "]\n\r \t;" );
			this->mGroups.push_back( IniGroup( group ) );
			current_group = group;
			continue;
		}

		if ( strstr( line, "=" ) )
		{
			char *key = strtok( line, "=" );
			char *val = strtok(    0, "\n\r" );
			this->mGroups[ this->mGroups.size()-1 ].mKeys.push_back( IniKey( key, val ) );
			continue;
		}
	}

	fclose( hFile );
	return true;
}

std::string IniGroup::FindKey( std::string Name )
{
	std::string Value = "(null)";

	for ( unsigned i=0; i<this->mKeys.size(); ++i )
	{
		if ( this->mKeys[i].mName == Name )
		{
			Value = this->mKeys[i].mValue;
			break;
		}
	}

	return Value;
}

int		IniFile::ReadKeyInt( std::string Group, std::string Key, int Default )
{
	int Value = Default;

	for ( unsigned i=0; i<this->mGroups.size(); ++i )
	{
		if ( this->mGroups[i].mName == Group )
		{
			std::string res = this->mGroups[i].FindKey( Key );
			if ( res != "(null)" )
			{
				Value = atoi( res.c_str() );
				break;
			}
		}
	}

	return Value;
}

float	IniFile::ReadKeyFloat( std::string Group, std::string Key, float Default )
{
	float Value = Default;

	for ( unsigned i=0; i<this->mGroups.size(); ++i )
	{
		if ( this->mGroups[i].mName == Group )
		{
			std::string res = this->mGroups[i].FindKey( Key );
			if ( res != "(null)" )
			{
				Value = (float)atof( res.c_str() );
				break;
			}
		}
	}

	return Value;
}

std::string	IniFile::ReadKeyString( std::string Group, std::string Key, std::string Default )
{
	std::string Value = Default;

	for ( unsigned i=0; i<this->mGroups.size(); ++i )
	{
		if ( this->mGroups[i].mName == Group )
		{
			std::string res = this->mGroups[i].FindKey( Key );
			if ( res != "(null)" )
			{
				Value = res;
				break;
			}
		}
	}

	return Value;
}

void	IniFile::WriteKeyInt( std::string Group, std::string Key, int Value )
{
}

void	IniFile::WriteKeyFloat( std::string Group, std::string Key, float Value )
{
}

void	IniFile::WriteKeyString( std::string Group, std::string Key, std::string Value )
{
}
