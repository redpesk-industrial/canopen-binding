#pragma once

extern char * findFile(char *file, char * searchpaths);
extern int fixDcfRequires(char * dcffile);
extern void cleanDcfRequires();