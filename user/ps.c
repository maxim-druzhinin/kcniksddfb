#include "stddef.h"
#include "kernel/types.h"  // typedefs
#include "user/user.h"  // syscalls
#include "kernel/param.h"
#include "kernel/process_info.h"
#include "kernel/riscv.h"
#include "kernel/syscall.h"


void print_syscall_name(int x) {
  if (x == SYS_fork) printf("fork");
  else if (x == SYS_exit) printf("exit");
  else if (x == SYS_wait) printf("wait");
  else if (x == SYS_pipe) printf("pipe");
  else if (x == SYS_read) printf("read");
  else if (x == SYS_kill) printf("kill");
  else if (x == SYS_exec) printf("exec");
  else if (x == SYS_fstat) printf("fstat");
  else if (x == SYS_chdir) printf("chdir");
  else if (x == SYS_dup) printf("dup");
  else if (x == SYS_getpid) printf("getpid");
  else if (x == SYS_sbrk) printf("sbrk");
  else if (x == SYS_sleep) printf("sleep");
  else if (x == SYS_uptime) printf("uptime");
  else if (x == SYS_open) printf("open");
  else if (x == SYS_write) printf("write");
  else if (x == SYS_mknod) printf("mknod");
  else if (x == SYS_unlink) printf("unlink");
  else if (x == SYS_link) printf("link");
  else if (x == SYS_mkdir) printf("mkdir");
  else if (x == SYS_close) printf("close");
  else if (x == SYS_dummy) printf("dummy");
  else if (x == SYS_ps_list) printf("ps_list");
  else if (x == SYS_ps_info) printf("ps_info");
  else if (x == SYS_ps_pt0) printf("ps_pt0");
  else if (x == SYS_ps_pt1) printf("ps_pt1");
  else if (x == SYS_ps_pt2) printf("ps_pt2");
  else if (x == SYS_ps_copy) printf("ps_copy");
  else if (x == SYS_ps_sleep_write) printf("ps_sleep_write");
  else printf("unknown syscall: %d", x);
}


void print_pte_info(int ind, uint64 pte, int v) {

    if (!(PTE_V && pte)) {
        if (v == 0) {
            printf("%d\n", ind);
            printf("%x\n", PTE2PA(pte));
            printf("INVALID\n");
        }
    } else {
        printf("%d\n", ind);
        printf("%x\n", PTE2PA(pte));
        if (PTE_R && pte) {
            printf("READABLE ");
        }
        if (PTE_W && pte) {
            printf("WRITIBLE ");
        }
        if (PTE_X && pte) {
            printf("EXECUTABLE ");
        }
        if (PTE_U && pte) {
            printf("USER-ACCESS ");
        }
        printf("\n");
    }

}


