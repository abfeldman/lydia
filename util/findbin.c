#include "findbin.h"
#include "strndup.h"
#include "strdup.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
# include <unistd.h>
#else
# include <direct.h>
#endif
#include <limits.h>
#include <stdio.h>

#ifndef PATH_MAX
# define PATH_MAX 4096
#endif
#ifdef WIN32
# define DIR_SEPARATOR '\\'
# define PATH_SEPARATOR ";"
#else
# define DIR_SEPARATOR '/'
# define PATH_SEPARATOR ":"
#endif

char *findbin(const char *bin, int up)
{
    static char result[PATH_MAX + 1];
    static char initial[PATH_MAX + 1];
    static char tmp[PATH_MAX + 1];

    int i;
    char *q, *p;
    size_t t;

    getcwd(initial, PATH_MAX);
    strncpy(result, bin, PATH_MAX);
    p = strrchr(result, DIR_SEPARATOR);
    if (NULL != p) {
        *p = '\0';
        chdir(result);
    } else { /* Check the path. */
        q = p = strdup(getenv("PATH"));
        if (NULL != p) {
            p = strtok(p, PATH_SEPARATOR);
            while (p != NULL) {
                struct stat st;

                strncpy(tmp, p, PATH_MAX);
                t = strlen(tmp);

                if (t > 0 && tmp[t - 1] != DIR_SEPARATOR) {
                    tmp[t] = '/';
                    tmp[t + 1] = '\0';
                }
                strncat(tmp, bin, PATH_MAX);
                memset(&st, 0, sizeof(struct stat));
                stat(tmp, &st);
#ifdef WIN32
                if (st.st_mode & _S_IEXEC) {
#else
                if (!S_ISDIR(st.st_mode) && 
                    ((st.st_mode & S_IXOTH) ||
                     ((st.st_mode & S_IXGRP) && (st.st_uid == getgid())) ||
                     ((st.st_mode & S_IXUSR) && (st.st_gid == getuid())))) {
#endif
                    strncpy(result, p, PATH_MAX);
                    chdir(result);
                    break;
                }
                p = strtok(NULL, ":");
            }
            free(q);
        }
    }
    for (i = 0; i < up; i++) {
        chdir("..");
    }
    getcwd(result, PATH_MAX);
    chdir(initial);

    return result;
}

static void addslash(char *buf)
{
    size_t t = strlen(buf);

    if (t > 0 && buf[t - 1] != DIR_SEPARATOR) {
        buf[t] = DIR_SEPARATOR;
        buf[t + 1] = '\0';
    }
}

char *canonify(const char *prefix, const char *file)
{
    static char result[PATH_MAX + 1];

    if (NULL == file) {
        return NULL;
    }

    if (file[0] == DIR_SEPARATOR) {
        strncpy(result, file, PATH_MAX);
        return result;
    }

    strncpy(result, prefix, PATH_MAX);
    addslash(result);
    strncat(result, file, PATH_MAX);
    return result;
}
