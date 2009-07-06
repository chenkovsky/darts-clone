// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace {

int ShowProgress(std::size_t current, std::size_t total)
{
	static const char bar[] = "*******************************************";
	static const int scale = sizeof(bar) - 1;

	static int prev = 0;

	int cur_percentage = static_cast<int>(100.0 * current / total);
	int bar_len = static_cast<int>(1.0 * current * scale / total);

	if (prev != cur_percentage)
	{
		std::printf("Making double-array: %3d%% |%.*s%*s|",
			cur_percentage, bar_len, bar, scale - bar_len, "");
		if (cur_percentage >= 100)
			std::printf("\n");
		else
			std::printf("\r");
		std::fflush(stdout);
	}

	prev = cur_percentage;

	return 1;
};

void InputKeys(std::istream *key_input_stream,
	std::vector<char> *keys_buf, std::vector<const char *> *keys)
{
	std::string key_string;
	std::vector<std::size_t> indices;
	while (std::getline(*key_input_stream, key_string))
	{
		indices.push_back(keys_buf->size());
		keys_buf->insert(keys_buf->end(),
			key_string.begin(), key_string.end());
		keys_buf->push_back('\0');
	}
	std::vector<char>(*keys_buf).swap(*keys_buf);
	keys->resize(indices.size());
	for (std::size_t i = 0; i < indices.size(); ++i)
		(*keys)[i] = &(*keys_buf)[indices[i]];
}

int BuildDoubleArray(std::istream *key_input_stream,
	const std::string &index_file_path, bool no_value)
{
	std::vector<char> keys_buf;
	std::vector<const char *> keys;
	InputKeys(key_input_stream, &keys_buf, &keys);

	Darts::DoubleArray da;
	if (no_value)
	{
		std::vector<int> values(keys.size(), 0);
		if (da.build(keys.size(), &keys[0], 0, &values[0], ShowProgress) != 0)
		{
			std::cerr << "\nError: cannot build double-array" << std::endl;
			return 1;
		}
	}
	else
	{
		if (da.build(keys.size(), &keys[0], 0, 0, ShowProgress) != 0)
		{
			std::cerr << "\nError: cannot build double-array" << std::endl;
			return 1;
		}
	}

	if (da.save(index_file_path.c_str()) != 0)
	{
		std::cerr << "Error: cannot save double-array: "
			<< index_file_path << std::endl;
		return 1;
	}

	std::cout << "Done! Compression ratio: " <<
	    100.0 * da.nonzero_size() / da.size() << " %" << std::endl;

	return 0;
}

int MainProcess(std::istream *key_input_stream,
	const std::string &index_file_path, bool no_value)
{
	try
	{
		return BuildDoubleArray(key_input_stream, index_file_path, no_value);
	}
	catch (const std::exception &ex)
	{
		std::cerr << "Error: " << ex.what() << std::endl;
	}

	return 1;
}

void ShowUsage(const char *cmd)
{
	std::cerr << "Usage: " << cmd << " [-n] KeyFile IndexFile" << std::endl;
}

}  // namespace

int main(int argc, char **argv)
{
	if (argc != 3 && argc != 4)
	{
		ShowUsage(argv[0]);
		return 1;
	}

	const std::string option(argc > 3 ? argv[1] : "");
	const std::string key_file_path(argv[1 + (argc > 3)]);
	const std::string index_file_path(argv[2 + (argc > 3)]);

	if (option != "" && option != "-n")
	{
		ShowUsage(argv[0]);
		return 1;
	}
	bool no_value = (option == "-n");

	if (key_file_path == "-")
		return MainProcess(&std::cin, index_file_path, no_value);

	std::ifstream key_file(key_file_path.c_str());
	if (!key_file.is_open())
	{
		std::cerr << "Error: cannot open: " << key_file_path << std::endl;
		return 1;
	}

	return MainProcess(&key_file, index_file_path, no_value);
}
