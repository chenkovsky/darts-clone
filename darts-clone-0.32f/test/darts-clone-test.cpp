// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "../src/darts-clone.h"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#define ERR (std::cerr << __FILE__ << ':' << __LINE__ << " :error: ")

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
		if (cur_percentage == 100)
			std::printf("\n");
		else
			std::printf("\r");
		std::fflush(stdout);
	}

	prev = cur_percentage;

	return 1;
};

template <typename KeyPointerPointer>
int TestExactMatchSearch(const Darts::DoubleArray &da,
	const std::set<std::string> &key_set, KeyPointerPointer keys)
{
	typedef Darts::DoubleArray::value_type value_type;
	typedef Darts::DoubleArray::result_pair_type result_pair_type;

	for (std::size_t i = 0; i < key_set.size(); ++i)
	{
		value_type result;
		da.exactMatchSearch(keys[i], result);
		if (static_cast<std::size_t>(result) != i)
		{
			ERR << "exactMatchSearch() failed: " << result << std::endl;
			return 1;
		}

		result_pair_type result_pair;
		result_pair.length = 0;
		da.exactMatchSearch(keys[i], result_pair);
		if (static_cast<std::size_t>(result_pair.value) != i
			|| result_pair.length != strlen(keys[i]))
		{
			ERR << "exactMatchSearch() failed: " << result_pair.value
				<< ", " << result_pair.length << std::endl;
			return 1;
		}
	}

	return 0;
}

template <typename KeyPointerPointer>
int TestCommonPrefixSearch(const Darts::DoubleArray &da,
	const std::set<std::string> &key_set, KeyPointerPointer keys)
{
	typedef Darts::DoubleArray::value_type value_type;
	typedef Darts::DoubleArray::result_pair_type result_pair_type;

	enum { MAX_NUM_OF_RESULTS = 256 };

	value_type results[MAX_NUM_OF_RESULTS];
	result_pair_type result_pairs[MAX_NUM_OF_RESULTS];

	for (std::size_t i = 0; i < key_set.size(); ++i)
	{
		std::size_t num_of_results = da.commonPrefixSearch(
			keys[i], results, MAX_NUM_OF_RESULTS);
		std::size_t num_of_result_pairs = da.commonPrefixSearch(
			keys[i], result_pairs, MAX_NUM_OF_RESULTS);

		if (!num_of_results || num_of_results != num_of_result_pairs)
		{
			ERR << "commonPrefixSearch() failed: "
				<< num_of_results << ", " << num_of_result_pairs << std::endl;
			return 1;
		}

		const value_type &result = results[num_of_results - 1];
		const result_pair_type &result_pair = result_pairs[num_of_results - 1];

		if (num_of_results <= MAX_NUM_OF_RESULTS
			&& (static_cast<std::size_t>(result) != i
				|| static_cast<std::size_t>(result_pair.value) != i))
		{
			ERR << "commonPrefixSearch() failed: "
				<< results[num_of_results - 1] << ", "
				<< result_pairs[num_of_results - 1].value << std::endl;
			return 1;
		}

		if (num_of_results > MAX_NUM_OF_RESULTS)
			num_of_results = num_of_result_pairs = MAX_NUM_OF_RESULTS;
		for (std::size_t j = 0; j < num_of_results; ++j)
		{
			if (results[j] != result_pairs[j].value)
			{
				ERR << "commonPrefixSearch() failed: "
					<< results[j] << std::endl;
				return 1;
			}
		}
	}

	return 0;
}

template <typename KeyPointerPointer>
int TestTraverse(const Darts::DoubleArray &da,
	const std::set<std::string> &key_set, KeyPointerPointer keys)
{
	typedef Darts::DoubleArray::value_type value_type;

	for (std::size_t i = 0; i < key_set.size(); ++i)
	{
		std::size_t length = strlen(keys[i]);

		for (std::size_t j = 1; j <= length; ++j)
		{
			std::size_t da_index = 0;
			std::size_t key_index = 0;
			value_type value = da.traverse(keys[i], da_index, key_index, j);
			if (value == static_cast<value_type>(-2))
			{
				ERR << "traverse() failed: " << value << std::endl;
				return 1;
			}

			value = da.traverse(keys[i], da_index, key_index, length);
			if (value != static_cast<value_type>(i))
			{
				ERR << "traverse() failed: " << value << std::endl;
				return 1;
			}
		}
	}

	return 0;
}

template <typename KeyPointerPointer>
int TestSearch(const Darts::DoubleArray &da,
	const std::set<std::string> &key_set, KeyPointerPointer keys)
{
	if (TestExactMatchSearch(da, key_set, keys) != 0)
		return 1;

	if (TestCommonPrefixSearch(da, key_set, keys) != 0)
		return 1;

	if (TestTraverse(da, key_set, keys) != 0)
		return 1;

	return 0;
}

template <typename KeyPointerPointer>
int TestDoubleArray(Darts::DoubleArray &da,
	const std::set<std::string> &key_set, KeyPointerPointer keys)
{
	if (da.build(key_set.size(), keys) != 0 ||
		TestSearch(da, key_set, keys) != 0 ||
		da.build(key_set.size(), keys, 0) != 0 ||
		TestSearch(da, key_set, keys) != 0 ||
		da.build(key_set.size(), keys, 0, 0) != 0 ||
		TestSearch(da, key_set, keys) != 0 ||
		da.build(key_set.size(), keys, 0, 0, ShowProgress) != 0 ||
		TestSearch(da, key_set, keys) != 0 ||
		da.build(key_set.size(), keys, 0, 0, 0) != 0 ||
		TestSearch(da, key_set, keys) != 0)
	{
		return 1;
	}

	return 0;
}

template <typename KeyPointer>
int TestDoubleArray(const std::set<std::string> &key_set, KeyPointer *keys)
{
	Darts::DoubleArray da;

	if (TestDoubleArray(da, key_set, keys) != 0 ||
		TestDoubleArray(da, key_set,
			const_cast<const KeyPointer *>(keys)) != 0)
	{
		return 1;
	}

	return 0;
}

int TestDoubleArray(const std::set<std::string> &key_set,
	const std::vector<const char *> &keys)
{
	if (TestDoubleArray(key_set, const_cast<char **>(&keys[0])) != 0)
		return 1;

	if (TestDoubleArray(key_set, &keys[0]) != 0)
		return 1;

	return 0;
}

}  // namespace

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		std::cerr << "Usage: " << argv[0] << " KeyFile" << std::endl;
		return 1;
	}

	const std::string key_file_path(argv[1]);
	std::ifstream key_file(key_file_path.c_str());
	if (!key_file.is_open())
	{
		ERR << "failed to open file: " << key_file_path << std::endl;
		return 1;
	}

	std::string key;
	std::set<std::string> key_set;
	while (std::getline(key_file, key))
		key_set.insert(key);

	std::vector<const char *> keys;
	keys.reserve(key_set.size());
	for (std::set<std::string>::iterator it = key_set.begin();
		it != key_set.end(); ++it)
		keys.push_back(it->c_str());

	if (TestDoubleArray(key_set, keys) != 0)
		return 1;

	return 0;
}
