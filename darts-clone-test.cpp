// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

#define ERR (cerr << __FILE__ << ':' << __LINE__ << " :error: ")

namespace
{

class ProgressBar
{
public:
	ProgressBar() : prev_progress_(0) {}

	void operator()(size_t current, size_t total)
	{
		size_t progress   = static_cast<int>(100.0 * current / total);
		size_t bar_length = static_cast<int>(1.0 * current * Size / total);

		if (prev_progress_ != progress)
		{
			cerr << "Making double-array: " << setw(3) << progress << '%'
				<< " |";
			cerr.write(Bar, bar_length);
			cerr.write(Space, Size - bar_length);
			cerr << '|' << (progress == 100 ? '\n' : '\r');
			prev_progress_ = progress;
		}
	}

private:
	size_t prev_progress_;

	static const char Bar[];
	static const char Space[];
	static const size_t Size;
};

const char ProgressBar::Bar[] =
	"*******************************************";
const char ProgressBar::Space[] =
	"                                           ";
const size_t ProgressBar::Size = sizeof(ProgressBar::Bar) - 1;

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
		if (cur_percentage == 100)
			printf("\n");
		else
			printf("\r");
		fflush(stdout);
	}

	prev = cur_percentage;

	return 1;
};

}  // namespace

template <typename Dictionary, typename KeyPointerPointer>
int test_darts_exact_match_search(const Dictionary &da,
	size_t num_of_keys, KeyPointerPointer keys)
{
	typedef typename Dictionary::value_type value_type;
	typedef typename Dictionary::result_pair_type result_pair_type;

	for (size_t i = 0; i < num_of_keys; ++i)
	{
		value_type result;
		if (da.exactMatchSearch(keys[i], result) != 0)
		{
			ERR << "exactMatchSearch() failed: " << endl;
			return 1;
		}

		if (static_cast<size_t>(result) != i)
		{
			ERR << "exactMatchSearch() failed: " << result << endl;
			return 1;
		}

		result_pair_type result_pair;
		if (da.exactMatchSearch(keys[i], result_pair) != 0 ||
			static_cast<size_t>(result_pair.value) != i ||
			result_pair.length != strlen(keys[i]))
		{
			ERR << "exactMatchSearch() failed: "
				<< result_pair.value << ", " << result_pair.length << endl;
			return 1;
		}
	}

	return 0;
}

template <typename Dictionary, typename KeyPointerPointer>
int test_darts_common_prefix_search(const Dictionary &da,
	size_t num_of_keys, KeyPointerPointer keys)
{
	typedef typename Dictionary::value_type value_type;
	typedef typename Dictionary::result_pair_type result_pair_type;

	enum { MAX_NUM_OF_RESULTS = 256 };

	value_type results[MAX_NUM_OF_RESULTS];
	result_pair_type result_pairs[MAX_NUM_OF_RESULTS];

	for (size_t i = 0; i < num_of_keys; ++i)
	{
		size_t num_of_results = da.commonPrefixSearch(
			keys[i], results, MAX_NUM_OF_RESULTS);
		size_t num_of_result_pairs = da.commonPrefixSearch(
			keys[i], result_pairs, MAX_NUM_OF_RESULTS);

		if (!num_of_results || num_of_results != num_of_result_pairs)
		{
			ERR << "commonPrefixSearch() failed: "
				<< num_of_results << ", " << num_of_result_pairs << endl;
			return 1;
		}

		if (num_of_results <= MAX_NUM_OF_RESULTS &&
			(static_cast<size_t>(results[num_of_results - 1]) != i ||
			static_cast<size_t>(result_pairs[num_of_results - 1].value) != i))
		{
			ERR << "commonPrefixSearch() failed: "
				<< results[num_of_results - 1] << ", "
				<< result_pairs[num_of_results - 1].value << endl;
			return 1;
		}

		if (num_of_results > MAX_NUM_OF_RESULTS)
			num_of_results = num_of_result_pairs = MAX_NUM_OF_RESULTS;
		for (size_t j = 0; j < num_of_results; ++j)
		{
			if (results[j] != result_pairs[j].value)
			{
				ERR << "commonPrefixSearch() failed: " << results[j] << endl;
				return 1;
			}
		}
	}

	return 0;
}

