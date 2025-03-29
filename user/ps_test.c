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

int
main(int argc, char *argv[])
{
    // Test 1: plist = NULL
    int cnt = ps_listinfo(0, 0);
    printf("Number of processes: %d\n", cnt);

    // Test 2: making a list of processes
    struct procinfo *plist = malloc(cnt * sizeof(struct procinfo));
    int ret = ps_listinfo(plist, cnt);
    if (ret < 0) 
 	{
        fprintf(2, "Error: %d\n", ret);
    } else 
 	{
        printf("Got %d processes:\n", ret);
        for (int i = 0; i < ret; ++i)
 		{
            printf("pid: %d, name: %s, state: %s, ppid: %d\n",
				plist[i].pid, plist[i].name, state_to_str(plist[i].state), plist[i].ppid);
        }
    }
    free(plist);
 
    // Test 3: small buffer
    struct procinfo small_plist[1];
    ret = ps_listinfo(small_plist, 1);
    if (ret < 0) 
 	{
        fprintf(2, "Small buffer error: %d (expected if processes > 1)\n", ret);
    } else 
 	{
        fprintf(2, "Unexpected success with small buffer\n");
    }
 
    // Test 4: invalid address
    ret = ps_listinfo((struct procinfo*)0xFFFFFFFF, 10);
    if (ret < 0) 
 	{
    	fprintf(2, "Invalid address error: %d\n", ret);
    } else 
 	{
        fprintf(2, "Unexpected success with invalid address\n");
    }
 
    exit(0);
}