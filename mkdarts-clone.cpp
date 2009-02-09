// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

int progress_bar(size_t current, size_t total)
{
	static const char bar[] = "*******************************************";
	static const int scale = sizeof(bar) - 1;

	static int prev = 0;

	int cur_percentage = static_cast<int>(100.0 * current / total);
	int bar_len = static_cast<int>(1.0 * current * scale / total);

	if (prev != cur_percentage)
	{
		printf("Making double-array: %3d%% |%.*s%*s|",
			cur_percentage, bar_len, bar, scale - bar_len, "");
		if (cur_percentage >= 100)
			printf("\n");
		else
			printf("\r");
		fflush(stdout);
	}

	prev = cur_percentage;

	return 1;
};

void input_keys(istream &key_input_stream,
	vector<char> &keys_buf, vector<const char *> &keys)
{
	string key_string;
	vector<size_t> indices;
	while (getline(key_input_stream, key_string))
	{
		indices.push_back(keys_buf.size());
		keys_buf.insert(keys_buf.end(), key_string.begin(), key_string.end());
		keys_buf.push_back('\0');
	}
	vector<char>(keys_buf).swap(keys_buf);
	keys.resize(indices.size());
	for (size_t i = 0; i < indices.size(); ++i)
		keys[i] = &keys_buf[indices[i]];
}

int build_da(istream &key_input_stream,
	const string &index_file_path, bool no_value)
{
	vector<char> keys_buf;
	vector<const char *> keys;
	input_keys(key_input_stream, keys_buf, keys);

	Darts::DoubleArray da;
	if (no_value)
	{
		vector<int> values(keys.size(), 0);
		if (da.build(keys.size(), &keys[0], 0, &values[0], progress_bar) != 0)
		{
			cerr << "\nError: cannot build double-array" << endl;
			return 1;
		}
	}
	else
	{
		if (da.build(keys.size(), &keys[0], 0, 0, progress_bar) != 0)
		{
			cerr << "\nError: cannot build double-array" << endl;
			return 1;
		}
	}

	if (da.save(index_file_path.c_str()) != 0)
	{
		cerr << "Error: cannot save double-array: " << index_file_path << endl;
		return 1;
	}

	cout << "Done! Compression ratio: " <<
	    100.0 * da.nonzero_size() / da.size() << " %" << std::endl;

	return 0;
}

int mkdarts(istream &key_input_stream,
	const string &index_file_path, bool no_value)
{
	try
	{
		return build_da(key_input_stream, index_file_path, no_value);
	}
	catch (const std::exception &ex)
	{
		cerr << "Error: " << ex.what() << endl;
	}

	return 1;
}

void show_usage(const char *cmd)
{
	cerr << "Usage: " << cmd << " [-n] KeyFile IndexFile" << endl;
}

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 4)
	{
		show_usage(argv[0]);
		return 1;
	}

	const string option(argc > 3 ? argv[1] : "");
	const string key_file_path(argv[1 + (argc > 3)]);
	const string index_file_path(argv[2 + (argc > 3)]);

	if (option != "" && option != "-n")
	{
		show_usage(argv[0]);
		return 1;
	}
	bool no_value = (option == "-n");

	if (key_file_path == "-")
		return mkdarts(cin, index_file_path, no_value);

	ifstream key_file(key_file_path.c_str());
	if (!key_file.is_open())
	{
		cerr << "Error: cannot open: " << key_file_path << endl;
		return 1;
	}

	return mkdarts(key_file, index_file_path, no_value);
}
