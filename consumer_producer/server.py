import threading

from workers import Sender, Receiver


def main():
	public_keys = dict()

	receiver_thread = Receiver(port=5555, number_mixes=2, public_keys=public_keys)
	receiver_thread.start()

	sender_thread = Sender(min_number_mixes=2, public_keys=public_keys)
	sender_thread.start()

	sender_thread.join()
	receiver_thread.join()


if __name__ == '__main__':
	main()
