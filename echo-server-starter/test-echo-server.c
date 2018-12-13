/*
 * echoserverts.c - A concurrent echo server using threads
 * and a message buffer.
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/* Simplifies calls to bind(), connect(), and accept() */
typedef struct sockaddr SA;

/* Max text line length */
#define MAXLINE 8192

/* Second argument to listen() */
#define LISTENQ 1024

// We will use this as a simple circular buffer of incoming messages.
char message_buf[20][50];

// This is an index into the message buffer.
int msgi = 0;

// A lock for the message buffer.
pthread_mutex_t lock;
pthread_mutex_t roomLock;
// A structure to represent a user session.
struct session {
  char nickname[20];
  int port;
};

// A structure to represent chat rooms.
struct room {
  char name[20];
  struct session sessions[50];
};

// Room buffer.
struct room room_buf[20];

// Initialize the message buffer to empty strings.
void init_message_buf() {
  int i;
  for (i = 0; i < 20; i++) {
    strcpy(message_buf[i], "");
  }
}
struct session sessions[1000];
void init_sessions(){
	for(int i = 0; i<1000;i++){
		strcpy(sessions[i].nickname, "");
		sessions[i].port = -1;
}
}
// Initialize the room buffer to empty strings.
void init_room_buf() {
  int i;
  int j;
  for (i = 0; i < 20; i++) {
    strcpy(room_buf[i].name, "");
    for (j = 0; j < 50; j++) {
      strcpy(room_buf[i].sessions[j].nickname, "");
      room_buf[i].sessions[j].port = -1;
    }
  }
}

// This function adds a message that was received to the message buffer.
// Notice the lock around the message buffer.
void add_message(char *buf) {
  pthread_mutex_lock(&lock);
  strncpy(message_buf[msgi % 20], buf, 50);
  int len = strlen(message_buf[msgi % 20]);
  message_buf[msgi % 20][len] = '\0';
  msgi++;
  pthread_mutex_unlock(&lock);
}

// Destructively modify string to be upper case
void upper_case(char *s) {
  while (*s) {
    *s = toupper(*s);
    s++;
  }
}

// A wrapper around recv to simplify calls.
int receive_message(int connfd, char *message) {
  return recv(connfd, message, MAXLINE, 0);
}

// A wrapper around send to simplify calls.
int send_message(int connfd, char *message) {
  return send(connfd, message, strlen(message), 0);
}

// A predicate function to test incoming message.
int is_list_message(char *message) { return strncmp(message, "-", 1) == 0; }

// Checks if the message is starts with the command string.
int is_command(char *message) { return strncmp(message, "\\", 1) == 0; }

// Checks if the message is a join command.
int is_join_command(char *message) { return strncmp(message, "\\JOIN", 5) == 0; }

// Checks if the message is a rooms command.
int is_rooms_command(char *message) { return strncmp(message, "\\ROOMS", 6) == 0; }

// Checks if the message is a leave command.
int is_leave_command(char *message) { return strncmp(message, "\\LEAVE", 6) == 0; }

// Checks if the message is a who command.
int is_who_command(char *message) { return strncmp(message, "\\WHO", 4) == 0; }

// Checks if the message is a help command.
int is_help_command(char *message) { return strncmp(message, "\\HELP", 5) == 0; }

// Checks if the message is a nickname command.
int is_message_command(char *message) { return strncmp(message, "\\MESSAGE", 8) == 0; }

