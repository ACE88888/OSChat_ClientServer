from socket import *
from threading import Thread
import threading
import sys

SERVER_NAME = "localhost"
SERVER_PORT = 3000


def connect():
    # Create a socket of Address family AF_INET.
    sock = socket(AF_INET, SOCK_STREAM)
    # Client socket connection to the server
    sock.connect((SERVER_NAME, SERVER_PORT))
    sock.setblocking(False)

    return sock


def send(sock, message):
    sock.send(bytearray(message, "utf-8"))


def recv(sock):
    try:
        msg = sock.recv(1024).decode("utf-8")
    except:
        msg = ""

    return msg


def list(sock):
    send(sock, "-")
    result = recv(sock).rstrip()
    if result == "":
        return []
    else:
        return result.split(",")[0:-1]


def last_list(sock):
    xs = list(sock)
    print(xs)
    if len(xs) == 0:
        return ""
    else:
        return xs[-1]


def ask(prompt=":-p"):
    return input(prompt)


def prompt_on_last(sock):
    last = last_list(sock)
    if last == "":
        return ask()
    else:
        return ask(last)


def fileclient(f):
    fd = open(f, "r")
    connection = connect()
    thread = Thread(target=message_listener, args=(connection,))
    thread.start()
    thread.join()
    thread = Thread(target=send_command_file, args=(connection, fd))
    thread.start()
    thread.join()


"""
# Old client method (BROKEN)
def client():
    connection = connect()
    thread = Thread(target=message_listener, args=(connection,))
    thread.start()
    thread.join()
    thread = Thread(target=send_command, args=(connection,))
    thread.start()
    thread.join()
"""


def client():
    lock = threading.Lock()
    # connect to the server, connection is a socket here
    connection = connect()
    # create the message_listener thread, and then start it up
    thread = Thread(target=message_listener, args=(connection, lock))
    thread.start()
    # send_command will loop to send and prompt for messages
    send_commands(connection, lock)
    # by this point, the client will have entered "quit", so at
    # this point we can join the message_listener thread and program will end:
    thread.join()


def message_listener(connection, lock):
    # lock = threading.Lock()
    while 1:
        # lock.acquire()
        response = recv(connection)
        # lock.release()
        if len(response) > 0:
            print(response.strip() + "\n")


def send_command_file(connection, fd):
    sentence = fd.readline()
    # opens passed in file, reads and sends messages as if the user was inputting commands.
    while sentence != "":
        print(sentence)
        send(connection, sentence)
        response = recv(connection)
        print(response.strip())
        sentence = fd.readline()


def send_commands(connection, lock):
    sentence = prompt_on_last(connection)
    while sentence != "quit":
        # lock.acquire()
        send(connection, sentence)
        sentence = prompt_on_last(connection)
        # lock.release()


if __name__ == "__main__":
    if len(sys.argv) == 1:  # checks for case where a file is passed in
        client()
    elif len(sys.argv) > 1:  # if there is a file provided, let's load commands.
        fileclient(sys.argv[1])
