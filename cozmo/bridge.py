import socket
import sys
from thread import *
 
HOST = ''   # Symbolic name meaning all available interfaces
PORT = 8889 # Arbitrary non-privileged port

robot_message = "none"
 
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
print 'Socket created'
 
#Bind socket to local host and port
try:
    s.bind((HOST, PORT))
except socket.error as msg:
    print 'Bind failed. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
    sys.exit()
     
print 'Socket bind complete'
 
#Start listening on socket
s.listen(10)
print 'Socket now listening'
 
#Function for handling connections. This will be used to create threads
def clientthread(conn):
    start_new_thread(udpClientThread ,(conn,))
    #Sending message to connected client
###    conn.send('Welcome to the server. Type something and hit enter\n') #send only takes string
     
    #infinite loop so that function do not terminate and thread do not end.
    while True:
         
        #Receiving from client
        data = conn.recv(1024)
        reply = 'ref ' + str(0.123) + " " + str(0.456)
        print reply
        if not data: 
            break
     
        conn.sendall(robot_message)
        #conn.sendall(reply)
     
    #came out of loop
    conn.close()
 
#now keep talking with the client

def udpClientThread(conn):
  global robot_message
  robot_message = "no"
  UDP_IP = "104.131.47.73"
  UDP_PORT = 2362

  sock = socket.socket(socket.AF_INET, # Internet
                     socket.SOCK_DGRAM) # UDP
  sock.bind((UDP_IP, UDP_PORT))
  print "in udp thread"
  while True:
      data, addr = sock.recvfrom(1024) # buffer size is 1024 bytes
      robot_message = data
      print "received message:", data

while 1:
    #wait to accept a connection - blocking call
    conn, addr = s.accept()
    print 'Connected with ' + addr[0] + ':' + str(addr[1])
     
    #start new thread takes 1st argument as a function name to be run, second is the tuple of arguments to the function.
    start_new_thread(clientthread ,(conn,))
 
s.close()
