#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "stat.h"

extern struct proc proc[NPROC];
extern struct spinlock wait_lock;

uint64
sys_ps_listinfo(void)
{
	struct procinfo *plist, pinfo;
    struct proc *p, *curr_proc = myproc();
    int lim = 0, pcnt = 0;

    argaddr(0, (uint64*)&plist);
	argint(1, &lim);

    if (!plist) 
	{
        for (p = proc; p < &proc[NPROC]; ++p) 
		{
			acquire(&p->lock);
			if (p->state != UNUSED && p->state != USED)
			{
				++pcnt;
			}
			release(&p->lock);
		}
		  return pcnt;
    }

    if ((uint64)plist < 0x1000 || (uint64)plist + lim * sizeof(struct procinfo) > curr_proc->sz)
    {
        return -2;
    }

    for (p = proc; p < &proc[NPROC] && pcnt < lim; ++p)
    {
        acquire(&p->lock);
        if (p->state != UNUSED && p->state != USED)
        { 
            pinfo.pid = p->pid;
            safestrcpy(pinfo.name, p->name, sizeof(pinfo.name));
            pinfo.state = p->state;
        
            acquire(&wait_lock);
            if (p->parent) 
            {
                acquire(&p->parent->lock);
                pinfo.ppid = p->parent->pid;
                release(&p->parent->lock);
            } else
            {
                pinfo.ppid = 0;
            }
            release(&wait_lock);

            if (copyout(curr_proc->pagetable, (uint64)(plist + pcnt), (char*)&pinfo, sizeof(pinfo)) < 0) 
            {
                release(&p->lock);
                return -2;
            }
            ++pcnt;
        }
        release(&p->lock);
    }

    for (; p < &proc[NPROC]; ++p) 
    {
        acquire(&p->lock);
        if (p->state != UNUSED && p->state != USED) 
        {
            release(&p->lock);
            return -1;
        }
        release(&p->lock);
    }

    return pcnt;
}