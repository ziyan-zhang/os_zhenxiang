#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "stdio.h"
#include "memory.h"
#include "fs.h"
#include "string.h"
#include "dir.h"

// 以下的include是为了调试
#include "fs.h"
#include "ide.h"
#include "inode.h"
#include "bitmap.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int main(void) {
	put_str("I am kernel\n");
	init_all();
   process_execute(u_prog_a, "u_prog_a");
   process_execute(u_prog_b, "u_prog_b");
   thread_start("k_thread_a", 31, k_thread_a, "I am thread a");
   thread_start("k_thread_b", 31, k_thread_b, "I am thread b");

   uint32_t fd;
   fd = sys_open("/file1", O_CREAT);
   printf("opened with create fd: %d\n", fd);
   sys_close(fd);

   fd = sys_open("/file1", O_RDWR);
   printf("opened with read/write fd0:%d\n", fd);
   sys_write(fd, "hello,world\n", 12);
   sys_close(fd);

   printf("/dir1/subdir1 create %s\n",\
      sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
   printf("/dir1 create %s!\n",\
      sys_mkdir("/dir1") == 0 ? "done" : "fail");
   printf("now, /dir1/subdir1 create %s\n", \
      sys_mkdir("/dir1/subdir1") == 0 ? "done" : "fail");
   fd = sys_open("/dir1/subdir1/file2", O_CREAT|O_RDWR);
   if (fd != -1) {
      printf("/dir1/subdir1/file2 create done!\n");
      sys_write(fd, "Catch me if you can!\n", 21);
      sys_lseek(fd, 0, SEEK_SET);
      char buf[32] = {0};
      sys_read(fd, buf, 21);
      printf("/dir1/subdir1/file2 says:\n%s", buf);
      sys_close(fd);
   }

   /* 测试打开和关闭目录 */
   struct dir* p_dir = sys_opendir("/dir1/subdir1");
   if (p_dir) {
      printf("/dir1/subdir1 open done!\n");
      if (sys_closedir(p_dir) == 0) {
         printf("/dir1/subdir1 close done\n");
      } else {
         printf("/dir1/subdir1 close fail\n");
      }
   } else {
      printf("/dir/subdir1 open fail\n");
   }

   /* 测试读取目录项 */
   p_dir = sys_opendir("/dir1/subdir1");
   if (p_dir) {
      printf("/dir1/subdir1 open done!\ncontent:\n");
      char* type = NULL;
      struct dir_entry* dir_e = NULL;
      while ((dir_e = sys_readdir(p_dir))) {
         if (dir_e->f_type == FT_REGULAR) {
            type = "regular";
         } else {
            type = "directory";
         }
         printf("  %s  %s\n", type, dir_e->filename);
      }
      if (sys_closedir(p_dir) == 0) {
         printf("/dir1/subdir1 close done!\n");
      } else {
         printf("/dir1/subdir1 close fail!\n");
      }
   } else {
      printf("/dir1/subdir1 open fail!\n");
   }

   /* 测试代码删除目录 */
   printf("/dir1 content before delete /dir/subdir1:\n");
   struct dir* dir = sys_opendir("/dir1/");
   char* type = NULL;
   struct dir_entry* dir_e = NULL;
   while (dir_e = sys_readdir(dir)) {
      if (dir_e->f_type == FT_REGULAR) {
         type = "regular";
      } else {
         type = "directory";
      }
      printf("  %s  %s\n", type, dir_e->filename);
   }
   printf("try to delete no-enpty directory /dir1/subdir1\n");
   if (sys_rmdir("/dir1/subdir1") == 1) {
      printf("sys_rmdir: /dir1/subdir1 delete fail!\n");
   }

   printf("try to delete /dir1/subdir1/file2\n");
   if (sys_rmdir("/dir1/subdir1/file2") == -1) {
      printf("sys_rmdir: /dir1/subdir1/file2 fail!\n");
   }
   if (sys_unlink("/dir1/subdir1/file2") == 0) {
      printf("sys_unlink: /dir1/subdir1/file2 delete done\n");
   }

   printf("try to delete directory /dir1/subdir1 again!\n");
   if (sys_rmdir("/dir1/subdir1") == 0) {
      printf("/dir1/subdir1 delete done!\n");
   }

   printf("/dir1 content after delete /dir1/subdir1: \n");
   sys_rewinddir(dir);
   while ((dir_e = sys_readdir(dir))) {
      if (dir_e->f_type == FT_REGULAR) {
         type = "regular";
      } else {
         type = "directory";
      }
      printf("  %s  %s\n", type, dir_e->filename);
   }

   printf("dir '/' content:\n");
   dir = sys_opendir("/");
   type = NULL;
   dir_e = NULL;
   while (dir_e = sys_readdir(dir)) {
      if (dir_e->f_type == FT_REGULAR) {
         type = "regular";
      } else {
         type = "directory";
      }
      printf("  %s  %s\n", type, dir_e->filename);
   }

   /* 测试获取当前目录和更改当前目录 */
   char cwd_buf[32] = {0};
   sys_getcwd(cwd_buf, 32);
   printf("cwd:%s\n", cwd_buf);
   sys_chdir("/dir1");
   printf("change cwd now\n");
   sys_getcwd(cwd_buf, 32);
   printf("cwd:%s\n", cwd_buf);

   /* 测试获得文件属性 */
   struct stat obj_stat;
   sys_stat("/", &obj_stat);
   printf("/s inode_info\n ino:%d\n size:%d\n filetype:%s\n",\
   obj_stat.st_ino, obj_stat.st_size, obj_stat.st_filetype==2 ? "directory" : "regular");
   sys_stat("/dir1", &obj_stat);
   printf("/dir1's info\n i_no:%d\n size:%d\n filetype:%s\n",\
   obj_stat.st_ino, obj_stat.st_size, obj_stat.st_filetype==2 ? "directory" : "regular");

   while(1);
   return 0;
}

