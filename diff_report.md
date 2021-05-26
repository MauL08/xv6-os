# Assignment 2

# UID, GID, PPID

```diff
diff --git a/proc.c b/proc.c
-174,7 +188,12 @@ userinit(void)
   p->tf->ss = p->tf->ds;
   p->tf->eflags = FL_IF;
   p->tf->esp = PGSIZE;
-  p->tf->eip = 0;  // beginning of initcode.S
+  p->tf->eip = 0; // beginning of initcode.S
+
+#ifdef CS333_P2
+  p->uid = DEFAULT_UID;
+  p->gid = DEFAULT_GID;
+#endif
```

```diff
diff --git a/proc.c b/proc.c
@@ -239,14 +261,16 @@ fork(void)
   // Clear %eax so that fork returns 0 in the child.
   np->tf->eax = 0;
 
-  for(i = 0; i < NOFILE; i++)
-    if(curproc->ofile[i])
+  for (i = 0; i < NOFILE; i++)
+    if (curproc->ofile[i])
       np->ofile[i] = filedup(curproc->ofile[i]);
   np->cwd = idup(curproc->cwd);
 
   safestrcpy(np->name, curproc->name, sizeof(curproc->name));
 
   pid = np->pid;
+  np->uid = curproc->uid;
+  np->gid = curproc->gid;
 
   acquire(&ptable.lock);
   np->state = RUNNABLE;
```
```diff
index 0a0b4c5..86e95ab 100644
--- a/proc.h
+++ b/proc.h
@@ -49,6 +49,11 @@ struct proc {
   struct file *ofile[NOFILE];  // Open files
   struct inode *cwd;           // Current directory
   char name[16];               // Process name (debugging)
+  uint start_ticks;
+  uint uid;
+  uint gid;
+  uint cpu_ticks_total;        // total elapsed ticks in CPU
+  uint cpu_ticks_in;           // ticks when scheduled
 };
```
```diff
diff --git a/syscall.c b/syscall.c
index 9105b52..e32e1b2 100644
--- a/syscall.c
+++ b/syscall.c
@@ -106,32 +106,54 @@ extern int sys_uptime(void);
 #ifdef PDX_XV6
 extern int sys_halt(void);
 #endif // PDX_XV6
+#ifdef CS333_P1
+extern int sys_date(void);
+#endif
+#ifdef CS333_P2
+extern int sys_getuid(void);
+extern int sys_getgid(void);
+extern int sys_getppid(void);
+extern int sys_setuid(void);
+extern int sys_setgid(void);
+extern int sys_getprocs(void);
+#endif
```
```diff
+#ifdef CS333_P2
+  [SYS_getuid]  sys_getuid,
+  [SYS_getgid]  sys_getgid,
+  [SYS_getppid] sys_getppid,
+  [SYS_setuid]  sys_setuid,
+  [SYS_setgid]  sys_setgid,
+  [SYS_getprocs] sys_getprocs
+#endif
 };
```
```diff
@@ -157,6 +179,12 @@ static char *syscallnames[] = {
   [SYS_link]    "link",
   [SYS_mkdir]   "mkdir",
   [SYS_close]   "close",
+  [SYS_getuid]  "getuid",
+  [SYS_getgid]  "getgid",
+  [SYS_getppid] "getppid",
+  [SYS_setuid]  "setuid",
+  [SYS_setgid]  "setgid",
+  [SYS_getprocs]  "getprocs"
 #ifdef PDX_XV6
   [SYS_halt]    "halt",
 #endif // PDX_XV6
```
```diff
diff --git a/syscall.h b/syscall.h
index 7fc8ce1..a7fd353 100644
--- a/syscall.h
+++ b/syscall.h
@@ -22,3 +22,10 @@
 #define SYS_close   SYS_mkdir+1
 #define SYS_halt    SYS_close+1
 // student system calls begin here. Follow the existing pattern.
+#define SYS_date    SYS_halt+1
+#define SYS_getuid  SYS_date+1
+#define SYS_getgid  SYS_getuid+1
+#define SYS_getppid SYS_getgid+1
+#define SYS_setuid  SYS_getppid+1
+#define SYS_setgid  SYS_setuid+1
+#define SYS_getprocs SYS_setgid+1
```
```diff
@@ -97,3 +100,79 @@ sys_halt(void)
   return 0;
 }
 #endif // PDX_XV6
+#ifdef CS333_P1
+int
+sys_date(void)
+{
+  struct rtcdate *d;
+
+  if(argptr(0, (void*)&d, sizeof(struct rtcdate)) < 0)
+  {
+    return -1;
+  }
+
+  cmostime(d);
+
+  return 0;
+}
+#endif
+#ifdef CS333_P2
+int
+sys_getuid(void)
+{
+  return myproc()->uid; 
+}
+int
+sys_getgid(void)
+{
+  return myproc()->gid;
+}
+int
+sys_getppid(void)
+{
+  if(myproc()->pid == 1)
+  {
+    return myproc()->pid;
+  }
+  return myproc()->parent->pid;
+}
+int
+sys_setuid(void)
+{
+  int tmp;
+
+  if(argint(0,&tmp) < 0 || tmp < 0 || tmp > 32767)
+  {
+    return -1;
+  }
+
+  myproc()->uid = (uint)tmp;
+  return 0;
+}
+int
+sys_setgid(void)
+{
+  int tmp;
+
+  if(argint(0,&tmp) < 0 || tmp < 0 || tmp > 32767)
+  {
+    return -1;
+  }
+
+  myproc()->gid = (uint)tmp;
+  return 0;
+}
+#endif
+#ifdef CS333_P2
+int
+sys_getprocs(void)
+{
+  int max;
+  struct uproc* up;
+
+  if (argint(0, &max) < 0 || argptr(1, (void*)&up, sizeof(struct uproc) * max) < 0)
+    return -1;
+
+  return copy_proc(max, up);
+}
+#endif // CS333_P2
```
```diff
diff --git a/user.h b/user.h
index 31d9134..9bbacff 100644
--- a/user.h
+++ b/user.h
@@ -43,3 +43,14 @@ int atoi(const char*);
 int atoo(const char*);
 int strncmp(const char*, const char*, uint);
 #endif // PDX_XV6
+#ifdef CS333_P1
+int date(struct rtcdate*);
+#endif // CS333_P1
+#ifdef CS333_P2
+uint getuid(void);
+uint getgid(void);
+uint getppid(void);
+int setuid(uint);
+int setgid(uint);
+int getprocs(uint, struct uproc*);
+#endif // CS333_P2
```
```diff
diff --git a/usys.S b/usys.S
index 0d4eaed..7c0ca74 100644
--- a/usys.S
+++ b/usys.S
@@ -30,3 +30,10 @@ SYSCALL(sbrk)
 SYSCALL(sleep)
 SYSCALL(uptime)
 SYSCALL(halt)
+SYSCALL(date)
+SYSCALL(getuid)
+SYSCALL(getgid)
+SYSCALL(getppid)
+SYSCALL(setuid)
+SYSCALL(setgid)
+SYSCALL(getprocs)
```
# Process Execution Time
```diff
diff --git a/proc.c b/proc.c
@@ -137,25 +147,29 @@ allocproc(void)
 
   // Leave room for trap frame.
   sp -= sizeof *p->tf;
-  p->tf = (struct trapframe*)sp;
+  p->tf = (struct trapframe *)sp;
 
   // Set up new context to start executing at forkret,
   // which returns to trapret.
   sp -= 4;
-  *(uint*)sp = (uint)trapret;
+  *(uint *)sp = (uint)trapret;
 
   sp -= sizeof *p->context;
-  p->context = (struct context*)sp;
+  p->context = (struct context *)sp;
   memset(p->context, 0, sizeof *p->context);
   p->context->eip = (uint)forkret;
 
+  p->start_ticks = ticks;
+
+  p->cpu_ticks_in = 0;
+  p->cpu_ticks_total = 0;
+
   return p;
 }
```
```diff
@@ -414,32 +446,31 @@ scheduler(void)
 // be proc->intena and proc->ncli, but that would
 // break in the few places where a lock is held but
 // there's no process.
-void
-sched(void)
+void sched(void)
 {
   int intena;
   struct proc *p = myproc();
 
-  if(!holding(&ptable.lock))
+  if (!holding(&ptable.lock))
     panic("sched ptable.lock");
-  if(mycpu()->ncli != 1)
+  if (mycpu()->ncli != 1)
     panic("sched locks");
-  if(p->state == RUNNING)
+  if (p->state == RUNNING)
     panic("sched running");
-  if(readeflags()&FL_IF)
+  if (readeflags() & FL_IF)
     panic("sched interruptible");
   intena = mycpu()->intena;
+  p->cpu_ticks_total += ticks - (p->cpu_ticks_in);
   swtch(&p->context, mycpu()->scheduler);
   mycpu()->intena = intena;
 }
```
```diff
@@ -357,41 +387,42 @@ wait(void)
 //  - swtch to start running that process
 //  - eventually that process transfers control
 //      via swtch back to the scheduler.
-void
-scheduler(void)
+void scheduler(void)
 {
   struct proc *p;
   struct cpu *c = mycpu();
   c->proc = 0;
 #ifdef PDX_XV6
-  int idle;  // for checking if processor is idle
-#endif // PDX_XV6
+  int idle; // for checking if processor is idle
+#endif      // PDX_XV6
 
-  for(;;){
+  for (;;)
+  {
     // Enable interrupts on this processor.
     sti();
 
 #ifdef PDX_XV6
-    idle = 1;  // assume idle unless we schedule a process
-#endif // PDX_XV6
+    idle = 1; // assume idle unless we schedule a process
+#endif        // PDX_XV6
     // Loop over process table looking for process to run.
     acquire(&ptable.lock);
-    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
-      if(p->state != RUNNABLE)
+    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
+    {
+      if (p->state != RUNNABLE)
         continue;
 
-      // Switch to chosen process.  It is the process's job
-      // to release ptable.lock and then reacquire it
-      // before jumping back to us.
+        // Switch to chosen process.  It is the process's job
+        // to release ptable.lock and then reacquire it
+        // before jumping back to us.
 #ifdef PDX_XV6
-      idle = 0;  // not idle this timeslice
-#endif // PDX_XV6
+      idle = 0; // not idle this timeslice
+#endif          // PDX_XV6
       c->proc = p;
       switchuvm(p);
       p->state = RUNNING;
+      p->cpu_ticks_in = ticks;
       swtch(&(c->scheduler), p->context);
       switchkvm();
-
       // Process is done running for now.
       // It should have changed its p->state before coming back.
       c->proc = 0;
```
```diff
index 0a0b4c5..86e95ab 100644
--- a/proc.h
+++ b/proc.h
@@ -49,6 +49,11 @@ struct proc {
   struct file *ofile[NOFILE];  // Open files
   struct inode *cwd;           // Current directory
   char name[16];               // Process name (debugging)
+  uint start_ticks;
+  uint uid;
+  uint gid;
+  uint cpu_ticks_total;        // total elapsed ticks in CPU
+  uint cpu_ticks_in;           // ticks when scheduled
 }; 
```

