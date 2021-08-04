/***************************************************************************//**
*  \file       test_app.c
*
*  \details    Userspace application to test the Device driver
*
*  \author     Bari Shamli
*
*  \Tested with Linux 5.4.0-80-generic
*
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
 
#define WR_VALUE _IOW('a','a',int32_t*)
#define RD_VALUE _IOR('a','b',int32_t*)



int8_t write_buf[1024];
int8_t read_buf[1024];

int main()
{
        int fd;
        char option;
        int32_t value, number;
        
        
        
        
        printf("*********************************\n");
        printf("*******Barishamli@gmail.com*******\n");

        fd = open("/dev/Bari_Device", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }

        while(1) {
                printf("****Please Enter the Option*******************\n");
                printf("        1. Write to kernel buff               \n");
                printf("        2. Read from kernel buff              \n");
                printf("        3. Write to kernel param (IOCTL)      \n");
                printf("        4. Read from kernel param (IOCTL)     \n");
                printf("        5. Exit                 			  \n");
                printf("**********************************************\n");
                scanf(" %c", &option);
                printf("Your Option = %c\n", option);
                
                switch(option) {
                        case '1':
                                printf("Enter the string to write into driver :");
                                scanf("  %[^\t\n]s", write_buf);
                                printf("Data Writing ...");
                                write(fd, write_buf, strlen(write_buf)+1);
                                printf("Done!\n");
                                break;
                        case '2':
                                printf("Data Reading ...");
                                read(fd, read_buf, 1024);
                                printf("Done!\n\n");
                                printf("Data = %s\n\n", read_buf);
                                break;
                                
                        case '3':
                                printf("Enter the Value to send\n");
								scanf("%d",&number);
								printf("Writing Value to Driver\n");
								ioctl(fd, WR_VALUE, (int32_t*) &number); 
								break;
								
						case '4':
								printf("Reading Value from Driver\n");
								ioctl(fd, RD_VALUE, (int32_t*) &value);
								printf("Value is %d\n", value);
								break;
                                
                        case '5':
                                close(fd);
                                exit(1);
                                break;
                        default:
                                printf("Enter Valid option = %c\n",option);
                                break;
                }
        }
        close(fd);
}