// int main(void) {
// 	put_str("I am kernel\n");
// 	init_all();
//    uint32_t fd;
//    fd = sys_open("/file1", O_CREAT);
//    printf("opened with create fd: %d\n", fd);
//    sys_close(fd);

//    uint32_t fd0 = sys_open("/file1", O_RDWR);
//    printf("opened with read/write fd0:%d\n", fd0);
//    sys_write(fd0, "vvvvvv", 6);

//    uint32_t fd1 = sys_open("/file1", O_RDWR);
//    printf("opened with read/write fd1:%d\n", fd1);
//    sys_write(fd1, "hello,", 6);

//    // sys_close(fd2);
//    sys_close(fd1);
//    sys_close(fd0);


//    uint32_t fd2 = sys_open("/file1", O_RDWR);
//    char buf[64] = {0};
//    int read_bytes = sys_read(fd2, buf, 12);
//    printf("fd2_ read %d bytes:\n%s\n", read_bytes, buf);

//    sys_close(fd2);
//    // sys_close(fd1);

//    uint32_t fd3 = sys_open("/file1", O_RDWR);
//    printf("open /file1, fd3:%d\n", fd3);
//    char buf2[64] = {0};
//    read_bytes = sys_read(fd3, buf2, 12);
//    printf("fd3_ read %d bytes:\n%s\n", read_bytes, buf2);
//    sys_close(fd3);

//    // printf("%d closed now\n", fd);
//    while(1);
//    return 0;
// }

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
   void* addr1 = sys_malloc(256);
   void* addr2 = sys_malloc(255);
   void* addr3 = sys_malloc(254);
   console_put_str(" thread_a malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
   while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
   void* addr1 = sys_malloc(256);
   void* addr2 = sys_malloc(255);
   void* addr3 = sys_malloc(254);
   console_put_str(" thread_b malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
   while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
   void* addr1 = malloc(256);
   void* addr2 = malloc(255);
   void* addr3 = malloc(254);
   printf(" prog_a malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   free(addr1);
   free(addr2);
   free(addr3);
   while(1);
}

/* 测试用户进程 */
void u_prog_b(void) {
   void* addr1 = malloc(256);
   void* addr2 = malloc(255);
   void* addr3 = malloc(254);
   printf(" prog_b malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   free(addr1);
   free(addr2);
   free(addr3);
   while(1);
}

   //todo 几个打印函数，内核，用户态的
