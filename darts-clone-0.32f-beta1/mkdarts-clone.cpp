// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008 Susumu Yata <syata@acm.org>
// All rights reserved.

#include "darts-clone.h"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

using namespace std;

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

}  // namespace

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

template <typename Dictionary>
int mkdarts(istream &key_input_stream, const string &index_file_path)
{
	vector<char> keys_buf;
	vector<const char *> keys;
	input_keys(key_input_stream, keys_buf, keys);

	Dictionary da;
	if (da.build(keys.size(), &keys[0], 0, 0, ProgressBar()) != 0)
	{
		cerr << "\nError: cannot build double-array" << endl;
		return 1;
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

int mkdarts(istream &key_input_stream, const string &index_file_path)
{
	return mkdarts<Darts::DoubleArray>(key_input_stream, index_file_path);
}

int main(int argc, char **argv)
{
	if (argc != 3)
	{
		cerr << "Usage: " << argv[0] << " KeyFile IndexFile" << endl;
		return 1;
	}

	const string key_file_path(argv[1]);
	const string index_file_path(argv[2]);

	if (key_file_path == "-")
		return mkdarts(cin, index_file_path);

	ifstream key_file(key_file_path.c_str());
	if (!key_file.is_open())
	{
		cerr << "Error: cannot open: " << key_file_path << endl;
		return 1;
	}

	return mkdarts(key_file, index_file_path);
}
