/*
 *	  CSC 501 - HW5 sample code
 */

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include "openssl/md5.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <math.h>

#define BUFLEN 512

int curr_host;
int TOTAL_NODES;
int portnum = 50000;

typedef struct node {
	char * node;
	struct node *next;
	struct node *prev;
} NODE;

NODE *head = NULL;
NODE *finger_table_head = NULL;

/*
 *Sid: Count Number of elements in the nodelist
 */
int count_nodes(NODE *head){
    int i = 0;
    NODE *ptr = head;

    while (ptr != NULL) {
        i++;
        ptr = ptr->next;
    }

    return i;

}

/*
 *Sid:Count number of lines in file
 */
int countlines(FILE *fp) {
	char ch;
	int number=0;

	while ((ch = fgetc(fp) )!= EOF ) {
		if (ch == '\n') {
			number++;	
		}
	} 	
	return number;
} 

void display_list(NODE *head) 
{
	int i = 0;
	printf("---------------------------------------------\n");
        NODE *ptr = head;
	while(ptr != NULL) {
		printf(">>%s\n",ptr->node);
		ptr = ptr->next;
	}
	printf("---------------------------------------------\n");
}

int compareAddressPort(char *ele1, char*ele2)
{

	char a[40], b[40], *ptr1, *ptr2;	
	char *token1, *token2;	
	int num1,num2;

	strcpy(a,ele1);
	strcpy(b,ele2);

	token1=strtok_r(a, ".",&ptr1);
	token2=strtok_r(b, ".",&ptr2);

	num1=atoi(token1);
	num2=atoi(token2);
		
	if (num1 > num2) {
		return 1;
	} else if (num1 < num2) {
		return -1;
	}	


	while ((token1 = strtok_r(NULL,".",&ptr1))!=NULL) {
		num1=atoi(token1);
		token2=strtok_r(NULL,".",&ptr2);
		num2=atoi(token2);
		if (num1 == num2)
			continue;
		return (num1>num2) ? 1 : -1;				
	}

	return (num1>num2) ? 1 : -1;	
			
	/*	printf("strlen a %d strlen b %d\n", strlen(a), strlen(b));
	if(strlen(a)>strlen(b))
		return 1;
	else if(strlen(a)<strlen(b))
		return -1;
	else if(strcmp(a,b) > 0)
		return 1;
	else
		return -1;
	*/
}

NODE* insert(NODE *element, NODE *head) 
{
	element->next=element->prev=NULL;
	if (head == NULL) {
		printf("YES.. NULL\n");
		head=element;
		return head;
	}

	NODE *ptr=head;
	NODE *prevPtr=head;
	int i = 0;
	int ret = compareAddressPort(element->node,ptr->node);
	printf("Return value is %d\n", ret);
	while (ptr!=NULL && ret>0)
	{
		ptr=ptr->next;
		if (ptr != NULL) 
			ret = compareAddressPort(element->node,ptr->node);
		printf("Return value is %d\n", ret);
		if(i++ != 0)
			prevPtr = prevPtr->next;
	}
	
	if (ptr!=NULL)
	{
		printf("Entered at the right point\n");
		if (ptr == head) 
		{
			head= element;
			element->next = ptr;
			ptr->prev = element;
		} else {
			element->next=ptr;
			element->prev=ptr->prev;
			if (ptr->prev != NULL) {
				ptr->prev->next=element;
			}
			ptr->prev=element;
		}
	} else {
		prevPtr->next = element;
		element->prev = prevPtr;
	}
        return head;
}

//void getendpoints(FILE *fp, NODE * head)
void getendpoints(FILE *fp)
{
        int fsize=0;
	char *ptr3;
	char *endpoints;
	fseek(fp, 0, SEEK_END);
	fsize=ftell(fp);
	rewind(fp);
	endpoints=(char *)malloc(sizeof(char)* (fsize+1));
	fread(endpoints, sizeof(char), fsize, fp);
	endpoints[fsize] = 0;

	char *node = strtok_r(endpoints,"\n",&ptr3);
  	while (node != NULL)
  	{
		NODE *element = (NODE*)malloc(sizeof(NODE));
		element->node = node;
    		printf ("Element ---> \"%s\"\n",node);
		head = insert(element, head);
	        display_list(head);	
    		node = strtok_r(NULL, "\n",&ptr3);
  	}	
}

void printhash(unsigned char h[16])
{	 int i;
	 for(i=0;i<16;i++)
		 printf("%02x",h[i]);
}

/*
 *  calculates the hash and stores in h
 */
void calculatehash(char *c, int len, char *h)
{	
	MD5(c,len, h);
}


/*
 * forwards message m to port
 */