int send_list_message(int connfd) {
  char message[20 * 50] = "";
  for (int i = 0; i < 20; i++) {
    if (strcmp(message_buf[i], "") == 0) break;
    strcat(message, message_buf[i]);
    strcat(message, ",");
  }

  // End the message with a newline and empty. This will ensure that the
  // bytes are sent out on the wire. Otherwise it will wait for further
  // output bytes.
  strcat(message, "\n\0");
  printf("Sending: %s", message);

  return send_message(connfd, message);
}
void echo_to_rooms(int connfd, char *message){
        for (int i = 0; i < 20; i++) {
    //If the room name provided matches a room name, then loop through the room sessions.
    for (int j=0;j<50; j++){
        if (room_buf[i].sessions[j].port == connfd) {//if sender is in room i
                for(int k = 0; k<50;k++){//sends message to all other users in the room.
                        if (room_buf[i].sessions[k].port != 0) {
			char newmsg[50]; 
			strcpy(newmsg, room_buf[i].sessions[j].nickname);
			strcat(newmsg,": ");
                        strcat(newmsg, message);
			send_message(room_buf[i].sessions[k].port,newmsg);
                }
                }
        }
}
}
}

int send_echo_message(int connfd, char *message) {
  upper_case(message);
  add_message(message);
pthread_mutex_lock(&roomLock); 
 echo_to_rooms(connfd, message);
pthread_mutex_unlock(&roomLock);
  return 1;
}

//This method allows a user to join a room with a nickname of their choice.
int handleJoinRoom(int connfd, char* nick_name, char* room_name) {

  int clientPort = connfd;
  int i, j, flag = 0;
  //Loop through the available rooms.
    pthread_mutex_lock(&roomLock);
  for (i = 0; i < 20; i++) {
    //If the room name provided matches a room name, then loop through the room sessions.
    if (strcmp(room_buf[i].name, room_name) == 0) {
      for (j = 0; j < 50; j++) {
        //If there is an empty session (no nickname), then give that session a nickname and port.
        if (strcmp(room_buf[i].sessions[j].nickname, "") == 0) {

         strcpy(room_buf[i].sessions[j].nickname, nick_name);
          room_buf[i].sessions[j].port = clientPort;
          flag = 1;
           for(j = 0; j < 50; j++){// loops over each session
        	 	if(strcmp(room_buf[i].sessions[j].nickname,"")!=0){//checks for another member in the room
				char msg[50];
				strcpy(msg, nick_name);
				strcat(msg, " has joined the room.\n");
        			send_message(room_buf[i].sessions[j].port,msg);//sends the other member a message`
        	 	}
      	   }
      	  break;
        }
      }
    }
  }
  //If provided room does not exist, then loop through the rooms.
  if (flag == 0) {
  	for (i = 0; i < 20; i++) {
      //If there is an empty room (no name), then create a room and provide attributes to the new user session.
      if (strcmp(room_buf[i].name, "") == 0) {
        strcpy(room_buf[i].name, room_name);
        strcpy(room_buf[i].sessions[0].nickname, nick_name);
        room_buf[i].sessions[j].port = clientPort;
        flag = 1;
        char msg[50] = "You have successfully created room : ";
	strcat(msg,room_name);
	send_message(connfd, msg);
	break;
      }
    }
  }
  pthread_mutex_unlock(&roomLock);
  return 1;
}

//This method will send a user the list of available rooms.
int handleRoomList(int connfd) {
  int i;
  char* roomList;
  //Loop through the list of rooms.
  pthread_mutex_lock(&roomLock);
  for(i = 0; i < 20; i++) {
    //If the room is not blank (an existing room), then print the room name.
    if (strcmp(room_buf[i].name, "") != 0) {
      send_message(connfd, room_buf[i].name);
      send_message(connfd, "\n");
    }
  }
  pthread_mutex_unlock(&roomLock);
  return 1;
}

//This method will remove a user from a chat room and send a GOODBYE message.
int handleExitSession(int connfd) {
	pthread_mutex_lock(&roomLock);
  //Loop through all the rooms and sessions.
	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 50; j++) {
      //If the a session matches the current port, then reset the session references.
			if (room_buf[i].sessions[j].port == connfd) {
  			room_buf[i].sessions[j].port = -1;
  			strcpy(room_buf[i].sessions[j].nickname, "");
  		}
  	}
  }
  pthread_mutex_unlock(&roomLock);
  return send_message(connfd, (char*) "GOODBYE\n");
}

