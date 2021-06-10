# first of all import the socket library 
import socket  
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
from Crypto.Hash import SHA256
  
s = socket.socket()
print ("Socket successfully created")
  
port = 5555              
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  
s.bind(('', port))         
print ("socket binded to %s" %(port)) 

s.listen(5)     
print ("socket is listening")  

public_keys = {}
  

for i in range(2):
   c, addr = s.accept()     
   print ('Got connection from', addr )

   data = c.recv(261)
   message_type = data[5]
   message_from_port = data[:4]
   
   print(data[:5])
   module = int.from_bytes(data[5:], byteorder='little')

   key = RSA.construct((module, 65537))

   cipher_rsa = PKCS1_OAEP.new(key, SHA256)
   public_keys[int(message_from_port)] = cipher_rsa

   # Close the connection with the client 
   c.close() 



for port in public_keys:

   s = socket.socket()
   
   # connect to the server on local computer 
   s.connect(('127.0.0.1', port)) 
   
   # receive data from the server 
   encrypted_data = public_keys[port].encrypt(b'Hello World!')

   print(s.send(b'1' + encrypted_data))
   for i in encrypted_data:
      print(i, end='')
   print()
   # close the connection 
   s.close()  
