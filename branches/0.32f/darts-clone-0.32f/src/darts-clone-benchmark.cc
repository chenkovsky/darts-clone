// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"
#include "timer.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <vector>

namespace {

//
// Classes and functions for reading files are defined below.
//

class KeyLessThan
{
public:
	bool operator()(const char *lhs, const char *rhs) const
	{
		while (*lhs != '\0' && *lhs == *rhs)
			++lhs, ++rhs;
		return static_cast<unsigned char>(*lhs)
			< static_cast<unsigned char>(*rhs);
	}
};

class KeyEqualTo
{
public:
	bool operator()(const char *lhs, const char *rhs) const
	{
		while (*lhs != '\0' && *lhs == *rhs)
			++lhs, ++rhs;
		return *lhs == *rhs;
	}
};

// Loads a whole file and divides it to lines.
bool LoadLines(const char *file_name,
	std::vector<char> *lines_buf, std::vector<const char *> *lines)
{
	std::ifstream file(file_name, std::ios::binary);
	if (!file)
	{
		std::cerr << "error: failed to open file: " << file_name << std::endl;
		return false;
	}

	file.seekg(0, std::ios::end);
	std::ifstream::pos_type file_size = file.tellg();
	if (file_size <= 0)
		return false;
	file.seekg(0, std::ios::beg);

	lines_buf->resize(static_cast<std::size_t>(file_size) + 1);
	if (!file.read(&(*lines_buf)[0], lines_buf->size() - 1))
		return false;
	lines_buf->back() = '\0';

	bool is_valid_line = false;
	for (std::size_t i = 0; i < lines_buf->size() - 1; ++i)
	{
		if ((*lines_buf)[i] == '\r' || (*lines_buf)[i] == '\n')
		{
			(*lines_buf)[i] = '\0';
			is_valid_line = false;
		}
		else if (!is_valid_line)
		{
			lines->push_back(&(*lines_buf)[i]);
			is_valid_line = true;
		}
	}

	return true;
}

// Loads a keyset file.
bool LoadKeyset(const char *file_name,
	std::vector<char> *keys_buf, std::vector<const char *> *keys)
{
	if (!LoadLines(file_name, keys_buf, keys))
		return false;

	if (keys->empty())
	{
		std::cerr << "error: empty keyset" << std::endl;
		return false;
	}

	std::cerr << "sorting keyset..." << std::endl;
	std::sort(keys->begin(), keys->end(), KeyLessThan());

	std::vector<const char *>::iterator unique_it =
		std::unique(keys->begin(), keys->end(), KeyEqualTo());
	keys->erase(unique_it, keys->end());
	std::cerr << "number of unique keywords: " << keys->size() << std::endl;

	return true;
}

// Loads a text file.
bool LoadText(const char *file_name,
	std::vector<char> *lines_buf, std::vector<const char *> *lines)
{
	if (file_name == NULL)
		return true;

	if (!LoadLines(file_name, lines_buf, lines))
		return false;

	return true;
}

//
// The following class is a random number generator.
//

class MersenneTwister
{
private:
	// Constant parameters.
	enum { N = 624, M = 397 };

public:
	// 32-bit unsigned integer.
	typedef unsigned int UInt32;

	// Initializes an internal array.
	explicit MersenneTwister(UInt32 seed = 0)
		: mt_(N), mti_(0) { Init(seed); }

	// Initializes an internal array.
	void Init(UInt32 seed)
	{
		mt_[0]= seed;
		for (mti_ = 1; mti_ < N; ++mti_)
		{
			mt_[mti_] = 1812433253UL
				* (mt_[mti_ - 1] ^ (mt_[mti_ - 1] >> 30)) + mti_;
		}
	}

