#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>


/*returns 1 if path given is a dir, else 0*/
int is_dir(const char*file)
{
        struct stat fs;
        if (stat(file, &fs) == 0 && S_ISDIR(fs.st_mode)) // dir
                return 1;
        return 0;
}

#define long_long long long 

/*if not a regular file returns 0*/
long_long du_file(const char* file)
{
        struct stat fs;
        if (stat(file, &fs) == 0 && S_ISREG(fs.st_mode)) // regular file
                return fs.st_size;

        return 0;
}

/*expects a path to dir as parameter*/
long_long du_dir(const char* file)
{
        DIR* dirp = opendir(file);
        struct dirent *dp = NULL;
        char *path = (char*)malloc((PATH_MAX+1)*sizeof(char));
        long long size = 0;

        if (dirp)
        {
                while( (dp=readdir(dirp)) != NULL)
                {
                        snprintf(path, PATH_MAX, "%s/%s", file, dp->d_name);
                        if (is_dir(path)) 
                        {
                                if (dp->d_name[0] == '.') 
                                        continue; //omit all . (dot) dirs 
                                size += du_dir(path);
                        }
                        else
                                size += du_file(path);
                } // while
        } // if

        free(path);
        return size;
}

long_long du_ignore_hidden_dirs(const char* file)
{
        if (is_dir(file)) 
                return du_dir(file);
        else 
                return du_file(file);
}

