// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <ctime>

#include <algorithm>
#include <exception>
#include <fstream>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

using namespace std;

namespace
{

// Time recorder.
class time_recorder
{
public:
	time_recorder() : values_() {}
	void push_back(double value) { values_.push_back(value); }

	size_t size() const { return values_.size(); }
	double total() const
	{ return accumulate(values_.begin(), values_.end(), 0.0); }
	double ave() const { return size() ? (total() / size()) : 0.0; }
	double min() const { return *min_element(values_.begin(), values_.end()); }
	double max() const { return *max_element(values_.begin(), values_.end()); }
	double med() const
	{
		vector<double> values_copy(values_);
		sort(values_copy.begin(), values_copy.end());
		return size() ? values_copy[size() / 2] : 0.0;
	}

private:
	vector<double> values_;

	time_recorder(const time_recorder &);
	const time_recorder &operator=(const time_recorder &);
};

// Scope timer.
class time_watch
{
public:
	time_watch(time_recorder &recorder)
		: recorder_(recorder), begin_(clock()) {}
	~time_watch()
	{
		clock_t end = clock();
		double sec = 1.0 * (end - begin_) / CLOCKS_PER_SEC;
		recorder_.push_back(sec);
	}

private:
	time_recorder &recorder_;
	clock_t begin_;

	time_watch(const time_watch &);
	const time_watch &operator=(const time_watch &);
};

// Random number generator based on Mersenne Twister.
class mt_rand
{
private:
	// Constant parameters.
	enum { N = 624, M = 397 };

public:
	// 32-bit unsigned integer.
	typedef unsigned int uint32;

	// Initializes the internal array.
	mt_rand(uint32 seed = 0) { init(seed); }

	// Initializes the internal array.
	void init(uint32 seed)
	{
		mt_[0]= seed;
		for (mti_ = 1; mti_ < N; mti_++)
		{
			mt_[mti_] = 1812433253UL
				* (mt_[mti_ - 1] ^ (mt_[mti_ - 1] >> 30)) + mti_;
		}
	}

	// Generates a random number [0, 0xFFFFFFFFUL].
	uint32 gen()
	{
		uint32	y;

		if (mti_ >= N)
		{
			int i = 0;
			for (i = 0; i < N - M; i++)
			{
				y = to_upper(mt_[i]) | to_lower(mt_[i + 1]);
				mt_[i] = mt_[i + M] ^ (y >> 1) ^ mag01(y);
			}
			for (; i < N - 1; i++)
			{
				y = to_upper(mt_[i]) | to_lower(mt_[i + 1]);
				mt_[i] = mt_[i + (M - N)] ^ (y >> 1) ^ mag01(y);
			}
			y = to_upper(mt_[N - 1]) | to_lower(mt_[0]);
			mt_[N - 1] = mt_[M - 1] ^ (y >> 1) ^ mag01(y);

			mti_ = 0;
		}

		y = mt_[mti_++];

		y ^= (y >> 11);
		y ^= (y << 7) & 0x9D2C5680UL;
		y ^= (y << 15) & 0xEFC60000UL;
		y ^= (y >> 18);
		return y;
	}

	// Generates a random number for std::random_shuffle().
	uint32 operator()(uint32 limit) { return gen() % limit; }

private:
	uint32 mt_[N];
	int mti_;

