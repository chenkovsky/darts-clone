// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <iostream>
#include <string>
#include <vector>

using namespace std;

int darts(const string &index_file_path)
{
	Darts::DoubleArray da;
	if (da.open(index_file_path.c_str()))
	{
		cerr << "Error: cannot open: " << index_file_path << endl;
		return 1;
	}

	vector<Darts::DoubleArray::result_pair_type> result_pairs(1024);

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

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " IndexFile" << endl;
		return 1;
	}

	const string index_file_path = argv[1];

	return darts(index_file_path);
}
