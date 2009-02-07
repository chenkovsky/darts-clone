// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

template <typename Dictionary>
int darts(const string &index_file_path)
{
	Dictionary da;
	if (da.open(index_file_path.c_str()))
	{
		cerr << "Error: cannot open: " << index_file_path << endl;
		return 1;
	}

	vector<typename Dictionary::result_pair_type> result_pairs(1024);

	string line;
	while (getline(cin, line))
	{
		size_t num_of_results = da.commonPrefixSearch(
			line.c_str(), &result_pairs[0], result_pairs.size());
		if (num_of_results)
		{
			cout << line << ": found, num = " << num_of_results;
			for (size_t i = 0; i < num_of_results; ++i)
			{
				cout << ' ' << result_pairs[i].value
					<< ':' << result_pairs[i].length;
			}
			cout << endl;
		}
		else
			cout << line << ": not found" << endl;
	}

	return 0;
}

int darts(const string &index_file_path, const string &option)
{
	if (option == "-h")
		return darts<Darts::HugeDoubleArray>(index_file_path);

	return darts<Darts::DoubleArray>(index_file_path);
}

int main(int argc, char **argv)
{
	if (argc < 2 || argc > 3)
	{
		cerr << "Usage: " << argv[0] << " [-h] IndexFile" << endl;
		return 1;
	}

	const string option(argc > 2 ? argv[1] : "");
	const string index_file_path = argv[argc - 1];

	return darts(index_file_path, option);
}
