import time

from workers import Sender, Receiver


def main():
	public_keys = dict()
	number_messages = 100000
	received_messages = []

	receiver_thread = Receiver(port=5555, public_keys=public_keys, number_messages=number_messages,
	                           received_messages=received_messages)
	receiver_thread.start()

	sender_thread = Sender(min_number_mixes=10, rate=0.01, public_keys=public_keys, number_messages=number_messages)
	sender_thread.start()

	while True:
		if len(received_messages) == number_messages:
			break
		print(len(received_messages))
		time.sleep(3)

	sender_thread.join()
	receiver_thread.join()

	with open('results.txt', 'w') as file:
		file.write('\t'.join(received_messages))
	print("All done")


if __name__ == '__main__':
	main()
