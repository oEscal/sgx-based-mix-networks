
def get_data_from_file(file_name: str) -> list:
	with open(file_name, 'r') as file:
		data = file.read().rstrip().split('\t')
	return data
