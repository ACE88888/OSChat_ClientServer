from socket import *
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
    sentence = fd.readline()

    while sentence != '':
        print(sentence)
        send(connection, sentence)
        response = recv(connection)
        print(response.strip())
        sentence = fd.readline()

def client():
    connection = connect()
    sentence = prompt_on_last(connection)

    while sentence != 'quit':
        send(connection, sentence)
        response = recv(connection)
        print(response.strip())
        sentence = prompt_on_last(connection)


if __name__=="__main__":
	if len(sys.argv) == 1 :
		client()
	elif len(sys.argv)>1:
		fileclient(sys.argv[1])
		
