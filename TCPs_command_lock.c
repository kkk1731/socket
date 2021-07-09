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
#define COMSIZE 256
#define NAMESIZE 64

pthread_mutex_t lock;

//get command
int get_cmd(char *buf, char *filename){
	char *adr1, *adr2;
	if((adr1 = strstr(buf, "GET<")) != NULL){
		if((adr2 = strstr(buf, ">\n")) != NULL){
			strncpy(filename, adr1+4, adr2 - adr1 - 4);
			filename[adr2 - adr1 - 4] = '\0';
			//printf("filename=%s\n", filename);
			return 0;
		}
		return -1;
	}
	//PUT<filename> buf内容：1行目PUT<filename>, 2行目以降内容
	else if((adr1 = strstr(buf, "PUT<")) != NULL){
		if((adr2 = strstr(buf, ">\n")) != NULL){
			strncpy(filename, adr1+4, adr2 - adr1 - 4);
			filename[adr2 - adr1 - 4] = '\0';
			return 1;
		}
		return -1;
	}
	//DEL<filename>
	else if((adr1 = strstr(buf, "DEL<")) != NULL){
		if((adr2 = strstr(buf, ">\n")) != NULL){
			strncpy(filename, adr1+4, adr2 - adr1 - 4);
			filename[adr2 - adr1 - 4] = '\0';
			return 2;
		}
		return -1;
	}
	//LS<path>
	else if((adr1 = strstr(buf, "LS<")) != NULL){//LS<>\n
		if((adr2 = strstr(buf, ">\n")) != NULL){
			strncpy(filename, adr1+3, adr2 - adr1 - 3);
			filename[adr2 - adr1 - 3] = '\0';
			return 3;
		}
		return -1;
	}
	
    return -1;
}


void GET(char *filename, int fd){
	char buf_send[BUFSIZE] = "FILE(";
    char tmp[BUFSIZE];
    char cmd_wc[COMSIZE] = "wc -c < ~/home_server/";
    char cmd_cat[COMSIZE] = "cat ~/home_server/";
    FILE *fp;
	memset(tmp, '\0', BUFSIZE);
    //file size
    strcat(cmd_wc, filename);
	if((fp = popen(cmd_wc, "r")) == NULL){
		pclose(fp);  
		perror("popen");
        send(fd, "err popen\n", 10, 0);
		return;
	}
	while(fgets(tmp, BUFSIZE, fp) != NULL){
		strncat(buf_send, tmp, strlen(tmp) - 1);
	}

	//not exist file
	if(strlen(tmp) <= 1){
		pclose(fp);
		send(fd, "NOT FOUND\n", 10, 0);
		return;
	}
	
    pclose(fp);
    strcat(buf_send, "):");

    //file content
    strcat(cmd_cat, filename);
	if((fp = popen(cmd_cat, "r")) == NULL){    
    	send(fd, "error cat\n", 10, 0);
		return;
	}
	while(fgets(tmp, BUFSIZE, fp) != NULL){
		strcat(buf_send, tmp);
	}
    pclose(fp);
            
	//send
    send(fd, buf_send, BUFSIZE, 0);
    return;
}

void PUT(char *buf, char *filename, int fd){
	char buf_send[BUFSIZE];
    char tmp[BUFSIZE];
    char cmd_echo[BUFSIZE + COMSIZE] = "echo \"";
	char cmd_success[BUFSIZE] = "PUT success\n";
    FILE *fp;

	memset(buf_send, '\0', BUFSIZE);
	memset(tmp, '\0', BUFSIZE);
	if(strlen(buf) <= strlen(filename) + 6){
		printf("NOT FOUND\n");
		send(fd, "NOT FOUND\n", 10, 0);
		return;
	}
	
    //make echo command
    strncat(cmd_echo, buf + strlen(filename) + 6, strlen(buf) - strlen(filename) - 6);
	strcat(cmd_echo, "\"> ~/home_server/");
	strcat(cmd_echo, filename);
	//printf("PUT:cmd_eho=\n%s\n", cmd_echo);

	if((fp = popen(cmd_echo, "r")) == NULL){
		perror("popen");
        send(fd, "err popen\n", 10, 0);
		return;
	}
	while(fgets(tmp, BUFSIZE, fp) != NULL){
		strcat(buf_send, tmp);
	}

    pclose(fp);
    strcat(buf_send, cmd_success);
	//send
    send(fd, buf_send, BUFSIZE, 0);
    return;
}

