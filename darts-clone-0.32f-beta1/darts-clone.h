#ifndef DARTS_H_
#define DARTS_H_

// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#define DARTS_VERSION "0.32"
#define DARTS_CLONE_VERSION "0.32f"

#ifdef _MSC_VER
#include <stdio.h>
#include <share.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <exception>
#include <stack>
#include <vector>

// Defines macros for throwing exceptions with line numbers.
#define THROW(msg) THROW_RELAY(__LINE__, msg)
#define THROW_RELAY(line, msg) THROW_FINAL(line, msg)
#define THROW_FINAL(line, msg) throw \
	exception_type("darts-clone-" DARTS_CLONE_VERSION " [" # line "]: " msg)

namespace Darts
{

// Exception class.
class DoubleArrayException : public std::exception
{
private:
	const char *msg_;

public:
	// A constant string should be passed.
	template <int Size>
	explicit DoubleArrayException(const char (&msg)[Size]) : msg_(msg) {}

	const char *what() const throw() { return msg_; }
};

// Common types are defined here.
class DoubleArrayCommonTypes
{
public:
	typedef char char_type;
	typedef unsigned char uchar_type;
	// Must be a 32-bit unsigned integer type.
	typedef unsigned int base_type;
	typedef std::size_t size_type;

	// For compatibility.
	typedef char_type key_type;

	typedef DoubleArrayException exception_type;
};

// The default progress function does nothing.
class DefaultProgressFunc : public DoubleArrayCommonTypes
{
public:
	int operator()(size_type, size_type) const { return 1; }
};

// File I/O.
class DoubleArrayFile : DoubleArrayCommonTypes
{
private:
	std::FILE *file_;

public:
	DoubleArrayFile(const char *file_name, const char *mode) : file_(0)
	{
		if (!file_name)
			THROW("Null file name");
		else if (!mode)
			THROW("Null file mode");

#ifdef _MSC_VER
		// To avoid warnings against std::fopen().
		file_ = _fsopen(file_name, mode, _SH_DENYWR);
#else
		file_ = std::fopen(file_name, mode);
#endif
	}
	~DoubleArrayFile() { is_open() && std::fclose(file_); }

	bool seek(size_type offset, int whence = SEEK_SET)
	{
		assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);

		return std::fseek(file_, static_cast<long>(offset), whence) == 0;
	}

	// Gets the file size.
	bool size(size_type &file_size)
	{
		long current_position = ftell(file_);
		if (current_position == -1L || !seek(0, SEEK_END))
			return false;

		long pos_end = ftell(file_);
		if (pos_end == -1L || !seek(current_position, SEEK_SET))
			return false;

		file_size = static_cast<size_type>(pos_end);
		return true;
	}

	template <typename T>
	bool read(T *buf, size_type nmemb)
	{ return std::fread(buf, sizeof(T), nmemb, file_) == nmemb; }
	template <typename T>
	bool write(const T *buf, size_type nmemb)
	{ return std::fwrite(buf, sizeof(T), nmemb, file_) == nmemb; }

	bool is_open() const { return file_ != 0; }
};

// Key.
template <typename ValueType>
class DoubleArrayKey : public DoubleArrayCommonTypes
{
public:
	typedef ValueType value_type;
	typedef DoubleArrayKey key_type;

private:
	const char_type *key_;
	base_type length_;
	value_type value_;

public:
	DoubleArrayKey() : key_(0), length_(0), value_() {}
	explicit DoubleArrayKey(const char_type *key)
		: key_(key), length_(0), value_()
	{
		while (key_[length_] != 0)
			++length_;
	}
	DoubleArrayKey(const char_type *key, size_type length)
		: key_(key), length_(static_cast<base_type>(length)), value_()
	{
		if (std::find(key_, key_ + length_, 0) != (key_ + length_))
			THROW("Null character");
	}

	// Compares 2 keys like std::strcmp().
	int compare(const DoubleArrayKey &rhs) const
	{
		base_type i = 0;
		while ((*this)[i] != 0 && (*this)[i] == rhs[i])
			++i;
		return (*this)[i] - rhs[i];
	}

	base_type length() const { return length_; }
	value_type value() const { return value_; }

	void set_value(value_type value) { value_ = value; }

	uchar_type operator[](base_type index) const
	{ return static_cast<uchar_type>((index != length_) ? key_[index] : 0); }

	DoubleArrayKey &operator++() { ++key_, --length_; return *this; }
	DoubleArrayKey &operator--() { --key_, ++length_; return *this; }
};

// Range of keys to be arranged.
template <typename ValueType>
class DoubleArrayKeyRange : DoubleArrayCommonTypes
{
public:
	typedef ValueType value_type;
	typedef DoubleArrayKey<value_type> key_type;
	typedef typename std::vector<key_type>::iterator iterator;

public:
	DoubleArrayKeyRange(iterator begin, iterator end, base_type index = 0)
		: begin_(begin), end_(end), index_(index) {}

	size_type size() const
	{ return static_cast<size_type>(std::distance(begin_, end_)); }

	void set_index(base_type index) { index_ = index; }

	iterator begin() { return begin_; }
	iterator end() { return end_; }
	base_type index() const { return index_; }

private:
	iterator begin_;
	iterator end_;
	base_type index_;
};

// A unit of a double-array.
template <typename ValueType>
class DoubleArrayUnit : public DoubleArrayCommonTypes
{
public:
	typedef ValueType value_type;

	static const base_type OFFSET_MAX = static_cast<base_type>(1) << 22;

private:
	base_type base_;
	value_type value_;

public:
	DoubleArrayUnit() : base_(0), value_() {}

	// Sets values.
	void set_is_end() { base_ |= 1; }
	void set_label(uchar_type label) { set<1, 8>(label); }
	void set_offset(base_type offset)
	{
		if (offset >= (OFFSET_MAX << 8))
			THROW("Too large offset");

		set<9, 23>((offset < OFFSET_MAX)
			? offset : ((1 << 22) + (offset >> 8)));
	}
	void set_value(value_type value) { value_ = value; }

	// Gets values.
	bool is_end() const { return (base_ & 1) == 1; }
	uchar_type label() const { return get<1, 8, uchar_type>(); }
	base_type offset() const
	{
//		return (base_ >> 9) << ((base_ >> 26) & 0x08);
		base_type offset = get<9, 23, base_type>();
		return (offset < OFFSET_MAX) ? offset : ((offset - OFFSET_MAX) << 8);
	}
	value_type value() const { return value_; }

private:
	// Sets a values.
	template <int OffsetBits, int NumOfBits, typename InputType>
	void set(InputType input)
	{
		static const base_type MASK =
			((static_cast<base_type>(1) << (NumOfBits)) - 1) << (OffsetBits);

		base_ &= ~MASK;
		base_ |= static_cast<base_type>(input) << (OffsetBits);
	}

	// Gets a value.
	template <int OffsetBits, int NumOfBits, typename OutputType>
	OutputType get() const
	{
		static const base_type MASK =
			((static_cast<base_type>(1) << (NumOfBits)) - 1) << (OffsetBits);

		return static_cast<OutputType>((base_ & MASK) >> (OffsetBits));
	}
};

// An extra information for building a double-array.
class DoubleArrayExtra : public DoubleArrayCommonTypes
{
private:
	base_type lo_values_;
	base_type hi_values_;

public:
	DoubleArrayExtra() : lo_values_(0), hi_values_(0) {}

	void clear() { lo_values_ = hi_values_ = 0; }

	// Sets values.
	void set_is_fixed() { lo_values_ |= 1; }
	void set_next(base_type next)
	{ lo_values_ = (lo_values_ & 1) | (next << 1); }
	void set_is_used() { hi_values_ |= 1; }
	void set_prev(base_type prev)
	{ hi_values_ = (hi_values_ & 1) | (prev << 1); }

	// Gets values.
	bool is_fixed() const { return (lo_values_ & 1) == 1; }
	base_type next() const { return lo_values_ >> 1; }
	bool is_used() const { return (hi_values_ & 1) == 1; }
	base_type prev() const { return hi_values_ >> 1; }
};

// A class for building a double-array.
template <typename ValueType, typename ProgressFunc>
class DoubleArrayBuilder : public DoubleArrayCommonTypes
{
public:
	typedef ValueType value_type;
	typedef DoubleArrayKey<value_type> key_type;
	typedef DoubleArrayKeyRange<value_type> range_type;
	typedef DoubleArrayUnit<value_type> unit_type;
	typedef DoubleArrayExtra extra_type;

	// Constant values.
	enum
	{
		BLOCK_SIZE = 256,
		NUM_OF_UNFIXED_BLOCKS = 16,
		UNFIXED_SIZE = BLOCK_SIZE * NUM_OF_UNFIXED_BLOCKS,
	};

private:
	std::vector<key_type> &keys_;
	std::vector<unit_type> &units_;
	ProgressFunc progress_func_;

	std::vector<std::vector<extra_type> *> extras_;
	base_type unfixed_index_;
	base_type num_of_unused_units_;

	// Disables copies.
	DoubleArrayBuilder(const DoubleArrayBuilder &);
	DoubleArrayBuilder &operator=(const DoubleArrayBuilder &);

public:
	// Builds a double-array from a set of keys.
	static void build(std::vector<key_type> &keys,
		std::vector<unit_type> &units, ProgressFunc progress_func)
	{
		DoubleArrayBuilder builder(keys, units, progress_func);
		builder.build();

		// Shrinks a double-array if possible.
		try { std::vector<unit_type>(units).swap(units); }
		catch (const std::exception &) {}
	}

private:
	DoubleArrayBuilder(std::vector<key_type> &keys,
		std::vector<unit_type> &units, ProgressFunc progress_func)
		: keys_(keys), units_(units), progress_func_(progress_func),
		extras_(), unfixed_index_(0), num_of_unused_units_(0) {}

	~DoubleArrayBuilder()
	{
		for (size_type i = 0; i < extras_.size(); ++i)
			delete extras_[i];
	}

	void build()
	{
		// 0 is reserved for the root.
		reserve_index(0);

		// To avoid invalid transitions.
		extras(0).set_is_used();
		units(0).set_offset(1);
		units(0).set_label(0);

		if (!keys_.empty())
			build_trie();

		// Fixes remaining blocks.
		fix_all_blocks();
	}

	// Builds a minimal prefix trie.
	void build_trie()
	{
		typedef typename range_type::iterator range_iterator;

		std::stack<range_type> range_stack;
		range_stack.push(range_type(keys_.begin(), keys_.end(), 0));

		size_type end_count = 0;
		std::vector<uchar_type> labels;
		std::vector<range_type> next_ranges;
		while (!range_stack.empty())
		{
			range_type range = range_stack.top();
			range_stack.pop();

			// Marks an end unit.
			range_iterator next_begin = range.begin();
			if ((*range.begin())[0] == 0)
			{
				units(range.index()).set_is_end();
				units(range.index()).set_value(range.begin()->value());

				progress_func_(++end_count, keys_.size());
				if (++next_begin == range.end())
					continue;
			}

			// Lists remaining labels and next ranges.
			labels.push_back((*next_begin)[0]);
			for (range_iterator it = next_begin; it != range.end(); ++it)
			{
				if ((*it)[0] != labels.back())
				{
					labels.push_back((*it)[0]);
					next_ranges.push_back(range_type(next_begin, it));
					next_begin = it;
				}
				++*it;
			}
			next_ranges.push_back(range_type(next_begin, range.end()));

			assert(labels.size() == next_ranges.size());

			// Finds a good offset.
			base_type offset_index = find_offset_index(range.index(), labels);
			base_type offset = range.index() ^ offset_index;
			units(range.index()).set_offset(offset);

			for (size_type i = next_ranges.size(); i > 0; )
			{
				--i;

				// Reserves a child unit.
				base_type child_index = offset_index ^ labels[i];
				reserve_index(child_index);
				units(child_index).set_label(labels[i]);

				// Puts an invalid offset.
				units(child_index).set_offset(labels[i]);

				// Pushes a next range to a stack.
				next_ranges[i].set_index(child_index);
				range_stack.push(next_ranges[i]);
			}
			extras(offset_index).set_is_used();

			labels.clear();
			next_ranges.clear();
		}
	}

	// Finds a good offset.
	base_type find_offset_index(base_type index,
		const std::vector<uchar_type> &labels)
	{
		static const base_type LOWER_MASK = unit_type::OFFSET_MAX - 1;
		static const base_type UPPER_MASK = ~LOWER_MASK;

		uchar_type first_label = labels[0];
		if (unfixed_index_ >= num_of_units())
			return num_of_units() | (index & 0xFF);

		// Scans empty units.
		base_type unfixed_index = unfixed_index_;
		do
		{
			base_type offset_index = unfixed_index ^ first_label;
			base_type offset = index ^ offset_index;

			if (!extras(offset_index).is_used()
				&& (!(offset & LOWER_MASK) || !(offset & UPPER_MASK)))
			{
				// Finds a collision.
				bool has_collision = false;
				for (size_type i = 1; i < labels.size(); ++i)
				{
					if (extras(offset_index ^ labels[i]).is_fixed())
					{
						has_collision = true;
						break;
					}
				}

				if (!has_collision)
					return offset_index;
			}
			unfixed_index = extras(unfixed_index).next();
		} while (unfixed_index != unfixed_index_);

		return num_of_units() | (index & 0xFF);
	}

	// Removes an empty unit from a circular linked list.
	void reserve_index(base_type index)
	{
		if (index >= num_of_units())
			expand_trie();

		assert(!extras(index).is_fixed());

		if (index == unfixed_index_)
		{
			unfixed_index_ = extras(index).next();
			if (unfixed_index_ == index)
				unfixed_index_ = num_of_units();
		}
		extras(extras(index).prev()).set_next(extras(index).next());
		extras(extras(index).next()).set_prev(extras(index).prev());
		extras(index).set_is_fixed();
	}

	// Expands a trie.
	void expand_trie()
	{
		base_type src_num_of_units = num_of_units();
		base_type src_num_of_blocks = num_of_blocks();

		base_type dest_num_of_units = src_num_of_units + BLOCK_SIZE;
		base_type dest_num_of_blocks = src_num_of_blocks + 1;

		// Fixes an old block.
		if (dest_num_of_blocks > NUM_OF_UNFIXED_BLOCKS)
			fix_block(src_num_of_blocks - NUM_OF_UNFIXED_BLOCKS);

		units_.resize(dest_num_of_units);
		extras_.resize(dest_num_of_blocks, 0);

		// Allocates memory to a new block.
		if (dest_num_of_blocks > NUM_OF_UNFIXED_BLOCKS)
		{
			base_type block_id = src_num_of_blocks - NUM_OF_UNFIXED_BLOCKS;
			std::swap(extras_[block_id], extras_.back());
			for (base_type i = src_num_of_units; i < dest_num_of_units; ++i)
				extras(i).clear();
		}
		else
			extras_.back() = new std::vector<extra_type>(BLOCK_SIZE);

		// Creates a circular linked list of empty units.
		for (base_type i = src_num_of_units + 1; i < dest_num_of_units; ++i)
		{
			extras(i - 1).set_next(i);
			extras(i).set_prev(i - 1);
		}

		extras(src_num_of_units).set_prev(dest_num_of_units - 1);
		extras(dest_num_of_units - 1).set_next(src_num_of_units);

		// Merges 2 circular linked lists.
		extras(src_num_of_units).set_prev(extras(unfixed_index_).prev());
		extras(dest_num_of_units - 1).set_next(unfixed_index_);

		extras(extras(unfixed_index_).prev()).set_next(src_num_of_units);
		extras(unfixed_index_).set_prev(dest_num_of_units - 1);
	}

	// Fixes all blocks to avoid invalid transitions.
	void fix_all_blocks()
	{
		base_type begin = 0;
		if (num_of_blocks() > NUM_OF_UNFIXED_BLOCKS)
			begin = num_of_blocks() - NUM_OF_UNFIXED_BLOCKS;
		base_type end = num_of_blocks();

		for (base_type block_id = begin; block_id != end; ++block_id)
			fix_block(block_id);
	}

	// Adjusts units in a given block.
	// Labels of empty units and offsets of end units are changed.
	void fix_block(base_type block_id)
	{
		assert(block_id < num_of_blocks());

		base_type begin = block_id * BLOCK_SIZE;
		base_type end = begin + BLOCK_SIZE;

		// Finds unused offsets.
		base_type unused_offset_for_label = 0;
		base_type unused_offset_for_offset = 0;
		for (base_type offset = begin; offset != end; ++offset)
		{
			if (!extras(offset).is_used())
			{
				if (unused_offset_for_label == 0)
					unused_offset_for_label = offset;
				else
				{
					unused_offset_for_offset = offset;
					break;
				}
			}
		}

		// In a special rare case, this code will fail to find an offset.
		if (unused_offset_for_offset == 0)
		{
			for (base_type offset = end; offset != num_of_units(); ++offset)
			{
				if (!extras(offset).is_used())
				{
					unused_offset_for_offset = offset;
					extras(offset).set_is_used();
					break;
				}
			}
			if (unused_offset_for_offset == 0)
				THROW("no unused offset");
		}

		// Labels of empty units and offsets of end units are changed.
		for (base_type index = begin; index != end; ++index)
		{
			if (!extras(index).is_fixed())
			{
				reserve_index(index);
				units(index).set_label(static_cast<uchar_type>(
					index ^ unused_offset_for_label));
				++num_of_unused_units_;
			}
			else if (units(index).offset() == units(index).label())
				units(index).set_offset(index ^ unused_offset_for_offset);
		}
	}

private:
	unit_type &units(base_type index)
	{
		if (index >= num_of_units())
			THROW("out of range");

		return units_[index];
	}
	extra_type &extras(base_type index)
	{
		if (index >= num_of_units())
			THROW("out of range");
		else if (!extras_[index / BLOCK_SIZE])
			THROW("access to deleted block");

		return (*extras_[index / BLOCK_SIZE])[index % BLOCK_SIZE];
	}

	// Including units for suffixes.
	base_type num_of_units() const
	{ return static_cast<base_type>(units_.size()); }
	// Not including units for suffixes.
	base_type num_of_blocks() const
	{ return static_cast<base_type>(extras_.size()); }
};

// The base class of the DoubleArray.
template <typename ValueType>
class DoubleArrayBase : public DoubleArrayCommonTypes
{
public:
	typedef ValueType value_type;
	typedef DoubleArrayFile file_type;
	typedef DoubleArrayUnit<value_type> unit_type;

	struct result_pair_type
	{
		value_type value;
		size_type length;
	};

	// For compatibility.
	typedef value_type result_type;

private:
	const unit_type *units_;
	size_type size_;
	std::vector<unit_type> units_buf_;

	// Disables copies.
	DoubleArrayBase(const DoubleArrayBase &);
	DoubleArrayBase &operator=(const DoubleArrayBase &);

public:
	DoubleArrayBase() : units_(0), size_(0), units_buf_() {}

	// For compatibility (to handle default arguments).
	int build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths = 0, const value_type *values = 0)
	{
		return build(num_of_keys, keys,
			lengths, values, DefaultProgressFunc());
	}
	// Builds a double-array from a set of keys.
	template <typename ProgressFunc>
	int build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths, const value_type *values,
		ProgressFunc progress_func)
	{
		build_double_array(num_of_keys, keys, lengths, values, progress_func);
		return 0;
	}
	// For compatibility (to ignore an integer zero).
	int build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths, const value_type *values, int null_pointer)
	{
		if (null_pointer)
			THROW("Invalid progress function");

		return build(num_of_keys, keys, lengths, values,
			DefaultProgressFunc());
	}
	// For compatibility (to ignore a null pointer).
	template <typename ProgressFunc>
	int build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths, const value_type *values,
		ProgressFunc *progress_func)
	{
		if (!progress_func)
			return build(num_of_keys, keys, lengths, values);

		build_double_array(
			num_of_keys, keys, lengths, values, progress_func);
		return 0;
	}

	// Returns the number of units.
	size_type size() const { return size_; }
	// Returns the size of each unit of an array.
	size_type unit_size() const { return sizeof(unit_type); }
	// Always returns the number of units (no use).
	size_type nonzero_size() const { return size(); }
	// Returns the array size.
    size_type total_size() const { return size_ * sizeof(unit_type); }

	// Sets the start address of an array.
	void set_array(const void *ptr, size_type size = 0)
	{
		clear();
		units_ = static_cast<const unit_type *>(ptr);
		size_ = size;
	}
	// Returns the start address of an array.
	const void *array() const { return units_; }

	// Frees allocated memory.
	void clear()
	{
		units_ = 0;
		size_ = 0;
		std::vector<unit_type>(0).swap(units_buf_);
	}

	// Loads a double-array from a file.
	int open(const char *file_name, const char *mode = "rb",
		size_type offset = 0, size_type size = 0)
	{
		file_type file(file_name, mode);
		if (!file.is_open() || !file.seek(offset))
			return -1;

		if (!size)
		{
			if (!file.size(size) || size <= offset)
				return -1;
			size -= offset;
		}
		if ((size % sizeof(unit_type)) != 0)
			return -1;
		size_type num_of_units = size / sizeof(unit_type);

		std::vector<unit_type> new_units(num_of_units);
		if (!file.read(&new_units[0], num_of_units))
			return -1;

		set_vector(new_units);
		return 0;
	}
	// Saves a double-array to a file.
	int save(const char *file_name, const char *mode = "wb",
		size_type offset = 0) const
	{
		file_type file(file_name, mode);
		if (!file.is_open() || !file.seek(offset))
			return -1;

		if (!file.write(units_, size_))
			return -1;
		return 0;
	}