template <typename IndexType, typename Dictionary, typename KeyPointerPointer>
int test_darts_traverse(const Dictionary &da,
	size_t num_of_keys, KeyPointerPointer keys)
{
	typedef typename Dictionary::value_type value_type;

	for (size_t i = 0; i < num_of_keys; ++i)
	{
		size_t length = strlen(keys[i]);

		for (size_t j = 0; j <= length; ++j)
		{
			IndexType da_index = 0;
			size_t key_index = 0;
			value_type value = da.traverse(keys[i], da_index, key_index, j);
			if (value == static_cast<value_type>(-2))
			{
				ERR << "traverse() failed: " << value << endl;
				return 1;
			}

			value = da.traverse(keys[i], da_index, key_index, length);
			if (value != static_cast<value_type>(i))
			{
				ERR << "traverse() failed: " << value << endl;
				return 1;
			}
		}
	}

	return 0;
}

template <typename Dictionary, typename KeyPointerPointer>
int test_darts_matching(const Dictionary &da,
	size_t num_of_keys, KeyPointerPointer keys)
{
	if (test_darts_exact_match_search(da, num_of_keys, keys) != 0)
		return 1;

	if (test_darts_common_prefix_search(da, num_of_keys, keys) != 0)
		return 1;

	if (test_darts_traverse<size_t>(da, num_of_keys, keys) != 0)
		return 1;

	if (test_darts_traverse<typename Dictionary::base_type>(
		da, num_of_keys, keys) != 0)
		return 1;

	return 0;
}

template <typename Dictionary, typename KeyPointerPointer>
int test_darts(Dictionary &da, size_t num_of_keys, KeyPointerPointer keys)
{
	if (da.build(num_of_keys, keys) != 0 ||
		test_darts_matching(da, num_of_keys, keys) != 0 ||
		da.build(num_of_keys, keys, 0) != 0 ||
		test_darts_matching(da, num_of_keys, keys) != 0 ||
		da.build(num_of_keys, keys, 0, 0) != 0 ||
		test_darts_matching(da, num_of_keys, keys) != 0 ||
		da.build(num_of_keys, keys, 0, 0, ProgressBar()) != 0 ||
		test_darts_matching(da, num_of_keys, keys) != 0 ||
		da.build(num_of_keys, keys, 0, 0, progress_bar) != 0 ||
		test_darts_matching(da, num_of_keys, keys) != 0 ||
		da.build(num_of_keys, keys, 0, 0, 0) != 0 ||
		test_darts_matching(da, num_of_keys, keys) != 0)
	{
		return 1;
	}

	return 0;
}

template <typename Dictionary, typename KeyPointer>
int test_darts(size_t num_of_keys, KeyPointer *keys)
{
	Dictionary da;

	if (test_darts(da, num_of_keys, keys) != 0 ||
		test_darts(da, num_of_keys, const_cast<const KeyPointer *>(keys)) != 0)
	{
		return 1;
	}

	return 0;
}

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		cerr << "Usage: " << argv[0] << " KeyFile" << endl;
		return 1;
	}

	const string key_file_path(argv[1]);
	ifstream key_file(key_file_path.c_str());
	if (!key_file.is_open())
	{
		ERR << "failed to open file: " << key_file_path << endl;
		return 1;
	}

	string key;
	vector<string> keys;
	while (getline(key_file, key))
		keys.push_back(key);

	vector<const char *> key_ptrs(keys.size());
	for (size_t i = 0; i < keys.size(); ++i)
		key_ptrs[i] = keys[i].c_str();

	if (test_darts<Darts::DoubleArray>(
		keys.size(), const_cast<char **>(&key_ptrs[0])) != 0 ||
		test_darts<Darts::DoubleArray>(keys.size(), &key_ptrs[0]) != 0)
	{
		return 1;
	}

	if (test_darts<Darts::HugeDoubleArray>(
		keys.size(), const_cast<char **>(&key_ptrs[0])) != 0 ||
		test_darts<Darts::HugeDoubleArray>(keys.size(), &key_ptrs[0]) != 0)
	{
		return 1;
	}

	return 0;
}
