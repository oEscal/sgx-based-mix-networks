import argparse
from collections import Counter

import matplotlib.pyplot as plt

from utils import get_data_from_file


NUMBER_MESSAGES = 100000


def histogram_dispersion(data, number_mixes):
	fig = plt.figure()
	fig.set_figheight(6)
	fig.set_figwidth(14)

	plt.hist(data, bins=200, density=True)

	plt.title(f"Messages dispersion for {number_mixes} mixes", fontsize=20)
	plt.xlabel("Dispersion value", fontsize=15)
	plt.ylabel("Ratio (%)", fontsize=15)
	plt.yticks(fontsize=10)
	plt.xticks(fontsize=10)

	plt.savefig(f"figures/dispersion_{number_mixes}.png")


def pie_hops(data, number_mixes):
	fig = plt.figure()
	fig.set_figheight(15)
	fig.set_figwidth(12)

	counter = dict(Counter(data))
	counter = dict(sorted(counter.items(), key=lambda item: item[1], reverse=True))

	for i in counter:
		print(f"{i} - {counter[i]}")

	labels_counters = [i for i in list(counter.keys())[:10]]
	labels_percentages = [(counter[i]*100/NUMBER_MESSAGES) for i in labels_counters]
	labels = [f"{l}: {s:1.1f}%" for l, s in zip(labels_counters, labels_percentages)]

	plt.pie([counter[i] for i in counter], autopct='%1.1f%%', radius=1.5, textprops={'fontsize': 15})
	plt.legend(title="Number of hops", labels=labels, bbox_to_anchor=(1.01, -0.15),
	           ncol=5, scatterpoints=10, title_fontsize=20, fontsize=15)

	plt.title(f"Number of hops visited by the messages for {number_mixes} mixes", fontsize=30, pad=120)

	plt.savefig(f"figures/hops_{number_mixes}.png")


def scatter_dispersion_hops(dispersion, hops, number_mixes):
	fig = plt.figure()
	fig.set_figheight(6)
	fig.set_figwidth(14)

	plt.scatter(dispersion, hops)

	plt.title(f"Messages dispersion by the number of hops they visited for {number_mixes} mixes", fontsize=20)
	plt.ylabel("Number of hops", fontsize=15)
	plt.xlabel("Dispersion", fontsize=15)
	plt.yticks(fontsize=10)
	plt.xticks(fontsize=10)

	plt.savefig(f"figures/dispersion_hops_{number_mixes}.png")


def main():
	number_mixes = 300
	data = get_data_from_file(f"data/results_{number_mixes}_100000.txt")

	messages_counters = []
	number_hops = []
	for message in data:
		index_under_line = message.find('_')
		index_two_dots = message.find(':')

		messages_counters.append(int(message[index_under_line + 1:index_two_dots]))
		number_hops.append(int(message[index_two_dots + 1:]))

	dispersion = [j - messages_counters[j] for j in range(len(messages_counters))]
	histogram_dispersion(dispersion, number_mixes)

	print(len(set(number_hops)))
	pie_hops(number_hops, number_mixes)

	scatter_dispersion_hops(dispersion, number_hops, number_mixes)


if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('--instruction', default="cpuid", type=str, help=f"Instruction: one of [cpuid, rtc, lgdt]")
	parser.add_argument('--mode', default="kernel", type=str, help="Mode: one of [kernel, user]")
	args = parser.parse_args()

	main()
