#include "utils.hpp"

#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <libgen.h>

static int found_file(void *closure, const char *path, size_t length)
{
	*(char**)closure = strndup(path, length);
	return 1;
}

char *findFile(const char *file, rp_path_search_t *searchpaths)
{
	char *result = NULL;
	if (file[0] == '/')
		result = strdup(file);
	else
		rp_path_search_find(searchpaths, file, found_file, &result);
	return result;
}

#define MAX_LINE_LEN        1024
#define MAX_PATH_LEN        512

// Check if master DCF require Upload Files(slave configuration SDO binary file)
// and make symbolic links to them in working directory so master can find them.
int fixDcfRequires(const char *dcfFile)
{

	int rc = 0;
	FILE *ifp;
	char buf[MAX_LINE_LEN+1];
	char key[MAX_LINE_LEN+1];
	char slaveBinPath[2*MAX_PATH_LEN+2];
	char cwd[MAX_PATH_LEN+1];
	char *dcfPath = strdup(dcfFile);

	dirname(dcfPath);

	// Get working directory path
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		strcpy(cwd, ".");

	// Open dcf file
	ifp = fopen(dcfFile, "r");
	if (ifp == NULL)
	{
		perror("error occured at opening DCF file");
		rc = -1;
	}
	else
	{
		// Scan the file
		while (!feof(ifp))
		{
			// read the file line by line
			fgets(buf, sizeof(buf), ifp);
			// printf("%s", buf);

			if (sscanf(buf, "UploadFile = %s", key) == 1)
			{
				// get the file name
				snprintf(slaveBinPath, sizeof slaveBinPath, "%s/%s", dcfPath, key);
				snprintf(buf, sizeof buf, "%s/%s", cwd, key);

				// If the link already exists remove it
				if (access(buf, F_OK) >= 0)
					if (unlink(buf))
						perror("unlink error");

				// Create a symbolic link to the slave bin file.
				if (symlink(slaveBinPath, buf) != 0){
					perror("Symlink error");
				}
				break;
			}
		}
		fclose(ifp);
	}

	free(dcfPath);
	return rc;
}

void cleanDcfRequires()
{
	unlink("*.bin");
}
