# first of all import the socket library 
import socket  
from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
from Crypto.Hash import SHA256
  
s = socket.socket()         
print ("Socket successfully created")
  
port = 4321              
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)  
s.bind(('', port))         
print ("socket binded to %s" %(port)) 
  
s.listen(5)     
print ("socket is listening")            
  
c, addr = s.accept()     
print ('Got connection from', addr )

data = c.recv(2560)
print(data)
module = int.from_bytes(data, byteorder='little')
# for i in data:
#    print(i, end='')
# print()
# print(module)
#print(int.from_bytes(data[1:], byteorder='big'))

key = RSA.construct((module, 65537))

cipher_rsa = PKCS1_OAEP.new(key, SHA256)
encrypted_data = cipher_rsa.encrypt(b'Hello World!')


# Close the connection with the client 
c.close() 





s = socket.socket()
  
# Define the port on which you want to connect 
port = 1234
  
# connect to the server on local computer 
s.connect(('127.0.0.1', port)) 
  
# receive data from the server 

print(s.send(encrypted_data))
for i in encrypted_data:
   print(i, end='')
print()
# close the connection 
s.close()  