void
main(int argc, char* argv[]) {

    if (argc < 2) {
    
        printf("usage:\n");
        printf("- ps count\n");
        printf("- ps pids\n");
        printf("- ps list\n");
        printf("- ps pt 0 <pid> [-v]\n");
        printf("- ps pt 1 <pid> <address> [-v]\n");
        printf("- ps pt 2 <pid> <address> [-v]\n");
        printf("- ps dump <pid> <address> <size>\n");
        printf("- ps sleep-write <pid>\n");
       
        exit(0);
    }

    // =================== ps count ===================
    if (!strcmp(argv[1], "count")) {

        if (argc != 2) {
            printf("incorrect arguments for ps count\n");
            exit(1);
        }

        int proc_cnt = ps_list(-1, NULL);

        if (proc_cnt == -1) {
            printf("error\n");
            exit(-1);
        } else {
            printf("%d\n", proc_cnt);
        }

    }

    // =================== ps pids ===================
    else if (!strcmp(argv[1], "pids")) {

        if (argc != 2) {
            printf("incorrect arguments for ps pids\n");
            exit(1);
        }

        int pids[NPROC];
        for (int i = 0; i < NPROC; ++i) {
            pids[i] = -1;
        }

        int proc_cnt = ps_list(NPROC, pids);
        if (proc_cnt == -1) {
            printf("ps_list: internal error\n");
            exit(-1);
        } else {
            printf("total: %d\n", proc_cnt);
            for (int i = 0; i < proc_cnt; ++i) {
                printf("%d ", pids[i]);
            }
            printf("\n");
        }

    }

    // =================== ps list ===================
    else if (!strcmp(argv[1], "list")) {

        if (argc != 2) {
            printf("incorrect arguments for ps list\n");
            exit(1);
        }

        int my_pids[NPROC];
        for (int i = 0; i < NPROC; ++i) {
            my_pids[i] = -1;
        }

        int proc_cnt = ps_list(NPROC, my_pids);

        for (int i = 0; i < proc_cnt; ++i) {

            struct process_info psinfo = {};
            int res = ps_info(my_pids[i], &psinfo);

            if (res == -1) {
                printf("ps_info: cannot get info about pid = %d\n\n", my_pids[i]);
            }
            
            else if (res == -2) {
                printf("ps_info: process pid %d is unused at the moment\n", i);
            }

            else {

                printf("info about pid = %d:\n", my_pids[i]);
                printf("state = %s\n", psinfo.state);
                printf("parent_id = %d\n", psinfo.parent_pid);
                printf("mem_size = %d bytes\n", psinfo.mem_size);
                printf("files_count = %d\n", psinfo.files_count);
                printf("proc_name = %s\n", psinfo.proc_name);
                printf("proc_ticks = %d\n", psinfo.proc_ticks);
                printf("run_time = %d\n", psinfo.run_time);
                printf("context_switches = %d\n", psinfo.context_switches);
                printf("ps_info return value = %d\n", res);
                printf("\n");

            }
        }

    }

    // =================== ps pt ... ===================
    else if (!strcmp(argv[1], "pt")) {

        if (argc < 3) {
            printf("incorrect arguments for ps pt\n");
            exit(1);
        }

        // =================== ps pt 0 ===================
        if (!strcmp(argv[2], "0")) {

            if ((argc != 4 && argc != 5) || (argc == 5 && strcmp(argv[4], "-v"))) {
                printf("incorrect arguments for ps pt\n");
                exit(1);
            }

            int limit_entries = 512;

            uint64* pt = (uint64*) malloc(limit_entries * 8);
            
            if (pt == 0) {
                printf("cannot allocate enough memory for pagetable dump\n");
                exit(-1);
            }

            int pid = atoi(argv[3]);
            int v = (argc == 5) ? 1 : 0;

            int res = ps_pt0(pid, pt);


            if (res == 0) {
                for (int i = 0; i < limit_entries; ++i) {
                    print_pte_info(i + 1, pt[i], v);
                }
            } else {
                printf("ps_pt0: internal error\n");
            }

            free(pt);

        }

        // =================== ps pt 1/2 ===================
        else if ((!strcmp(argv[2], "1")) || (!strcmp(argv[2], "2"))) {

            int level = atoi(argv[2]);

            if ((argc != 5 && argc != 6) || (argc == 6 && strcmp(argv[5], "-v"))) {
                printf("incorrect arguments for ps pt\n");
                exit(1);
            }

            int limit_entries = 512;

            uint64* pt = (uint64*) malloc(limit_entries * 8);
            
            if (pt == 0) {
                printf("cannot allocate enough memory for pagetable dump\n");
                exit(-1);
            }


            int pid = atoi(argv[3]);
            uint64 addr = atoi(argv[4]);
            int v = (argc == 6) ? 1 : 0;
            printf("v = %d\n", v);

            int res = 0;
            if (level == 1) {
                res = ps_pt1(pid, pt, (void*) addr);
            } else {
                res = ps_pt2(pid, pt, (void*) addr);
            }


            if (res == 0) {
                for (int i = 0; i < limit_entries; ++i) {
                    print_pte_info(i + 1, pt[i], v);
                }
            } else {
                printf("ps_pt%d: internal error\n", atoi(argv[2]));
            }

            free(pt);


        }

        // =================== unknown sub-cmd in ps pt ===================
        else {

            printf("unknown command: ps pt %s\n", argv[2]);
            exit(1);

        }


    }

    // =================== ps dump ===================
    else if (!strcmp(argv[1], "dump")) {
        
        if (argc != 5) {
            printf("incorrect arguments for ps dump\n");
            exit(1);
        }
        
        int pid = atoi(argv[2]);
        uint64 addr = atoi(argv[3]);
        int size = atoi(argv[4]);
        
        char* data = (char*) malloc(size);
        if (data == 0) {
            printf("cannot allocate enough memory for memory dump\n");
            exit(-1);
        }
        
        int res = ps_copy(pid, (void*) addr, size, (void*) data);
        
        if (res == 0) {
            for (int i = 0; i < size; i++) {
              printf("%x ", data[i]);
              if (i % 16 == 0) {
                  printf("\n");
              }
            }
            printf("\n");
        }
        else {
            printf("ps_copy: internal error\n");
        }
        
        free(data);
        
    }
    
    // =================== ps sleep-write ===================
    else if (!strcmp(argv[1], "sleep-write")) {
      
        if (argc != 3) {
            printf("incorrect arguments for ps sleep-write\n");
            exit(1);
        }
        
        int limit_size = 1024;  // simplified the task here
        int pid = atoi(argv[2]);
        char* data = (char*) malloc(limit_size);
        
        int syscall = ps_sleep_write(pid, (void*) data);
        
        if (syscall == 0) {
            printf("the process in not asleep\n");
        }
        else if (syscall == -2) {
            printf("pid not found\n");
        }
        else if (syscall == -3) {
            printf("pid is not assigned to any process at the moment\n");
        }
        else if (syscall > 0) {
            printf("the process fell asleep on syscall ");
            print_syscall_name(syscall);
            printf("\n");
            if (syscall == SYS_write) {  // write
            
              int fd = *((int*) data);
              data += sizeof(int);
              printf("file descriptor: %d\n", fd);
              
              int buf_size = *((int*) data);
              data += sizeof(int);
              printf("buffer size: %d\n", buf_size);
              
             for (int i = 0; i < buf_size / 16; ++i) {
               
               for (int j = 0; j < 16; ++j) {
                 printf("%s", data[j]);
               }
               
               printf(" ");
               
               for (int j = 0; j < 16; ++j) {
                 printf("%x", data[j]);
                 if (j % 4 == 0) {
                   printf(" ");
                 }
               }
               
               printf("\n");
               
               data += 16 * sizeof(char);
             }
              
            }
        }
        else {
            printf("ps_sleep_write: internal error\n");
        }
        
        free(data);
    
    }
    
    
    // =================== unknown cmd ===================
    else {

        printf("unknown command: ps %s\n", argv[1]);
        exit(1);

    }

    exit(0);

}