public:
	// Searches a double-array for a given key.
	template <typename ResultType>
	void exactMatchSearch(const char_type *key, ResultType &result,
		size_type length = 0, size_type index = 0) const
	{ result = exactMatchSearch<ResultType>(key, length, index); }

	// Searches a double-array for a given key.
	template <typename ResultType>
	ResultType exactMatchSearch(const char_type *key,
		size_type length = 0, size_type node_pos = 0) const
	{
		if (!length)
			length = length_of(key);

		ResultType result;
		set_result(result, -1, 0);

		base_type index = static_cast<base_type>(node_pos);
		size_type key_pos = 0;
		while (key_pos < length)
		{
			index ^= units_[index].offset()
				^ static_cast<uchar_type>(key[key_pos]);
			if (units_[index].label() != static_cast<uchar_type>(key[key_pos]))
				return result;
			++key_pos;
		}

		if (!units_[index].is_end())
			return result;

		set_result(result, units_[index].value(), key_pos);
		return result;
	}

	// Searches a double-array for prefixes of a given key.
	template <typename ResultType>
	size_type commonPrefixSearch(const char_type *key,
		ResultType *results, size_type max_num_of_results,
		size_type length = 0, size_type node_pos = 0) const
	{
		if (!length)
			length = length_of(key);

		size_type num_of_results = 0;

		base_type index = static_cast<base_type>(node_pos);
		for (size_type key_pos = 0; key_pos < length; ++key_pos)
		{
			index ^= units_[index].offset()
				^ static_cast<uchar_type>(key[key_pos]);
			if (units_[index].label() != static_cast<uchar_type>(key[key_pos]))
				break;
			else if (units_[index].is_end())
			{
				if (num_of_results < max_num_of_results)
				{
					set_result(results[num_of_results],
						units_[index].value(), key_pos + 1);
				}
				++num_of_results;
			}
		}

		return num_of_results;
	}

	// Searches a double-array for a given key.
	value_type traverse(const char_type *key, size_type &node_pos,
		size_type &key_pos, size_type length = 0) const
	{
		if (!length)
			length = length_of(key);

		base_type index = static_cast<base_type>(node_pos);
		for ( ; key_pos < length; ++key_pos)
		{
			index ^= units_[index].offset()
				^ static_cast<uchar_type>(key[key_pos]);
			if (units_[index].label() != static_cast<uchar_type>(key[key_pos]))
				return static_cast<value_type>(-2);

			node_pos = static_cast<size_type>(index);
		}

		if (units_[index].is_end())
			return units_[index].value();
		return static_cast<value_type>(-1);
	}

