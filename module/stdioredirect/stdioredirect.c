#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h> 

#define	CONSOLE_PATH		"/tmp/consolename"

int main()
{
	    char *tty = ttyname(1);
			int fd;
			    char name[64]={0};
					
					printf("tty %s \n", tty);
						fd = open(CONSOLE_PATH, O_RDWR | O_CREAT | O_TRUNC);
							
						if (fd >= 0) {
									sprintf(name, "%s", tty);
											write(fd, name, strlen(name) + 1);
													
													close(fd);
															fflush(NULL);
																
						} else {
									printf("open faild \n");
										
						}

						    return 0;

}



