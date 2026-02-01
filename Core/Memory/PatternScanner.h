// TODO: @timmie replace with ur offset finder or whatever lol

#include "../Common.h"
#include "../../../external/memcury/memcury.h"

class PatternScanner
{
public:
	static PatternScanner* Get();
public:
	uintptr_t FindGetEngineVersion();
};