private:
	// Builds a double-array from a set of keys.
	template <typename ProgressFunc>
	void build_double_array(size_type num_of_keys,
		const char_type * const *keys, const size_type *lengths,
		const value_type *values, ProgressFunc progress_func)
	{
		typedef DoubleArrayKey<value_type> key_type;
		typedef DoubleArrayBuilder<value_type, ProgressFunc> builder_type;

		if (num_of_keys > 0 && !keys)
			THROW("Null pointer");

		size_type key_count = 0;
		std::vector<key_type> internal_keys(num_of_keys);
		for (size_type i = 0; i < num_of_keys; ++i)
		{
			// Calculates lengths of keys if not provided.
			internal_keys[key_count] = lengths
				? key_type(keys[i], lengths[i]) : key_type(keys[i]);

			// Keys must be sorted in dictionary order.
			// Also, repeated keys are ignored.
			if (key_count > 0)
			{
				int result = internal_keys[key_count - 1].compare(
					internal_keys[key_count]);
				if (!result)
					continue;
				else if (result > 0)
					THROW("Unsorted keys");
			}

			// Uses indices as values if not provided.
			internal_keys[key_count].set_value(
				values ? values[i] : static_cast<value_type>(i));

			++key_count;
		}
		internal_keys.resize(key_count);

		// Builds a double-array.
		std::vector<unit_type> new_units;
		builder_type::build(internal_keys, new_units, progress_func);

		assert(!new_units.empty());

		// Deletes an old double-array.
		set_vector(new_units);
	}

 	// Sets a new vector.
	void set_vector(std::vector<unit_type> &new_units)
	{
		set_array(&new_units[0], new_units.size());
		units_buf_.swap(new_units);
	}

private:
	// Scans a given key and returns its length.
	static size_type length_of(const char_type *key)
	{
		size_type count = 0;
		while (key[count])
			++count;
		return count;
	}

	// Sets a result.
	template <typename ResultType>
	static void set_result(ResultType &result,
		value_type value, size_type length)
	{
		set_result_value(result, value);
		set_result_length(result, length);
	}

	static void set_result_value(value_type &result, value_type value)
	{ result = value; }
	static void set_result_value(result_pair_type &result, value_type value)
	{ result.value = value; }

	static void set_result_length(value_type &, size_type) {}
	static void set_result_length(result_pair_type &result, size_type length)
	{ result.length = length; }
};

// Suffixes are stored after sorting.
typedef DoubleArrayBase<int> DoubleArray;

}  // namespace Darts

// Undefines internal macros.
#undef THROW
#undef THROW_RELAY
#undef THROW_FINAL

#endif  // DARTS_H_