	static uint32 mag01(uint32 index)
	{
		static const uint32 mag01[] = { 0, 0x9908B0DFUL };
		return mag01[index & 1];
	}
	static uint32 to_upper(uint32 value) { return value & 0x80000000UL; }
	static uint32 to_lower(uint32 value) { return value & 0x7FFFFFFFUL; }
};

struct KeyLessThan
{
	bool operator()(const char *lhs, const char *rhs)
	{
		while (*lhs != '\0' && *lhs == *rhs)
			++lhs, ++rhs;
		return static_cast<unsigned char>(*lhs)
			< static_cast<unsigned char>(*rhs);
	}
};

struct KeyEqualTo
{
	bool operator()(const char *lhs, const char *rhs)
	{
		while (*lhs != '\0' && *lhs == *rhs)
			++lhs, ++rhs;
		return *lhs == *rhs;
	}
};

// Loads a file and divide it to a list of lines.
bool load_lines(const char *file_name,
	vector<char> &lines_buf, vector<const char *> &lines)
{
	ifstream file(file_name);
	if (!file.is_open())
	{
		cerr << "error: failed to open file: " << file_name << endl;
		return false;
	}

	file.seekg(0, ios::end);
	ifstream::pos_type file_size = file.tellg();
	if (file_size <= 0)
		return false;
	file.seekg(0, ios::beg);

	lines_buf.resize(static_cast<vector<char>::size_type>(file_size) + 1);
	file.read(&lines_buf[0], lines_buf.size() - 1);
	if (file.eof() || file.fail())
		return false;
	lines_buf.back() = '\0';

	bool is_valid_line = false;
	for (size_t i = 0; i < lines_buf.size() - 1; ++i)
	{
		if (lines_buf[i] == '\r' || lines_buf[i] == '\n')
		{
			lines_buf[i] = '\0';
			is_valid_line = false;
		}
		else if (!is_valid_line)
		{
			lines.push_back(&lines_buf[i]);
			is_valid_line = true;
		}
	}

	return true;
}

// Loads a keyset file.
bool load_keyset(const char *key_file_name,
	vector<char> &keys_buf, vector<const char *> &keys)
{
	if (!load_lines(key_file_name, keys_buf, keys))
		return false;

	if (keys.empty())
	{
		cerr << "error: empty keyset" << endl;
		return false;
	}

	cerr << "sorting keyset..." << endl;
	sort(keys.begin(), keys.end(), KeyLessThan());

	keys.erase(unique(keys.begin(), keys.end(), KeyEqualTo()), keys.end());
	cerr << "number of unique keywords: " << keys.size() << endl;

	return true;
}

// Loads a text file.
bool load_text(const char *text_file_name,
	vector<char> &lines_buf, vector<const char *> &lines)
{
	if (!text_file_name)
		return true;

	if (!load_lines(text_file_name, lines_buf, lines))
		return false;

	return true;
}

// Minimum number of tests and minimum time of tests.
const int MIN_TEST_TIMES = 5;
const double MIN_TEST_SEC = 5.0;

// Tests build().
template <typename DoubleArray>
bool test_build(DoubleArray &da, const vector<const char *> &keys)
{
//	vector<int> values(keys.size(), 0);

	time_recorder timer;
	for (int i = 0; i < MIN_TEST_TIMES || timer.total() < MIN_TEST_SEC; ++i)
	{
		cerr << i << ", " << timer.total() << '\r';
		time_watch watch(timer);

		da.clear();
		try
		{
//			if (da.build(keys.size(), &keys[0], 0, &values[0]) != 0)
			if (da.build(keys.size(), &keys[0]) != 0)
				return false;
		}
		catch (exception &ex)
		{
			cerr << "error: failed to build:\n" << ex.what() << endl;
			return false;
		}
	}

	cout << "Time: " << timer.total() << ", " <<
		timer.ave() << ", " << timer.med() << ", " <<
		timer.min() << ", " << timer.max() << endl;
	return true;
}

// Tests exactMatchSearch().
template <typename DoubleArray>
bool test_exact_match(const DoubleArray &da, const vector<const char *> &keys)
{
	time_recorder timer;
	for (int i = 0; i < MIN_TEST_TIMES || timer.total() < MIN_TEST_SEC; ++i)
	{
		cerr << i << ", " << timer.total() << '\r';
		time_watch watch(timer);

		for (size_t j = 0; j < keys.size(); ++j)
		{
			typename DoubleArray::value_type value;
			da.exactMatchSearch(keys[j], value);
			if (value == -1)
			{
				cerr << "error: failed to find key: " << keys[j] << endl;
				return false;
			}
		}
	}

	cout << "Time: " << timer.total() << ", " <<
		timer.ave() << ", " << timer.med() << ", " <<
		timer.min() << ", " << timer.max() << endl;
	return true;
}

// Tests commonPrefixSearch().
template <typename DoubleArray>
bool test_prefix_match(const DoubleArray &da, const vector<const char *> &lines)
{
	time_recorder timer;
	size_t total_matches = 0;
	for (int i = 0; i < MIN_TEST_TIMES || timer.total() < MIN_TEST_SEC; ++i)
	{
		cerr << i << ", " << timer.total() << '\r';
		time_watch watch(timer);

		static const size_t RESULT_MAX = 256;
		typename DoubleArray::value_type results[RESULT_MAX];

		total_matches = 0;
		for (size_t j = 0; j < lines.size(); ++j)
		{
			const char *line = lines[j];
			for (size_t k = 0; line[k] != 0; ++k)
			{
				size_t num_of_matches = da.commonPrefixSearch(
					&line[k], results, RESULT_MAX, lines.size() - k);
				total_matches += num_of_matches;
			}
		}
	}
	cerr << "number of matches: " << total_matches << endl;

	cout << "Time: " << timer.total() << ", " <<
		timer.ave() << ", " << timer.med() << ", " <<
		timer.min() << ", " << timer.max() << endl;
	return true;
}

// Tests traverse().
template <typename DoubleArray>
bool test_traverse(const DoubleArray &da, const vector<const char *> &lines)
{
	time_recorder timer;
	size_t total_matches = 0;
	for (int i = 0; i < MIN_TEST_TIMES || timer.total() < MIN_TEST_SEC; ++i)
	{
		cerr << i << ", " << timer.total() << '\r';
		time_watch watch(timer);

		total_matches = 0;
		for (size_t j = 0; j < lines.size(); ++j)
		{
			const char *line = lines[j];
			for (size_t k = 0; line[k] != 0; ++k)
			{
				size_t da_index = 0, key_index = k;
				typename DoubleArray::value_type result;
				do
				{
					result = da.traverse(&line[0], da_index,
						key_index, key_index + 1);
					if (result >= 0)
						++total_matches;
				} while (line[key_index] != 0 && result != -2);
			}
		}
	}
	cerr << "number of matches: " << total_matches << endl;

	cout << "Time: " << timer.total() << ", " <<
		timer.ave() << ", " << timer.med() << ", " <<
		timer.min() << ", " << timer.max() << endl;
	return true;
}

// Tests a specified double-array.
template <typename DoubleArray>
bool test(vector<const char *> &keys, const vector<const char *> &lines)
{
	cerr << "building double-arrays..." << endl;
	DoubleArray da;
	if (!test_build(da, keys))
		return false;
	cout << "Size: " << da.total_size() << endl;

	cerr << "matching sorted keyset..." << endl;
	if (!test_exact_match(da, keys))
		return false;

	cerr << "randomizing keyset..." << endl;
	mt_rand rng;
	random_shuffle(keys.begin(), keys.end(), rng);

	cerr << "matching randomized keyset..." << endl;
	if (!test_exact_match(da, keys))
		return false;

	if (!lines.empty())
	{
		cerr << "prefix matching..." << endl;
		if (!test_prefix_match(da, lines))
			return false;

		cerr << "traversing..." << endl;
		if (!test_traverse(da, lines))
			return false;
	}

	return true;
}

// Tests a double-array.
bool time_main(vector<const char *> &keys, const vector<const char *> &lines)
{
	if (!test<Darts::DoubleArray>(keys, lines))
		return false;

	sort(keys.begin(), keys.end(), KeyLessThan());
	if (!test<Darts::HugeDoubleArray>(keys, lines))
		return false;

	return true;
}

}  // namespace

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3)
	{
		cerr << "Usage: " << argv[0] << " KeyFile [TextFile]" << endl;
		return 1;
	}

	const char *key_file_name = argv[1];
	const char *text_file_name = (argc == 3) ? argv[2] : 0;

	vector<char> keys_buf;
	vector<const char *> keys;
	if (!load_keyset(key_file_name, keys_buf, keys))
		return 1;

	vector<char> lines_buf;
	vector<const char *> lines;
	if (!load_text(text_file_name, lines_buf, lines))
		return 1;

	if (!time_main(keys, lines))
		return 1;

	return 0;
}
