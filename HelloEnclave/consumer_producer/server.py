import sys
import time

from workers import Sender, Receiver


def main(number_mixes: int = 10, number_messages: int = 100000):
	public_keys = dict()
	received_messages = []

	receiver_thread = Receiver(port=5555, public_keys=public_keys, number_messages=number_messages,
	                           received_messages=received_messages)
	receiver_thread.start()

	sender_thread = Sender(min_number_mixes=number_mixes, rate=0, public_keys=public_keys, number_messages=number_messages)
	sender_thread.start()

	while True:
		if len(received_messages) == number_messages:
			break
		print(len(received_messages))
		time.sleep(3)

	sender_thread.join()
	receiver_thread.join()

	with open(f'results_{number_mixes}_{number_messages}.txt', 'w') as file:
		file.write('\t'.join(received_messages))
	print("All done")


if __name__ == '__main__':
	number_mixes = 10
	number_messages = 100000

	if len(sys.argv) > 1:
		number_mixes = int(sys.argv[1])
	
		if len(sys.argv) > 2:
			number_messages = int(sys.argv[2])

	main(number_mixes=number_mixes, number_messages=number_messages)
