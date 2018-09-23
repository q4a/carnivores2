#ifndef H_VERSION_H
#define H_VERSION_H

namespace Version{
	
	//Date Version Types
	static const char DATE[] = "27";
	static const char MONTH[] = "02";
	static const char YEAR[] = "2013";
	static const char UBUNTU_VERSION_STYLE[] = "13.02";
	
	//Software Status
	static const char STATUS[] = "Beta";
	static const char STATUS_SHORT[] = "b";
	
	//Standard Version Type
	static const long MAJOR = 3;
	static const long MINOR = 1;
	static const long BUILD = 71;
	static const long REVISION = 466;
	
	//Miscellaneous Version Types
	static const long BUILDS_COUNT = 760;
	#define RC_FILEVERSION 3,1,71,466
	#define RC_FILEVERSION_STRING "3, 1, 71, 466\0"
	static const char FULLVERSION_STRING[] = "3.1.71.466";
	
	//SVN Version
	static const char SVN_REVISION[] = "2";
	static const char SVN_DATE[] = "2013-02-16T17:17:52.254843Z";
	
	//These values are to keep track of your versioning state, don't modify them.
	static const long BUILD_HISTORY = 65;
	

}
#endif //H_VERSION_H
