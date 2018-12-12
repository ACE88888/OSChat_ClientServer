from socket import *
from threading import Thread
import sys

SERVER_NAME = 'localhost'
SERVER_PORT = 3000


def connect():
    # Create a socket of Address family AF_INET.
    sock = socket(AF_INET, SOCK_STREAM)
    # Client socket connection to the server
    sock.connect((SERVER_NAME, SERVER_PORT))

    return sock


def send(sock, message):
    sock.send(bytearray(message, 'utf-8'))


def recv(sock):
    return sock.recv(1024).decode('utf-8')


def list(sock):
    send(sock, '-')
    result = recv(sock).rstrip()
    if result == '':
        return []
    else:
        return result.split(',')[0:-1]


def last_list(sock):
    xs = list(sock)
    if len(xs) == 0:
        return ''
    else:
        return xs[-1]


def ask(prompt=':-p'):
    return input(prompt)


def prompt_on_last(sock):
    last = last_list(sock)
    if last == '':
        return ask()
    else:
        return ask(last)

def fileclient(f):
    fd = open(f, "r")
    connection = connect()
    thread = Thread(target = message_listener, args = (connection,))
    thread.start()
    thread.join()
    thread = Thread(target = send_command_file, args = (connection, fd,))
    thread.start()
    thread.join()

def client():
    connection = connect()
    thread1 = Thread(target = message_listener, args = (connection, ))
    thread1.start()
    send_command(connection)
    thread1.join()

def message_listener(connection):
    while 1:
        response = recv(connection)
        if (response == ""):
            break

def send_command_file(connection, fd):
    sentence = fd.readline()
	#opens passed in file, reads and sends messages as if the user was inputting commands.
    while sentence != '':
        print(sentence + "\n")
        send(connection, sentence)
        response = recv(connection)
        print(response.strip())
        sentence = fd.readline()

def send_command(connection):
    sentence = input("\n>")
    print(sentence + "\n")
    while sentence != 'quit':
        send(connection, sentence)
        sentence = input("\n>")

if __name__=="__main__":
	if len(sys.argv) == 1 :#checks for case where a file is passed in
		client()
	elif len(sys.argv) > 1:#if there is a file provided, let's load commands.
		fileclient(sys.argv[1])
