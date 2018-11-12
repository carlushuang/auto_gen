
#include <unistd.h>
#include <sys/stat.h>

namespace feifei
{
	void ensure_directory(const char * dir)
	{
		if (access(dir, F_OK) == -1)
		{
			::mkdir(dir, 0777);
		}
	}
}