//This method will provide a list of all the users in the current room.
int handleUserList(int connfd) {
  //Loop through the user sessions in the room.
  pthread_mutex_lock(&roomLock);

int j;
for(int i = 0; i <20 ;i++){
  for (j = 0; j < 50; j++) {
    //If the user nickname is not blank (an existing user), then print the user.
    if (room_buf[i].sessions[j].port  == connfd) {//find the room
	for (j=0;j<50;j++){//iterate over users
      		if(room_buf[i].sessions[j].port > 0){//if user exists
			send_message(connfd, room_buf[i].sessions[j].nickname);
			send_message(connfd, "\n");
		}
	}
    }
  }
}
  pthread_mutex_unlock(&roomLock);
  return 1;
}

//This method will send the user a current list of all the commands.
void handleCommandList(int connfd) {
  send_message(connfd, (char*) "The available commands are:\n\\JOIN nickname room (join a specified room with the provided nickname)\n\\ROOMS (list of all the available rooms)\n\\LEAVE (leave the current room you are in)\n\\WHO (list of all users in the current room)\n\\HELP (list of commands)\n\\nickname message (send a message to a user in the room)\n\\MESSAGE nickname message (sends a message to a user in any room with same nickname)");
}

int handleGroupUserMessage(char* nick_name, char* message, int connfd) {
  //Loop through the rooms and user sessions.
  pthread_mutex_lock(&roomLock);
  char msg[50];
  for (int i = 0; i < 20; i++){
      for (int j = 0; j < 50;j++){
      //If the user with the provided nickname exists in a room, then send a message to that user.
         if (room_buf[i].sessions[j].port == connfd) {
			strcpy(msg, room_buf[i].sessions[j].nickname);
            strcat(strcat(msg,": "),message);
            for (int k = 0; k < 50;k++){
            //If the user with the provided nickname exists in a room, then send a message to that user.
               if (strcmp(room_buf[i].sessions[k].nickname, nick_name) == 0) {
                  send_message(room_buf[i].sessions[k].port, msg);
               }
             }
          }           
	   }	
  }
  pthread_mutex_unlock(&roomLock);
  return send_message(connfd, msg);
}

//This method will send a message to a specific user with the given nickname.
int handleAnyUserMessage(char* nick_name, char* message, int connfd) {
  //Loop through the rooms and user sessions.
  pthread_mutex_lock(&roomLock);
  char msg[50];
  for (int i = 0; i < 20; i++){
  	for (int j = 0; j < 50;j++){
        if (room_buf[i].sessions[j].port == connfd) {
            strcpy(msg, room_buf[i].sessions[j].nickname);
            strcat(strcat(msg,": "),message);
        }
      }
      }
      for (int i = 0; i < 20; i++){
         for (int j = 0; j < 50;j++){
      //If the user with the provided nickname exists in a room, then send a message to that user.
  		     if (strcmp(room_buf[i].sessions[j].nickname, nick_name) == 0) {
			   send_message(room_buf[i].sessions[j].port, msg);
      }
    }
  }
  pthread_mutex_unlock(&roomLock);
  return send_message(connfd,msg);
}

