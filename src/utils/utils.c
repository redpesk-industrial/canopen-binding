#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>

#include "utils.h"

#define MAX_PATH_LEN        256
#define DCF_UPLOAD_FILE_KEY "UploadFile"
#define KEY_FORMAT          DCF_UPLOAD_FILE_KEY"=%s"
#define KEYLEN              sizeof(DCF_UPLOAD_FILE_KEY)-1

static char *ScanDir(char *searchPath, const char *searched)
{
	char *result = NULL;
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
	if (rc >= 0)
	{
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
			if (result)
				break;
		}
	}
	closedir(dirHandle);
	return result;
}

char *findFile(char *file, char *searchpaths)
{
	char *singlepath;
	char *result = NULL;

	for (singlepath = strtok(searchpaths, ":"); singlepath && *singlepath && !result; singlepath = strtok(NULL, ":"))
		result = ScanDir(singlepath, file);

	return result;
}

// Check if master DCF require Upload Files(slave configuration SDO binary file)
// and make symbolic links to them in working directory so master can find them.
int fixDcfRequires(char *dcfFile)
{
	
	int rc = 0;
	FILE *ifp;
	char key[MAX_PATH_LEN];
	char buf[MAX_PATH_LEN];
	char slaveBinPaht[MAX_PATH_LEN];
	char cwd[MAX_PATH_LEN];
	char * dcfPath = (char*)malloc(sizeof(dcfFile));
	strcpy(dcfPath, dcfFile);
	dirname(dcfPath);

	// int keylen = strlen(DCF_UPLOAD_FILE_KEY);

	// Get working directory path
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		cwd[0] = '\0';

	// Open dcf file
	ifp = fopen(dcfFile, "r");
	if (ifp == NULL)
	{
		perror("error occured at opening DCF file");
		return -1;
	}
	
	
	// Scan the antier file
	while (!feof(ifp))
	{
		// read the file line by line
		fgets(buf, sizeof(buf), ifp);
		// printf("%s", buf);

		if (strncmp(buf, DCF_UPLOAD_FILE_KEY, KEYLEN) == 0)
		{
			
			// get the file name
			sscanf(buf, KEY_FORMAT, key);
			snprintf(slaveBinPaht, strlen(dcfPath) + strlen(key) + 2, "%s/%s", dcfPath, key);
			snprintf(buf, strlen(dcfPath) + strlen(key) + 2, "%s/%s", cwd, key);
			
			// If the link already exists remove it
			if (access(buf, F_OK) >= 0)
				if (unlink(buf))
					perror("unlink error");
			
			// Create a symbolic link to the slave bin file.
			if (symlink(slaveBinPaht, buf) != 0){
				perror("Symlink error");
			}
		}
	}

	rc = fclose(ifp);
	return rc;
}

void cleanDcfRequires()
{
	unlink("*.bin");
}
