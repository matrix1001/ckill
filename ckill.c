#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <regex.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define MAX_PROC 10240
#define MAX_FILENAME 256
#define MAX_BUFF 4096
#define MAX_PROC_NAME_SIZE 256

#define SCAN_INTERVAL 1000

#define PROC_DIR "/proc"

static int PID_arr[MAX_PROC] = {0};
static void refresh_pids()
{
    memset(PID_arr, 0, sizeof(int) * MAX_PROC);
    DIR* dir = opendir(PROC_DIR);
    struct dirent* entry;
    if (!dir)
    {
        printf("Cant read proc dir\n");
        exit(EXIT_FAILURE);
    }
    int index = 0;
    while ((entry = readdir(dir)) != NULL) 
    {
        char* name = entry->d_name;
        if (name[0] < '0' || name[0] > '9') 
            continue;
        int pid = atoi(name);
        if (pid && index < MAX_PROC)
        {
            PID_arr[index++] = pid;
        }
    }
    closedir(dir);
    return;
}

static void load_name(int pid, char *name)
{
    /*read stat file*/
    char filename[MAX_FILENAME] = {0};
    snprintf(filename, MAX_FILENAME, "%s/%d/stat", PROC_DIR, pid);
    int fd = open(filename, O_RDONLY);
    if(fd == -1)
    {
        printf("failed to open %s", filename);
        return;
    }
        
    static char buf[MAX_BUFF];
    int size = read(fd, buf, MAX_BUFF-1);
    close(fd);
    buf[size] = 0;

    char *ptr_s, *ptr_e;
    ptr_s = strchr(buf, '(') + 1;
    ptr_e = strrchr(ptr_s, ')');
    if (!ptr_s || !ptr_e) 
    {
        printf("failed to name for %d", pid);
        return;
    }
    
    unsigned int name_size = ptr_e - ptr_s;
    if (name_size >= MAX_PROC_NAME_SIZE)
        name_size = MAX_PROC_NAME_SIZE - 1;
    memcpy(name, ptr_s, name_size);
    name[name_size] = 0;
}

int check_str_in_arr(const char *str, const char **arr, int len)
{
	for(int idx=0;idx<len;idx++)
		if(!strcmp(str, arr[idx]))
			return 1;
	return 0;
}
int main(int argc, char **argv)
{
	int kill_cnt = argc - 1;
	if (kill_cnt <=0 ) exit(EXIT_FAILURE);
	char **kill_names = &argv[1];
	char temp_name[MAX_PROC_NAME_SIZE];
	while(1)
	{
		usleep(SCAN_INTERVAL);
		refresh_pids();
		for(int idx=0;PID_arr[idx]!=0;idx++)
		{
			load_name(PID_arr[idx], temp_name);
			if(check_str_in_arr(temp_name, kill_names, kill_cnt))
			{
				printf("Killing %s, pid %d\n", temp_name, PID_arr[idx]);
				kill(PID_arr[idx], 9);
			}
		}
	}
}