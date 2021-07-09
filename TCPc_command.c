#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORTNUM 8000 
#define BUFSIZE 4096
#define NAMESIZE 64
#define COMSIZE 256
int is_PUT(char *buf, char *filename){
	char *adr1, *adr2;
	if((adr1 = strstr(buf, "PUT<")) != NULL){
		if((adr2 = strstr(buf, ">\n")) != NULL){
			strncpy(filename, adr1+4, adr2 - adr1 - 4);
			filename[adr2 - adr1 - 4] = '\0';
			return 1;
		}
		return -1;
	}
	return 0;
}

//put protocol
//line1 :"PUT<filename>\n"
//line2~:"contents" 
void set_PUT(char *buf, char *filename){
	char cmd_cat[COMSIZE] = "cat ~/home_client/";
	char tmp[BUFSIZE];
	FILE *fp;

	strcat(cmd_cat, filename);
	if((fp = popen(cmd_cat, "r")) == NULL){
		pclose(fp);  
		perror("popen");
		return;
	}
	while(fgets(tmp, BUFSIZE, fp) != NULL){
		//printf("strlen(tmp) = %ld\n", strlen(tmp));
		strcat(buf, tmp);
	}
	//printf("buf =\n%s\n", buf);
	pclose(fp);
	return;
}

int main(int argc, char **argv)
{
	struct sockaddr_in saddr;
	int                fd, ret, cmd;
	char               buf[BUFSIZE];
	char			   filename[NAMESIZE];
	char               *end_msg = "exit";
	/* make server's socket */
	if((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family      = AF_INET;
	saddr.sin_port        = htons(PORTNUM);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	if (connect(fd, (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
		perror("connect");
		exit(-1);
	}
	printf("client fd = %d\n", fd);
	write(1,">>>",3);
    while(!(fgets(buf, BUFSIZE, stdin) == NULL || buf[0] == '\n')){
		//set PUT
		if((cmd = is_PUT(buf, filename)) >= 1){
			set_PUT(buf, filename);
		}
		
		//send
		send(fd, buf, strlen(buf), 0);

		//receive
		if ((ret = recv(fd, buf, BUFSIZE, 0)) > 0) {
			//write(1,"client receive\n",15);
			write(1,buf,ret);
		}
		memset(filename, '\0', NAMESIZE);
		memset(buf, '\0', BUFSIZE);
		printf("\n>>>");
	}
	//send exit msg
	write(1,"\nend client\n",13);
	send(fd, end_msg, strlen(end_msg), 0);

	close(fd);
	return 0;
}