import random
import time
import socket
import threading

from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
from Crypto.Hash import SHA256


MESSAGE_RECV_SIZE = 261


class Receiver(threading.Thread):
	def __init__(self, port: int, number_mixes: int, public_keys: dict):
		threading.Thread.__init__(self)
		self.port = port
		self.number_mixes = number_mixes
		self.public_keys = public_keys

	def run(self):
		try:
			s = socket.socket()
			s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
			s.bind(('', self.port))
			s.listen(self.number_mixes)

			while True:
				c, addr = s.accept()
				data = c.recv(MESSAGE_RECV_SIZE)

				print("Received!")

				message_from_port = int(data[:4])
				message_type = chr(data[4])
				message = data[5:]

				if message_type == '0':
					self.save_new_public_key(message_from_port, message)
		except Exception as e:
			print(f"Error running the Receiver worker: <{e}>")
			return

	def save_new_public_key(self, port_from: int, module_bytes: bytes):
		module = int.from_bytes(module_bytes, byteorder='little')
		key = RSA.construct((module, 65537))

		cipher_rsa = PKCS1_OAEP.new(key, SHA256)
		self.public_keys[port_from] = cipher_rsa


class Sender(threading.Thread):
	def __init__(self, min_number_mixes: int, public_keys: dict):
		threading.Thread.__init__(self)
		self.min_number_mixes = min_number_mixes
		self.public_keys = public_keys

	def run(self):
		num = 0
		while True:
			if len(self.public_keys) < self.min_number_mixes:
				print("Waiting...")
				time.sleep(0.1)
				continue

			msg = f"TEST_{num}".encode()

			port_send = random.choice(list(self.public_keys.keys()))
			msg_send = self.cipher_message(msg, port_send)

			self.send(msg_send, port_send)

			num += 1
			time.sleep(1)

	def cipher_message(self, msg: bytes, receiver_port: int) -> bytes:
		return b'1' + self.public_keys[receiver_port].encrypt(msg)

	@staticmethod
	def send(msg: bytes, port: int):
		try:
			s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			s.connect(("localhost", port))
			s.send(msg)
		except Exception as e:
			print(f"Error running the Sender worker: <{e}>")
			return
