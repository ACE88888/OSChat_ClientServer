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
int is_nickname_command(char *message) { return strncmp(message, "\\NICKNAME", 9) == 0; }

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

int send_echo_message(int connfd, char *message) {
  upper_case(message);
  add_message(message);
  return send_message(connfd, message);
}

void handleJoinRoom(char* nick_name, char* room_name) {

}

void roomlist() {
  int i;
  for(i =0; i < 20; i++) {
    if (strcmp(room_buf[i].name, "") == 0) {
      printf("%s\n", room_buf[i].name);
    }
  }
}

void endsession() {
}

void userlist(int roomId) {
  int i;
  for(i =0; i < 20; i++) {
    if (strcmp(room_buf[roomId].sessions[i].nickname, "") == 0) {
      printf("%s\n", room_buf[roomId].sessions[i].nickname);
    }
  }
}

void handleCommandList() {
  printf("The available commands are:\n");
  printf("\\JOIN nickname room (join a specified room with the provided nickname)\n");
  printf("\\ROOMS (list of all the available rooms)\n");
  printf("\\LEAVE (leave the current room you are in)\n");
  printf("\\WHO (list of all users in the current room)\n");
  printf("\\HELP (list of commands)\n");
  printf("\\nickname message (send a message to a user with the provided nickname)\n");
}

void handleusermessage(char* nick_name, char* message) {
}

int process_message(int connfd, char *message) {
  if (is_list_message(message)) {
    printf("Server responding with list response.\n");
    return send_list_message(connfd);
  //Checking if the message starts with a command back-slash
  } else if (is_command(message)) {
    //Parse the command arguments, if any.
    int i = 0;
    char *args[3];
    char *ptr = strtok(message, " ");
    while (ptr != NULL) {
      printf("%s\n", ptr);
      args[i++] = ptr;
      ptr = strtok(NULL, " ");
    }
    if (is_join_command(message)) {
      handleJoinRoom(args[1], args[2]);
      printf("Server received the join command.\n");
    } else if (is_rooms_command(message)) {
      roomlist();
      printf("Server received the rooms command.\n");
    } else if (is_leave_command(message)) {
      endsession();
      printf("Server received the leave command.\n");
    } else if (is_who_command(message)) {
      int roomId = 1;
      userlist(roomId);
      printf("Server received the who command.\n");
    } else if (is_help_command(message)) {
      handleCommandList();
      printf("Server received the help command.\n");
    } else if (is_nickname_command(message)) {
      handleusermessage(args[0], args[1]);
      printf("Server received the nickname command.\n");
    } else {
      printf("The following command %s was not recognized.\n", message);
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
