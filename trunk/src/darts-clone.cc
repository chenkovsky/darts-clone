// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <iostream>
#include <string>
#include <vector>

namespace {

int MainProcess(const std::string &index_file_path)
{
	Darts::DoubleArray da;
	if (da.open(index_file_path.c_str()))
	{
		std::cerr << "Error: cannot open: " << index_file_path << std::endl;
		return 1;
	}

	std::vector<Darts::DoubleArray::result_pair_type> result_pairs(1024);

	std::string line;
	while (std::getline(std::cin, line))
	{
		std::size_t num_of_results = da.commonPrefixSearch(
			line.c_str(), &result_pairs[0], result_pairs.size());
		if (num_of_results)
		{
			std::cout << line << ": found, num = " << num_of_results;
			for (std::size_t i = 0; i < num_of_results; ++i)
			{
				std::cout << ' ' << result_pairs[i].value
					<< ':' << result_pairs[i].length;
			}
			std::cout << std::endl;
		}
		else
			std::cout << line << ": not found" << std::endl;
	}

	return 0;
}

}  // namespace

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " IndexFile" << std::endl;
		return 1;
	}

	const std::string index_file_path = argv[1];

	return MainProcess(index_file_path);
}