void forward_message(int port, char *m)
{
		struct sockaddr_in sock_client;
		struct hostent *hent;
		int sc, i, slen = sizeof(sock_client);

		if ((sc = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			printf("socket creation failed ");
			exit(1);
		}

		hent = gethostbyname("localhost");
		if(hent == NULL)
		{	printf("gethostbyname failed ");
			exit(1);
		}

		memset((char *) &sock_client, 0, sizeof(sock_client));

		sock_client.sin_family = AF_INET;
		sock_client.sin_port = htons(port);
		sock_client.sin_addr = *(struct in_addr*)(hent ->h_addr_list[0]);

		if (connect(sc, (struct sockaddr *) &sock_client, slen) == -1) {
			printf("connect failed");
			exit(1);
		}

		if (send(sc, m, BUFLEN, 0) == -1) {
			printf("send failed ");
			exit(1);
		}
		close(sc);
}

/*
 *Sid: Finger Table initialization
 */
void create_finger_table(char *my_addr) {
    
    //Sid: my_pos flag is set once my position is found in the endpoints list.
    int my_pos = 0;
    NODE *ptr = head;
    double N = count_nodes(ptr);
    //Sid: Log of N to the base 2
    N = floor(log2(N));
    double count = 0.0;
    char checkString[100];

    while (1) {
        memset(checkString, '\0', sizeof(checkString));
        strcpy(checkString,ptr->node);
        strcat(checkString,"\n");
        printf ("%s",checkString);
        if (strlen(checkString) == strlen(my_addr) && strcmp(checkString, my_addr)==0) {

            if (my_pos == 1) {
                //Sid: if my_pos is already set, we have looped around the linked list somehow, time to end the loop
                break;
            } else {
                //Sid: Set the my_pos flag and get the next log N elements in the list
                my_pos = 1;
            }

        }

        if (my_pos == 1) {
            //Sid: Get the next log N elements
            NODE *element = (NODE*)malloc(sizeof(NODE));
            element->node = ptr->node;
            finger_table_head = insert(element, finger_table_head);
            count++;
        }

        //Sid: Check if log N(or N) elements have been inserted into the finger table
        if ( count == N) {
            break;
        }

        if (ptr->next != NULL) {
            ptr = ptr->next;
        } else {
            //Sid: If reached the end of the list point to the head and begin traversing again
            ptr = head;
        }
    }
}

void server_listen(int max_node) {
	struct sockaddr_in sock_server, sock_client;
	int s, slen = sizeof(sock_client);
	char *command;
	char buf[BUFLEN];
	int client;
	int portnum;
	FILE *fp;

	time_t t = time(0); 
	srand(t);

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		printf("error in socket creation");
		exit(1);
	}

	memset((char *) &sock_server, 0, sizeof(sock_server));
	sock_server.sin_family = AF_INET;
   	portnum =(rand()%64000)+1025;
	sock_server.sin_port = htons(portnum);
	sock_server.sin_addr.s_addr = htonl(INADDR_ANY);
   
   	while (bind(s,(struct sockaddr *)&sock_server,sizeof(sock_server)) == EADDRINUSE) {
		portnum = (rand()%64000)+1025;
   	}

   	fp =fopen("endpoints","a"); 

   	if (fp == NULL ) {
		printf("File open failed\n");
		exit(0);
   	}

	char address[100];
   	sprintf(address,"%s.%d\n","127.0.0.1",portnum);
   	fputs(address,fp);
   	fclose(fp);
	fp =fopen("endpoints","r"); 

   	if (fp == NULL ) {
                printf("File open failed\n");
                exit(0);
   	}

	//NODE head=NULL;
   	int count = countlines(fp);
        //getendpoints(fp,&head);
        getendpoints(fp);
   	fclose(fp);

#ifdef DEBUG
printf("I am already here\n");
#endif

	if (max_node == count ) {
		//create_finger_table(address);
                //Sid: Display Finger Table
                printf("Finger Table: \n");
                //display_list(finger_table_head);
                printf("\n");
	}

//	if (bind(s, (struct sockaddr *) &sock_server, sizeof(sock_server)) == -1) {
//		printf("error in binding socket");
//		exit(1);
//	}

	if (listen(s, 10) == -1) {
		printf("listen error");
		exit(1);
	}

	char nodehash_string[20], hash[16];

	sprintf(nodehash_string,"localhost%d",portnum);
	calculatehash(nodehash_string, strlen(nodehash_string), hash);

	printf("DHT node (");
	printhash(hash);
	printf("): Listening on port number %d . . . \n", portnum);

	while (1) { /* quit only on END message */
		if ((client = accept(s, (struct sockaddr *) &sock_client, &slen)) == -1) {
			printf("accept error");
			exit(1);
		}

		if (recv(client, buf, BUFLEN, 0) == -1) {
			printf("recv error");
			exit(1);
		}

		command = strtok(buf, ":");
		if (strcmp(command, "END") == 0) {

				printf("END message received \n");

		}
		else if (strcmp(command, "PUT") == 0) {



		}
		close(client);
	}

	close(s);
}

int main(int argc, char *argv[]) {

	if (argc != 2) {
		printf("wrong number of arguments");
		return 1;
	}

	TOTAL_NODES = atoi(argv[1]);
	//curr_host = atoi(argv[2]);

	//initialize_host();
	server_listen(TOTAL_NODES);

	return 0;
}

