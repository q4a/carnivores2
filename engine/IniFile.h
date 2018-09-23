
#ifndef INIFILE_H
#define INIFILE_H

#include <string>
#include <vector>


class IniKey
{
public:
	std::string	mName;
	std::string	mValue;

	IniKey();
	IniKey( std::string Name, std::string Value );
	~IniKey();
};

class IniGroup
{
public:
	std::string			mName;
	std::vector<IniKey>	mKeys;

	IniGroup();
	IniGroup( std::string Name );
	~IniGroup();
	std::string FindKey( std::string Name );
};


class IniFile
{
private:
	std::vector<IniGroup>	mGroups;
	std::string				mFileName;

public:

	IniFile();
	~IniFile();

	bool	File( std::string FileName );

	bool	ReadKeyBool( std::string Group, std::string Key, bool Default );
	int		ReadKeyInt( std::string Group, std::string Key, int Default );
	float	ReadKeyFloat( std::string Group, std::string Key, float Default );
	std::string	ReadKeyString( std::string Group, std::string Key, std::string Default );

	void	WriteKeyBool( std::string Group, std::string Key, bool Value );
	void	WriteKeyInt( std::string Group, std::string Key, int Value );
	void	WriteKeyFloat( std::string Group, std::string Key, float Value );
	void	WriteKeyString( std::string Group, std::string Key, std::string Value );
};

#endif // INIFILE_H
