#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
// for debug
#include <stdio.h>

#include "utils.h"


#define MAX_PATH_LEN 256

static char * ScanDir(char *searchPath, const char *searched)
{
	char * result = NULL;
	struct dirent *dirEnt;
	DIR *dirHandle = opendir(searchPath);
	
	if (!dirHandle)
		return 0;

	// check if file is in current directory
	char newpath[MAX_PATH_LEN + 1];
	memcpy(newpath, searchPath, sizeof(newpath) - 1);
	newpath[sizeof(newpath) - 1] = 0;
	strncat(newpath, "/", sizeof(newpath) - strlen(newpath) - 1);
	strncat(newpath, searched, sizeof(newpath) - strlen(newpath) - 1);
	fprintf(stderr, "checking : %s\n", newpath);
	int rc = access(newpath, F_OK);
	if (rc >= 0) {
		// if file is found return it's path
		result = (char *)malloc(strlen(newpath) + 1);
		strcpy(result, newpath);
		return result;
	}

	// check into sub directories
	while ((dirEnt = readdir(dirHandle)) != NULL && !result)
	{
		if (dirEnt->d_type == DT_DIR)
		{
			if (dirEnt->d_name[0] == '.' || dirEnt->d_name[0] == '_')
				continue;

			memcpy(newpath, searchPath, sizeof(newpath) - 1);
			newpath[sizeof(newpath) - 1] = 0;
			strncat(newpath, "/", sizeof(newpath) - strlen(newpath) - 1);
			strncat(newpath, dirEnt->d_name, sizeof(newpath) - strlen(newpath) - 1);
			result = ScanDir(newpath, searched);
			if (result) break;
		}
	}
	closedir(dirHandle);
	return result;
}

char *findFile(char *file, char * searchpaths)
{
	char *singlepath;
	char *result = NULL;

	for (singlepath = strtok(searchpaths, ":"); singlepath && *singlepath && !result; singlepath = strtok(NULL, ":"))
		result = ScanDir(singlepath, file);

	return result;
}