# "ps" Command
```diff
diff --git a/defs.h b/defs.h
index f85557d..aa976f8 100644
--- a/defs.h
+++ b/defs.h
@@ -4,6 +4,7 @@ struct file;
 struct inode;
 struct pipe;
 struct proc;
+struct uproc;
 struct rtcdate;
 struct spinlock;
 struct sleeplock;
@@ -124,6 +125,9 @@ void            userinit(void);
 int             wait(void);
 void            wakeup(void*);
 void            yield(void);
+#ifdef CS333_P2
+int             copy_proc(int, struct uproc*);
+#endif
 #ifdef CS333_P3
 void            printFreeList(void);
 void            printList(int);
```

```diff
diff --git a/proc.c b/proc.c
index d030537..b7fb645 100644
--- a/proc.c
+++ b/proc.c
@@ -6,27 +6,32 @@
 #include "x86.h"
 #include "proc.h"
 #include "spinlock.h"
+#include "uproc.h"
+#ifdef CS333_P2
+#include "pdx.h"
+#endif
```

```diff
@@ -606,28 +707,78 @@ procdump(void)
     cprintf("%d\t%s\t%s\t", p->pid, p->name, state);
 #endif
 
-    if(p->state == SLEEPING){
-      getcallerpcs((uint*)p->context->ebp+2, pc);
-      for(i=0; i<10 && pc[i] != 0; i++)
+    if (p->state == SLEEPING)
+    {
+      getcallerpcs((uint *)p->context->ebp + 2, pc);
+      for (i = 0; i < 10 && pc[i] != 0; i++)
         cprintf(" %p", pc[i]);
     }
     cprintf("\n");
   }
 #ifdef CS333_P1
-  cprintf("$ ");  // simulate shell prompt
-#endif // CS333_P1
+  cprintf("$ "); // simulate shell prompt
+#endif           // CS333_P1
 }
+#ifdef CS333_P2
+// Helper function to access ptable for sys_getprocs
+int copy_proc(int max, struct uproc *up)
+{
+  int tmp = 0;
+  struct proc *tp;
+  acquire(&ptable.lock);
+
+  for (tp = ptable.proc; tp < &ptable.proc[NPROC]; tp++)
+  {
+    if (tmp == max)
+      break;
+    if (tp->state == UNUSED || tp->state == EMBRYO)
+    {
+      continue;
+    }
+    else if (tp->state == SLEEPING || tp->state == RUNNABLE || tp->state == RUNNING || tp->state == ZOMBIE)
+    {
+      up[tmp].pid = tp->pid;
+      up[tmp].uid = tp->uid;
+      up[tmp].gid = tp->gid;
+
+      // Handle init PPID
+      if (tp->pid == 1)
+      {
+        up[tmp].ppid = tp->pid;
+      }
+      else
+      {
+        up[tmp].ppid = tp->parent->pid;
+      }
+
+      up[tmp].elapsed_ticks = ticks - tp->start_ticks;
+      up[tmp].CPU_total_ticks = tp->cpu_ticks_total;
+      safestrcpy(up[tmp].state, states[tp->state], sizeof(up[tmp].state));
+      up[tmp].size = tp->sz;
+      safestrcpy(up[tmp].name, (char *)tp->name, sizeof(tp->name));
+
+      tmp++;
+    }
+  }
+
+  release(&ptable.lock);
+  return tmp;
+}
+#endif // CS333_P2
```
```diff
diff --git a/runoff.list b/runoff.list
index 81930d9..a4b56b6 100644
--- a/runoff.list
+++ b/runoff.list
@@ -92,3 +92,5 @@ p2-test.c
 p3-test.c
 p4-test.c
 testsetprio.c
+ps.c
+time.c
```
```diff
diff --git a/syscall.c b/syscall.c
index 9105b52..e32e1b2 100644
--- a/syscall.c
+++ b/syscall.c
@@ -106,32 +106,54 @@ extern int sys_uptime(void);
 #ifdef PDX_XV6
 extern int sys_halt(void);
 #endif // PDX_XV6
+#ifdef CS333_P1
+extern int sys_date(void);
+#endif
+#ifdef CS333_P2
+extern int sys_getuid(void);
+extern int sys_getgid(void);
+extern int sys_getppid(void);
+extern int sys_setuid(void);
+extern int sys_setgid(void);
+extern int sys_getprocs(void);
+#endif

+#ifdef CS333_P2
+  [SYS_getuid]  sys_getuid,
+  [SYS_getgid]  sys_getgid,
+  [SYS_getppid] sys_getppid,
+  [SYS_setuid]  sys_setuid,
+  [SYS_setgid]  sys_setgid,
+  [SYS_getprocs] sys_getprocs
+#endif
 };
```
```diff
@@ -157,6 +179,12 @@ static char *syscallnames[] = {
   [SYS_link]    "link",
   [SYS_mkdir]   "mkdir",
   [SYS_close]   "close",
+  [SYS_getuid]  "getuid",
+  [SYS_getgid]  "getgid",
+  [SYS_getppid] "getppid",
+  [SYS_setuid]  "setuid",
+  [SYS_setgid]  "setgid",
+  [SYS_getprocs]  "getprocs"
 #ifdef PDX_XV6
   [SYS_halt]    "halt",
 #endif // PDX_XV6
```
```diff
diff --git a/syscall.h b/syscall.h
index 7fc8ce1..a7fd353 100644
--- a/syscall.h
+++ b/syscall.h
@@ -22,3 +22,10 @@
 #define SYS_close   SYS_mkdir+1
 #define SYS_halt    SYS_close+1
 // student system calls begin here. Follow the existing pattern.
+#define SYS_date    SYS_halt+1
+#define SYS_getuid  SYS_date+1
+#define SYS_getgid  SYS_getuid+1
+#define SYS_getppid SYS_getgid+1
+#define SYS_setuid  SYS_getppid+1
+#define SYS_setgid  SYS_setuid+1
+#define SYS_getprocs SYS_setgid+1
```
```diff
diff --git a/sysproc.c b/sysproc.c
index 98563ea..a32457c 100644
--- a/sysproc.c
+++ b/sysproc.c
@@ -6,6 +6,9 @@
 #include "memlayout.h"
 #include "mmu.h"
 #include "proc.h"
+#ifdef CS333_P2
+#include "uproc.h"
+#endif
 #ifdef PDX_XV6
 #include "pdx-kernel.h"
 #endif // PDX_XV6
```
```diff
@@ -97,3 +100,79 @@ sys_halt(void)
   return 0;
 }
 #endif // PDX_XV6
+#ifdef CS333_P1
+int
+sys_date(void)
+{
+  struct rtcdate *d;
+
+  if(argptr(0, (void*)&d, sizeof(struct rtcdate)) < 0)
+  {
+    return -1;
+  }
+
+  cmostime(d);
+
+  return 0;
+}
+#endif
+#ifdef CS333_P2
+int
+sys_getuid(void)
+{
+  return myproc()->uid; 
+}
+int
+sys_getgid(void)
+{
+  return myproc()->gid;
+}
+int
+sys_getppid(void)
+{
+  if(myproc()->pid == 1)
+  {
+    return myproc()->pid;
+  }
+  return myproc()->parent->pid;
+}
+int
+sys_setuid(void)
+{
+  int tmp;
+
+  if(argint(0,&tmp) < 0 || tmp < 0 || tmp > 32767)
+  {
+    return -1;
+  }
+
+  myproc()->uid = (uint)tmp;
+  return 0;
+}
+int
+sys_setgid(void)
+{
+  int tmp;
+
+  if(argint(0,&tmp) < 0 || tmp < 0 || tmp > 32767)
+  {
+    return -1;
+  }
+
+  myproc()->gid = (uint)tmp;
+  return 0;
+}
+#endif
+#ifdef CS333_P2
+int
+sys_getprocs(void)
+{
+  int max;
+  struct uproc* up;
+
+  if (argint(0, &max) < 0 || argptr(1, (void*)&up, sizeof(struct uproc) * max) < 0)
+    return -1;
+
+  return copy_proc(max, up);
+}
+#endif // CS333_P2
```
```diff
diff --git a/user.h b/user.h
index 31d9134..9bbacff 100644
--- a/user.h
+++ b/user.h
@@ -43,3 +43,14 @@ int atoi(const char*);
 int atoo(const char*);
 int strncmp(const char*, const char*, uint);
 #endif // PDX_XV6
+#ifdef CS333_P1
+int date(struct rtcdate*);
+#endif // CS333_P1
+#ifdef CS333_P2
+uint getuid(void);
+uint getgid(void);
+uint getppid(void);
+int setuid(uint);
+int setgid(uint);
+int getprocs(uint, struct uproc*);
+#endif // CS333_P2
```
```diff
diff --git a/usys.S b/usys.S
index 0d4eaed..7c0ca74 100644
--- a/usys.S
+++ b/usys.S
@@ -30,3 +30,10 @@ SYSCALL(sbrk)
 SYSCALL(sleep)
 SYSCALL(uptime)
 SYSCALL(halt)
+SYSCALL(date)
+SYSCALL(getuid)
+SYSCALL(getgid)
+SYSCALL(getppid)
+SYSCALL(setuid)
+SYSCALL(setgid)
+SYSCALL(getprocs)
```
```diff
diff --git a/ps.c b/ps.c
+#ifdef CS333_P2
+
+#define MAX_ENTRIES 16
+
+#include "types.h"
+#include "user.h"
+#include "uproc.h"
+
+int
+main(int argc, char *argv[])
+{
+  struct uproc* value;
+
+  value = malloc(sizeof(struct uproc) * MAX_ENTRIES);
+
+  if (getprocs(MAX_ENTRIES, value) < 0) {
+    printf(2,"Error: getprocs call failed. %s at line %d\n",
+        __FILE__, __LINE__);
+    exit();
+  }
+
+  printf(1, "\nPID\tName\t\tUID\tGID\tPPID\tElapsed\tCPU\tState\tSize\n");
+
+  for (int i = 0; i < MAX_ENTRIES; i++){
+    if (value[i].pid == NULL)
+    break;
+    printf(1, "%d\t%s\t\t%d\t%d\t%d\t", 
+      value[i].pid,
+      value[i].name,
+      value[i].uid,
+      value[i].gid,
+      value[i].ppid
+    );
+
+   // Special case for floating point
+    // Elapsed Ticks
+    if (value[i].elapsed_ticks < 10){
+      printf(1, "0.00%d\t", value[i].elapsed_ticks);
+    } else if (value[i].elapsed_ticks < 100){
+      printf(1, "0.0%d\t", value[i].elapsed_ticks);
+    } else if (value[i].elapsed_ticks < 1000){
+      printf(1, "0.%d\t", value[i].elapsed_ticks);
+    }else{
+      printf(1, "%d.%d\t", value[i].elapsed_ticks/1000, value[i].elapsed_ticks%1000);
+    }
+
+    // CPU Total Ticks
+    if (value[i].CPU_total_ticks < 10){
+      printf(1, "0.00%d", value[i].CPU_total_ticks);
+    } else if (value[i].CPU_total_ticks < 100){
+      printf(1, "0.0%d", value[i].CPU_total_ticks);
+    } else if (value[i].CPU_total_ticks < 1000){
+      printf(1, "0.%d", value[i].CPU_total_ticks);
+    }else{
+      printf(1, "%d.%d", value[i].CPU_total_ticks/1000, value[i].CPU_total_ticks%1000);
+    }
+
+    printf(1, "\t%s\t%d\n",
+      value[i].state,
+      value[i].size
+    );
+  }
+
+  printf(1, "\n");
+  exit();
+}
+
+#
```
# "time" Command
```diff
diff --git a/runoff.list b/runoff.list
index 81930d9..a4b56b6 100644
--- a/runoff.list
+++ b/runoff.list
@@ -92,3 +92,5 @@ p2-test.c
 p3-test.c
 p4-test.c
 testsetprio.c
+ps.c
+time.c
```
```diff
diff --git a/time.c b/time.c
+#include "types.h"
+#include "user.h"

+int
+main(int argc, char *argv[])
+{
+  int current_time = uptime();

+ if (argc == 1){
+      int leftover = 0;
+      int time_now = uptime() - current_time;
+      if(time_now > 1000){
+        leftover = time_now % 1000;
+        time_now /= 1000;
+    }
+      if(leftover != 0){
+        printf(1, "(null) ran in %d.%d seconds\n", time_now, leftover);
+    } else{
+        printf(1, "(null) ran in 0.%d seconds\n", time_now);
+     }
+    } else {
+        if (fork() == 0){
+            exec(argv[1], &argv[1]);
+        }else{
+            wait();
+             int leftover = 0;
+             int time_now = uptime() - current_time;
+            
+            if(time_now > 1000){
+            leftover = time_now % 1000;
+            time_now /= 1000;
+            }
+            if(leftover != 0){
+            printf(1, "%s ran in %d.%d seconds\n", argv[1], time_now, leftover);
+            } else{
+            printf(1, "%s ran in 0.%d seconds\n", argv[1], time_now);
+           }
+        }
+   }
+
+  exit();
+}
```
# Modifying Console
```diff
@@ -553,23 +587,89 @@ kill(int pid)
 // No lock to avoid wedging a stuck machine further.
 
 #if defined(CS333_P2)
-void
-procdumpP2P3P4(struct proc *p, char *state_string)
+void procdumpP2P3P4(struct proc *p, char *state_string)
 {
-  cprintf("TODO for Project 2, delete this line and implement procdumpP2P3P4() in proc.c to print a row\n");
+  if (p->pid == 1)
+  {
+    cprintf("%d\t%s\t\t%d\t%d\t%d\t",
+            p->pid,
+            p->name,
+            p->uid,
+            p->gid,
+            p->pid);
+  }
+  if (p->pid != 1)
+  {
+    cprintf("%d\t%s\t\t%d\t%d\t%d\t",
+            p->pid,
+            p->name,
+            p->uid,
+            p->gid,
+            p->parent->pid);
+  }
+
+  // Elapsed
+  if ((ticks - (p->start_ticks)) < 10)
+  {
+    cprintf("0.00%d\t", (ticks - (p->start_ticks)));
+  }
+  else if ((ticks - (p->start_ticks)) < 100)
+  {
+    cprintf("0.0%d\t", (ticks - (p->start_ticks)));
+  }
+  else if ((ticks - (p->start_ticks)) < 1000)
+  {
+    cprintf("0.%d\t", (ticks - (p->start_ticks)));
+  }
+  else
+  {
+    cprintf("%d.%d\t", (ticks - (p->start_ticks)) / 1000, (ticks - (p->start_ticks)) % 1000);
+  }
+
+  // CPU ticks
+  if (p->cpu_ticks_total < 10)
+  {
+    cprintf("0.00%d", p->cpu_ticks_total);
+  }
+  else if (p->cpu_ticks_total < 100)
+  {
+    cprintf("0.0%d", p->cpu_ticks_total);
+  }
+  else if (p->cpu_ticks_total < 1000)
+  {
+    cprintf("0.%d", p->cpu_ticks_total);
+  }
+  else
+  {
+    cprintf("%d.%d", p->cpu_ticks_total / 1000, p->cpu_ticks_total % 1000);
+  }
+
+  cprintf("\t%s\t%d\t",
+          states[p->state],
+          p->sz);
+
   return;
 }
```
