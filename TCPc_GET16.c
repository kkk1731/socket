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
#include <pthread.h>

#define PORTNUM 8000 
#define BUFSIZE 4096
#define NAMESIZE 64
#define COMSIZE 256
#define CLIENT 16
int fd_list[CLIENT];

void *send_get(int *index){
	int i, ret;
	i = (int)*index;
	char *end_msg = "exit";
	char buf[BUFSIZE];
	char command[COMSIZE];

	memset(buf, '\0', BUFSIZE);
	memset(command, '\0', COMSIZE);
	//コマンド作成
	sprintf(command, "GET<test%d.txt>\n", i);

	//GET送信
	send(fd_list[i], command, strlen(command), 0);
	if((ret = recv(fd_list[i], buf, BUFSIZE, 0)) > 0){
		printf("fd=%d\n%s", i, buf);
	}
	//通信終了
	send(fd_list[i], end_msg, strlen(end_msg), 0);
	close(fd_list[i]);
}

int main(int argc, char **argv)
{
	struct sockaddr_in saddr;
	int i;
	pthread_t pt[16];
	int index[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
	
	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family      = AF_INET;
	saddr.sin_port        = htons(PORTNUM);
	saddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	/* make server's socket */
	for(i=0; i<CLIENT; i++){
		if((fd_list[i] = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
			perror("socket");
			return -1;
		}
	
		if (connect(fd_list[i], (struct sockaddr*)&saddr, sizeof(saddr)) < 0) {
			perror("connect");
			exit(-1);
		}
		printf("connect socket %d\n", fd_list[i]);
	}

    for(i=0; i<CLIENT; i++){	
		if (pthread_create(&pt[i], NULL, (void*)(send_get), (void*)&index[i]) != 0) {
			perror("pthread_create");
			return -1;
		}
	}
	for(i=0; i<CLIENT; i++){
		pthread_join(pt[i], NULL);
	}
	return 0;
	
}
