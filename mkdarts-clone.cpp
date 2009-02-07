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

template <typename Dictionary>
int mkdarts(istream &key_input_stream, const string &index_file_path)
{
	string key_string;
	vector<string> key_strings;
	while (getline(key_input_stream, key_string))
		key_strings.push_back(key_string);

	vector<const char *> keys;
	for (vector<string>::const_iterator it = key_strings.begin();
		it != key_strings.end(); ++it)
	{
		keys.push_back(it->c_str());
	}

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

int mkdarts(istream &key_input_stream, const string &index_file_path,
	const string &option)
{
	if (option == "-h")
	{
		return mkdarts<Darts::HugeDoubleArray>(
			key_input_stream, index_file_path);
	}

	return mkdarts<Darts::DoubleArray>(key_input_stream, index_file_path);
}

int main(int argc, char **argv)
{
	if (argc < 3 || argc > 4)
	{
		cerr << "Usage: " << argv[0] << " [-h] KeyFile IndexFile" << endl;
		return 1;
	}

	const string option(argc > 3 ? argv[1] : "");
	const string key_file_path(argv[argc - 2]);
	const string index_file_path(argv[argc - 1]);

	if (key_file_path == "-")
		return mkdarts(cin, index_file_path, option);

	ifstream key_file(key_file_path.c_str());
	if (!key_file.is_open())
	{
		cerr << "Error: cannot open: " << key_file_path << endl;
		return 1;
	}

	return mkdarts(key_file, index_file_path, option);
}