int process_message(int connfd, char *message) {
  upper_case(message);
  if (is_list_message(message)) {
    printf("Server responding with list response.\n");
    return send_list_message(connfd);
  //Checking if the message starts with a command back-slash
  } else if (is_command(message)) {
    //Parse the command arguments, if any.
    int i = 0;
    char *args[10];
    for(int i = 0;i < 10;i++) {
    args[i] = NULL;
    }
    char *ptr = strtok(message, " \\");
    while (ptr != NULL) {
      printf("%s\n", ptr);
      args[i++] = ptr;
      ptr = strtok(NULL, " \\");
      fflush(stdout);

    }
    if (is_join_command(message)) {
      printf("Server received the join command.\n");
	handleJoinRoom(connfd, args[1], args[2]);
      printf("Server received the join command.\n");
    } else if (is_rooms_command(message)) {
      handleRoomList(connfd);
      printf("Server received the rooms command.\n");
    } else if (is_leave_command(message)) {
      handleExitSession(connfd);
      printf("Server received the leave command.\n");
    } else if (is_who_command(message)) {
      handleUserList(connfd);
      printf("Server received the who command.\n");
    } else if (is_help_command(message)) {
      handleCommandList(connfd);
      printf("Server received the help command.\n");
    } else if (args[1] != NULL && args[2] == NULL) {
      handleGroupUserMessage(args[0], args[1], connfd);
      printf("Server received the message any user command.\n");
    }else if (is_message_command(message)) {
      handleAnyUserMessage(args[1], args[2], connfd);
      printf("Server received the message group user command.\n");
    } else {
      send_message(connfd, (char*) "The following command was not recognized.\n");
    }
  } else {
    printf("Server responding with echo response.\n");
    return send_echo_message(connfd, message);
  }
}

// The main function that each thread will execute.
void echo(int connfd) {
  
size_t n;

  // Holds the received message.
  char message[MAXLINE];

  while ((n = receive_message(connfd, message)) > 0) {
    message[n] = '\0';  // null terminate message (for string operations)
    printf("Server received message %s (%d bytes)\n", message, (int)n);
    n = process_message(connfd, message);
  }
}

// Helper function to establish an open listening socket on given port.
int open_listenfd(int port) {
  int listenfd;    // the listening file descriptor.
  int optval = 1;  //
  struct sockaddr_in serveraddr;

  /* Create a socket descriptor */
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;

  /* Eliminates "Address already in use" error from bind */
  if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
                 sizeof(int)) < 0)
    return -1;

  /* Listenfd will be an endpoint for all requests to port
     on any IP address for this host */
  bzero((char *)&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr)) < 0) return -1;

  /* Make it a listening socket ready to accept connection requests */
  if (listen(listenfd, LISTENQ) < 0) return -1;
  return listenfd;
}

// thread function prototype as we have a forward reference in main.
void *thread(void *vargp);

int main(int argc, char **argv) {
  // Check the program arguments and print usage if necessary.
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(0);
  }

  // initialize message buffer.
  init_message_buf();

  // initialize room buffer.
  init_room_buf();

  // Initialize the message buffer lock.
  pthread_mutex_init(&lock, NULL);

  // Initialize the room lock.
  pthread_mutex_init(&roomLock, NULL);
  // The port number for this server.
  int port = atoi(argv[1]);

  // The listening file descriptor.
  int listenfd = open_listenfd(port);

  // The main server loop - runs forever...
  while (1) {
    // The connection file descriptor.
    int *connfdp = (int*) malloc(sizeof(int));

    // The client's IP address information.
    struct sockaddr_in clientaddr;

    // Wait for incoming connections.
    socklen_t clientlen = sizeof(struct sockaddr_in);
    *connfdp = accept(listenfd, (SA *)&clientaddr, &clientlen);

    /* determine the domain name and IP address of the client */
    struct hostent *hp =
        gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
                      sizeof(clientaddr.sin_addr.s_addr), AF_INET);

    // The server IP address information.
    char *haddrp = inet_ntoa(clientaddr.sin_addr);

    // The client's port number.
    unsigned short client_port = ntohs(clientaddr.sin_port);

    printf("server connected to %s (%s), port %u\n", hp->h_name, haddrp,
           client_port);

    // Create a new thread to handle the connection.
    pthread_t tid;
    pthread_create(&tid, NULL, thread, connfdp);
  }
}

/* thread routine */
void *thread(void *vargp) {
  // Grab the connection file descriptor.
  int connfd = *((int *)vargp);
  // Detach the thread to self reap.
  pthread_detach(pthread_self());
  // Free the incoming argument - allocated in the main thread.
  free(vargp);
  // Handle the echo client requests.
  echo(connfd);
  printf("client disconnected.\n");
  // Don't forget to close the connection!
  close(connfd);
  return NULL;
}