void DEL(char *filename, int fd){
	char buf_send[BUFSIZE];
    char tmp[BUFSIZE];
    char cmd_rm[COMSIZE] = "rm -v ~/home_server/";
    FILE *fp;

	memset(buf_send, '\0', BUFSIZE);
	memset(tmp, '\0', BUFSIZE);
    //file delete
    strcat(cmd_rm, filename);
	if((fp = popen(cmd_rm, "r")) == NULL){
		pclose(fp);
		perror("popen");
        send(fd, "err popen\n", 10, 0);
		return;
	}
	while(fgets(tmp, BUFSIZE, fp) != NULL){
		strcat(buf_send, tmp);
	}

	//not exist file
	if(strlen(tmp) <= 1){
		pclose(fp);
		send(fd, "NOT FOUND\n", 10, 0);
		return;
	}	
    pclose(fp);
            
	//send
    send(fd, buf_send, BUFSIZE, 0);
    return;
}

void LS(char *pathname, int fd){
	char buf_send[BUFSIZE];
    char tmp[BUFSIZE];
    char cmd_ls[COMSIZE] = "ls ~/home_server/";
    FILE *fp;

	memset(buf_send, '\0', BUFSIZE);
	memset(tmp, '\0', BUFSIZE);
    //file delete
    strcat(cmd_ls, pathname);
	if((fp = popen(cmd_ls, "r")) == NULL){
		pclose(fp);
		perror("popen");
        send(fd, "err popen\n", 10, 0);
		return;
	}
	while(fgets(tmp, BUFSIZE, fp) != NULL){
		strcat(buf_send, tmp);
	}
	//printf("cmd_LS=%s\n", cmd_ls);
	//not exist file
	if(strlen(tmp) <= 1){
		pclose(fp);
		send(fd, "NOT FOUND\n", 10, 0);
		return;
	}	
    pclose(fp);
            
	//send
    send(fd, buf_send, BUFSIZE, 0);
    return;
}

void *recv_and_resp(int *fd_socket)
{
	int  fd, ret, cmd_number;
	char buf_rcv[BUFSIZE];
    char filename[NAMESIZE];
    char *err_msg = "invalid command\n";

	//fd取得後ロック解除
	fd = (int)*fd_socket;
	pthread_mutex_unlock(&lock);

	while(1){
        memset(buf_rcv, '\0', BUFSIZE);
        memset(filename, '\0', NAMESIZE);
	    if ((ret = recv(fd, buf_rcv, BUFSIZE, 0)) >= 0) {
			//end
            if(strcmp(buf_rcv, "exit") == 0){
			    printf("end server %d\n", fd);
			    break;
		    }
			
        	//reveive
			//printf("fd%d buf_rcv=%s\n",fd, buf_rcv);
			//コマンド識別
            if((cmd_number = get_cmd(buf_rcv, filename)) < 0){
                //error→error msg
                printf("invalid command\n");
                send(fd, err_msg, strlen(err_msg), 0);
            }
            
            switch(cmd_number){
                case 0:
                    GET(filename, fd);
                    break;
                case 1:
					PUT(buf_rcv, filename, fd);
                    break;
                case 2:
					DEL(filename, fd);
                    break;
                case 3:
					LS(filename, fd);
                    break;
                //default:
					//break;
            }
        }
		else{
			perror("recv");
			continue;
		}
	}
    close(fd);
}

int main(void)
{
	struct sockaddr_in saddr, caddr;
	int                fd1, fd2, ret, len;
	pthread_t          pt;	

	/* make socket */
	if ((fd1 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("socket");
		return -1;
	}

	memset(&saddr, 0, sizeof(saddr));
	saddr.sin_family      = AF_INET;
	saddr.sin_port        = htons(PORTNUM);
	saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	if(bind(fd1, (struct sockaddr*)&saddr, sizeof(saddr))) {
		perror("bind");
		return -1;
	}

	if(listen(fd1, 16)) {
		perror("listen");
		return -1;
	}


	len = sizeof(caddr);
	for (;;) {
		int pc;
		//スレッドで生成される関数内で、fdが値として保存されるまでロック
		//アンロックは下の２タイミングで実行
		//1:スレッドが生成されなかった場合（328行目)
		//2:スレッド関数内で"fd = (int)*fd_socket;"により値が保存された後
		pthread_mutex_lock(&lock);

		if((fd2 = accept(fd1, (struct sockaddr*) &caddr, &len)) < 0) {
			perror("accept");
			exit(1);
		}
		if ((pc = pthread_create(&pt, NULL, (void*)(recv_and_resp), (void*)&fd2)) < 0) {
			perror("pthread_create");
			return -1;
		}
		//スレッドが生成されなかった場合unlock
		if(fd2 < 0 || pc < 0 )
			pthread_mutex_unlock(&lock);

		pthread_detach(pt);
	}
	
	return 0;
}
