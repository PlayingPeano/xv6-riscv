#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

const char* state_to_str(int state) 
{
    switch(state) 
    {
        case 0:  return "UNUSED";
        case 1:  return "USED";
        case 2:  return "SLEEPING";
        case 3:  return "RUNNABLE";
        case 4:  return "RUNNING";
        case 5:  return "ZOMBIE";
        default: return "UNKNOWN";
    }
}

char*
get_pname(struct procinfo *plist, int cnt, int ppid)
{
    for (int i = 0; i < cnt; ++i) 
	{
        if (plist[i].pid == ppid)
		{
            return plist[i].name;
		}
    }
    return "unknown";
}

int
main(int argc, char *argv[])
{
    int cnt = ps_listinfo(0, 0);
    if (cnt < 0) 
	{
        fprintf(2, "Error getting process cnt\n");
        exit(0);
    }

    struct procinfo *plist = malloc(cnt * sizeof(struct procinfo));
    int ret = ps_listinfo(plist, cnt);
    if (ret < 0) 
	{
        fprintf(2, "Error getting process list: %d\n", ret);
        free(plist);
        exit(0);
    }

    for (int i = 0; i < ret; ++i)
	{
        char *pname = get_pname(plist, ret, plist[i].ppid);
        printf("pid: %d, name: %s, state: %s, ppid: %d, PNAME:%s\n",
			plist[i].pid, plist[i].name, state_to_str(plist[i].state), plist[i].ppid, pname);
    }

    free(plist);
    exit(0);
}