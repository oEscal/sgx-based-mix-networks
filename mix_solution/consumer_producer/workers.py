import random
import time
import socket
import threading
from queue import Queue

from Crypto.PublicKey import RSA
from Crypto.Cipher import PKCS1_OAEP
from Crypto.Hash import SHA256


MESSAGE_RECV_SIZE = 261
FAN_OUT_WAIT_TIME = 2*60


class Receiver(threading.Thread):
	def __init__(self, port: int, public_keys: dict, number_messages: int, received_messages: list):
		threading.Thread.__init__(self)
		self.port = port
		self.public_keys = public_keys
		self.received_messages = received_messages
		self.number_messages = number_messages

	class __Receive_Message(threading.Thread):
		def __init__(self, public_keys, received_messages, message_queue: Queue):
			threading.Thread.__init__(self, daemon=True)
			self.public_keys = public_keys
			self.received_messages = received_messages
			self.message_queue = message_queue

		def run(self):
			while True:
				data = self.message_queue.get()
				message_from_port = int(data[:4])
				message_type = chr(data[4])
				message = data[5:]

				if message_type == '0':
					self.save_new_public_key(message_from_port, message)
				elif message_type == '2':
					self.received_messages.append(message.decode())

		def save_new_public_key(self, port_from: int, module_bytes: bytes):
			print(f"Saving new public key from port <{port_from}>")

			module = int.from_bytes(module_bytes, byteorder='little')
			key = RSA.construct((module, 65537))

			cipher_rsa = PKCS1_OAEP.new(key, SHA256)
			self.public_keys[port_from] = cipher_rsa

	def run(self):
		try:
			s = socket.socket()
			s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
			s.settimeout(10)
			s.bind(('127.0.0.1', self.port))
			s.listen(4096)

			message_queue = Queue()

			threads = list()
			for index in range(10):
				thread = self.__Receive_Message(self.public_keys, self.received_messages, message_queue=message_queue)
				thread.start()
				threads.append(thread)

			while True:
				try:
					c, addr = s.accept()
					message_queue.put(c.recv(MESSAGE_RECV_SIZE))
				except socket.timeout:
					print("oof")
					print(len(self.received_messages))
					if len(self.received_messages) == self.number_messages:
						print("Joining all the worker threads and finishing the main receiver worker")
						for thread in threads:
							thread.join(0)
						break

		except Exception as e:
			print(f"Error running the Receiver worker: <{e}>")
			return


class Sender(threading.Thread):
	def __init__(self, min_number_mixes: int, rate: float, public_keys: dict, number_messages: int):
		threading.Thread.__init__(self)
		self.min_number_mixes = min_number_mixes
		self.public_keys = public_keys
		self.number_messages = number_messages
		self.rate = rate

	def run(self):
		num = 0
		print("Waiting...")
		while True:
			if len(self.public_keys) < self.min_number_mixes:
				time.sleep(1)
				continue

			time.sleep(self.rate)

			msg = f"TEST_{num}:0".encode() + b'\x00'

			port_send = random.choice(list(self.public_keys.keys()))
			msg_send = self.cipher_message(msg, port_send)

			self.send(msg_send, port_send)

			num += 1

			if num > self.number_messages - 1:
				print(f"Stopped sending messages. Wating for <{FAN_OUT_WAIT_TIME}> seconds to send fan out order")
				time.sleep(FAN_OUT_WAIT_TIME)
				print("STOP")
				for port_send in self.public_keys:
					print(f"Sending Fan out to port {port_send}!\n\n\n")
					self.send(b'3\x00', port_send)
				break

	def cipher_message(self, msg: bytes, receiver_port: int) -> bytes:
		return b'1' + self.public_keys[receiver_port].encrypt(msg)

	@staticmethod
	def send(msg: bytes, port: int):
		try:
			s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
			s.connect(("localhost", port))
			s.send(msg)
			s.close()
		except Exception as e:
			print(f"Error running the Sender worker: <{e}>")
			return