	// Generates a random number [0, 0xFFFFFFFFUL].
	UInt32 Generate()
	{
		UInt32 y;
		if (mti_ >= N)
		{
			int i = 0;
			for (i = 0; i < N - M; i++)
			{
				y = ToUpper(mt_[i]) | ToLower(mt_[i + 1]);
				mt_[i] = mt_[i + M] ^ (y >> 1) ^ mag01(y);
			}
			for (; i < N - 1; i++)
			{
				y = ToUpper(mt_[i]) | ToLower(mt_[i + 1]);
				mt_[i] = mt_[i + (M - N)] ^ (y >> 1) ^ mag01(y);
			}
			y = ToUpper(mt_[N - 1]) | ToLower(mt_[0]);
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
	UInt32 operator()(UInt32 limit) { return Generate() % limit; }

private:
	std::vector<UInt32> mt_;
	int mti_;

	static UInt32 mag01(UInt32 index)
	{
		static const UInt32 mag01[] = { 0, 0x9908B0DFUL };
		return mag01[index & 1];
	}
	static UInt32 ToUpper(UInt32 value) { return value & 0x80000000UL; }
	static UInt32 ToLower(UInt32 value) { return value & 0x7FFFFFFFUL; }
};

//
// Classes and functions for benchmarks are defined below.
//

// Timer recorder.
class TimeRecorder
{
public:
	static const std::size_t MIN_SIZE = 3;
	static const double MIN_TOTAL = 3.0;

	TimeRecorder() : values_(), min_(1.0e10), max_(0.0), total_(0.0) {}

	void Add(double value)
	{
		values_.push_back(value);
		if (value < min_)
			min_ = value;
		if (value > max_)
			max_ = value;
		total_ += value;
	}

	void Show()
	{
		std::cout << "Time: " << total() << ' ' << ave() << ' '
			<< med() << ' ' << min() << ' ' << max() << std::endl;
	}

	std::size_t size() const { return values_.size(); }
	double min() const { return min_; }
	double max() const { return max_; }
	double total() const { return total_; }
	double ave() const { return values_.empty() ? 0.0 : (total() / size()); }
	double med() const
	{
		std::vector<double> values_copy(values_);
		std::sort(values_copy.begin(), values_copy.end());
		return values_copy.empty() ? 0.0 : values_copy[size() / 2];
	}

	bool still_more() const
	{
		return (size() < MIN_SIZE) || (total() < MIN_TOTAL);
	}

private:
	std::vector<double> values_;
	double min_;
	double max_;
	double total_;

	// Disallows copies.
	TimeRecorder(const TimeRecorder &);
	TimeRecorder &operator=(const TimeRecorder &);
};

// Scoped timer.
class TimeWatch
{
public:
	TimeWatch(TimeRecorder *recorder) : recorder_(recorder), timer_() {}
	~TimeWatch() { recorder_->Add(timer_.Elapsed()); }

private:
	TimeRecorder *recorder_;
	Darts::Timer timer_;

	// Disallows copies.
	TimeWatch(const TimeWatch &);
	TimeWatch &operator=(const TimeWatch &);
};

// Benchmarks build().
bool BenchmarkBuild(Darts::DoubleArray *da,
	const std::vector<const char *> &keys)
{
//	std::vector<int> values(keys.size(), 0);

	TimeRecorder timer;
	while (timer.still_more())
	{
		TimeWatch watch(&timer);

		da->clear();
//		if (da->build(keys.size(), &keys[0], 0, &values[0]) != 0)
		if (da->build(keys.size(), &keys[0]) != 0)
			return false;
	}

	timer.Show();
	return true;
}

// Tests exactMatchSearch().
bool BenchmarkExactMatch(const Darts::DoubleArray &da,
	const std::vector<const char *> &keys)
{
	TimeRecorder timer;
	while (timer.still_more())
	{
		TimeWatch watch(&timer);

		for (std::size_t j = 0; j < keys.size(); ++j)
		{
			Darts::DoubleArray::value_type value;
			da.exactMatchSearch(keys[j], value);
			if (value == -1)
			{
				std::cerr << "error: failed to find key: "
					<< keys[j] << std::endl;
				return false;
			}
		}
	}

	timer.Show();
	return true;
}

// Tests commonPrefixSearch().
bool test_prefix_match(const Darts::DoubleArray &da,
	const std::vector<const char *> &lines)
{
	std::size_t total_matches = 0;

	TimeRecorder timer;
	while (timer.still_more())
	{
		TimeWatch watch(&timer);

		static const std::size_t RESULT_MAX = 256;
		Darts::DoubleArray::value_type results[RESULT_MAX];

		total_matches = 0;
		for (std::size_t j = 0; j < lines.size(); ++j)
		{
			const char *line = lines[j];
			for (std::size_t k = 0; line[k] != 0; ++k)
			{
				std::size_t num_of_matches = da.commonPrefixSearch(
					&line[k], results, RESULT_MAX, lines.size() - k);
				total_matches += num_of_matches;
			}
		}
	}
	std::cerr << "number of matches: " << total_matches << std::endl;

	timer.Show();
	return true;
}

// Tests traverse().
bool test_traverse(const Darts::DoubleArray &da,
	const std::vector<const char *> &lines)
{
	std::size_t total_matches = 0;

	TimeRecorder timer;
	while (timer.still_more())
	{
		TimeWatch watch(&timer);

		total_matches = 0;
		for (std::size_t i = 0; i < lines.size(); ++i)
		{
			const char *line = lines[i];
			for (std::size_t j = 0; line[j] != 0; ++j)
			{
				std::size_t da_index = 0, key_index = j;
				Darts::DoubleArray::value_type result;
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
	std::cerr << "number of matches: " << total_matches << std::endl;

	timer.Show();
	return true;
}

// Tests a specified double-array.
bool Benchmark(const std::vector<const char *> &keys,
	const std::vector<const char *> &lines)
{
	std::cerr << "building double-arrays..." << std::endl;
	Darts::DoubleArray da;
	if (!BenchmarkBuild(&da, keys))
		return false;
	std::cout << "Size: " << da.total_size() << std::endl;

	std::cerr << "matching sorted keyset..." << std::endl;
	if (!BenchmarkExactMatch(da, keys))
		return false;

	std::cerr << "randomizing keyset..." << std::endl;
	MersenneTwister mt;
	std::vector<const char *> mt_keys(keys);
	std::random_shuffle(mt_keys.begin(), mt_keys.end(), mt);

	std::cerr << "matching randomized keyset..." << std::endl;
	if (!BenchmarkExactMatch(da, mt_keys))
		return false;

	if (!lines.empty())
	{
		std::cerr << "prefix matching..." << std::endl;
		if (!test_prefix_match(da, lines))
			return false;

		std::cerr << "traversing..." << std::endl;
		if (!test_traverse(da, lines))
			return false;
	}

	return true;
}

// Tests a double-array.
bool BenchmarkMain(const std::vector<const char *> &keys,
	const std::vector<const char *> &lines)
{
	if (!Benchmark(keys, lines))
		return false;

	return true;
}

}  // namespace

int main(int argc, char *argv[])
{
	if (argc < 2 || argc > 3)
	{
		std::cerr << "Usage: " << argv[0]
			<< " KeyFile [TextFile]" << std::endl;
		return 1;
	}

	const char *key_file_name = argv[1];
	const char *text_file_name = (argc == 3) ? argv[2] : 0;

	// Loads a keyset.
	std::vector<char> keys_buf;
	std::vector<const char *> keys;
	if (!LoadKeyset(key_file_name, &keys_buf, &keys))
		return 1;

	// Loads a text if available.
	std::vector<char> lines_buf;
	std::vector<const char *> lines;
	if (!LoadText(text_file_name, &lines_buf, &lines))
		return 1;

	if (!BenchmarkMain(keys, lines))
		return 1;

	return 0;
}
