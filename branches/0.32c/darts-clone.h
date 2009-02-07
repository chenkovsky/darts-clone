#ifndef DARTS_H_
#define DARTS_H_

// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008 Susumu Yata <syata@acm.org>
// All rights reserved.

#define DARTS_VERSION "0.32"

#ifdef _MSC_VER
#include <stdio.h>
#include <share.h>
#endif

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <stack>
#include <typeinfo>
#include <vector>

namespace Darts
{

using std::size_t;

// The default functions to calculate the length of a key.
template <typename  CharType>
class DefaultLengthFunc
{
public:
	size_t operator()(const CharType *key) const
	{
		assert(key != 0);
		size_t i = 0;
		while (key[i] != static_cast<CharType>(0))
			++i;
		return i;
	}
};

template <>
class DefaultLengthFunc<char>
{
public:
	size_t operator()(const char *key) const { return std::strlen(key); }
};

// The default progress function does nothing.
class DefaultProgressFunc
{
public:
	int operator()(size_t, size_t) const { return 1; }
};

// File I/O.
class DoubleArrayFile
{
public:
	DoubleArrayFile(const char *file, const char *mode) : fp_(0)
	{
		assert(file != 0);
		assert(mode != 0);
#ifdef _MSC_VER
		fp_ = _fsopen(file, mode, _SH_DENYWR);
#else
		fp_ = std::fopen(file, mode);
#endif
	}
	~DoubleArrayFile()
	{
		if (is_open())
			std::fclose(fp_);
	}

	bool seek(long offset, int whence)
	{
		assert(whence == SEEK_SET || whence == SEEK_CUR || whence == SEEK_END);
		return std::fseek(fp_, offset, whence) == 0;
	}
	long size()
	{
		assert(is_open());
		long current_position = ftell(fp_);
		if (current_position == -1L)
			return -1;
		if (!seek(0, SEEK_END))
			return -1;
		long file_size = ftell(fp_);
		if (!seek(current_position, SEEK_SET))
			return -1;
		return file_size;
	}

	template <typename T>
	bool read(T *buf, size_t nmemb)
	{
		assert(is_open());
		return std::fread(buf, sizeof(T), nmemb, fp_) == nmemb;
	}
	template <typename T>
	bool write(const T *buf, size_t nmemb)
	{
		assert(is_open());
		return std::fwrite(buf, sizeof(T), nmemb, fp_) == nmemb;
	}

	bool is_open() const { return fp_ != 0; }

private:
	std::FILE *fp_;
};

// Key information.
template <typename CharType, typename UCharType,
	typename ValueType, typename BaseType>
class DoubleArrayKey
{
public:
	typedef CharType char_type;
	typedef UCharType uchar_type;
	typedef ValueType value_type;

	template <typename LengthFunc>
	static DoubleArrayKey create(const char_type * const *keys,
		const size_t *lengths, const value_type *values,
		size_t index, LengthFunc length_func)
	{
		assert(keys != 0);
		return DoubleArrayKey(static_cast<const uchar_type *>(
			static_cast<const void *>(keys[index])),
			lengths != 0 ? lengths[index] : length_func(keys[index]),
			values != 0 ? values[index] : static_cast<value_type>(index));
	}

	bool equals_to(const DoubleArrayKey &target)
	{
		if (length() != target.length())
			return false;
		for (size_t i = 0; i < length(); ++i)
		{
			if (key(i) != target.key(i))
				return false;
		}
		return true;
	}

	bool is_suffix_of(const DoubleArrayKey &target)
	{
		if (length() > target.length())
			return false;
		for (size_t i = 1, min_length = length() + 1; i < min_length; ++i)
		{
			if (key(length() - i) != target.key(target.length() - i))
				return false;
		}
		return true;
	}

	uchar_type key(size_t index) const
	{
		assert(index <= length_);
		return index < length_ ? key_[index] : static_cast<uchar_type>(0);
	}

	void increment_pointer() { ++key_; --length_; }
	void decrement_pointer() { --key_; ++length_; }

	void set_leaf_index(size_t index) { leaf_index_ = index; }

	size_t length() const { return length_; }
	value_type value() const { return value_; }
	size_t leaf_index() const { return leaf_index_; }

private:
	const uchar_type *key_;
	size_t length_;
	value_type value_;
	size_t leaf_index_;

	DoubleArrayKey(const uchar_type *key, size_t length, value_type value)
		: key_(key), length_(length), value_(value), leaf_index_(0) {}
};

// A comparison function for reversed suffixes.
template <typename KeyType>
class ReverseSuffixComparisonFunc
{
public:
	bool operator()(const KeyType &lhs, const KeyType &rhs) const
	{
		size_t min_length =
			(lhs.length() < rhs.length() ? lhs.length() : rhs.length()) + 1;
		for (size_t i = 1; i < min_length; ++i)
		{
			if (lhs.key(lhs.length() - i) != rhs.key(rhs.length() - i))
				return lhs.key(lhs.length() - i) > rhs.key(rhs.length() - i);
		}
		return lhs.length() > rhs.length();
	}
};

// An element of a double-array.
template <typename CharType, typename UCharType,
	typename ValueType, typename BaseType, size_t RecordIDBits = 5>
class DoubleArrayUnit
{
public:
	typedef UCharType uchar_type;
	typedef BaseType base_type;

private:
	// The least significant bit is used as a bit flag `is_leaf'.
	static const size_t BaseTypeBits = sizeof(base_type) * 8;
	static const size_t UCharTypeBits =	sizeof(uchar_type) * 8;
	static const size_t OffsetBits = BaseTypeBits - UCharTypeBits - 1;
	static const size_t LinkedToBits = BaseTypeBits - RecordIDBits - 1;

	static const size_t LabelShift = 1;
	static const size_t OffsetShift = UCharTypeBits + LabelShift;
	static const size_t RecordIDShift = 1;
	static const size_t LinkedToShift = RecordIDBits + RecordIDShift;

	static const base_type LabelMask =
		((static_cast<base_type>(1) << UCharTypeBits) - 1) << LabelShift;
	static const base_type OffsetMask =
		static_cast<base_type>(-1) << OffsetShift;
	static const base_type RecordIDMask =
		((static_cast<base_type>(1) << RecordIDBits) - 1) << RecordIDShift;
	static const base_type LinkedToMask =
		static_cast<base_type>(-1) << LinkedToShift;

	static const size_t OffsetUpperLimit =
		static_cast<size_t>(-1) > (static_cast<base_type>(1) << OffsetBits)
		? (static_cast<base_type>(1) << OffsetBits) - 1
		: static_cast<size_t>(-1);
	static const size_t LinkedToUpperLimit = static_cast<size_t>(-1)
		> (static_cast<base_type>(1) << LinkedToBits)
		? (static_cast<base_type>(1) << LinkedToBits) - 1
		: static_cast<size_t>(-1);

public:
	static const size_t MaxRecordID =
		(static_cast<size_t>(1) << RecordIDBits) - 1;

	DoubleArrayUnit() : values_(0) {}
	DoubleArrayUnit(base_type values) : values_(values) {}

	void set_is_leaf()
	{
		assert(!is_leaf());
		values_ |= static_cast<base_type>(1);
		assert(is_leaf());
	}
	void set_label(uchar_type label)
	{
		assert(this->label() == static_cast<uchar_type>(0));
		values_ &= ~LabelMask;
		values_ |= (static_cast<base_type>(label) << LabelShift) & LabelMask;
		assert(this->label() == label);
	}
	void set_offset(size_t offset)
	{
		assert(this->offset() == static_cast<size_t>(0));
		if (offset > OffsetUpperLimit)
			throw "Too large offset";
		values_ &= ~OffsetMask;
		values_ |=
			(static_cast<base_type>(offset) << OffsetShift) & OffsetMask;
		assert(this->offset() == offset);
	}
	void set_record_id(size_t record_id)
	{
		values_ &= ~RecordIDMask;
		values_ |= (static_cast<base_type>(record_id) << RecordIDShift)
			& RecordIDMask;
		assert(this->record_id() == record_id);
	}
	void set_linked_to(size_t linked_to)
	{
		if (linked_to > LinkedToUpperLimit)
			throw "Too large key linked to";
		values_ &= ~LinkedToMask;
		values_ |= (static_cast<base_type>(linked_to) << LinkedToShift)
			& LinkedToMask;
		assert(this->linked_to() == linked_to);
	}

	base_type values() const { return values_; }
	bool is_leaf() const
	{
		return (values_ & static_cast<base_type>(1))
			== static_cast<base_type>(1);
	}
	uchar_type label() const
	{
		return static_cast<uchar_type>((values_ & LabelMask) >> LabelShift);
	}
	size_t offset() const
	{
		return static_cast<size_t>((values_ & OffsetMask) >> OffsetShift);
	}
	size_t record_id() const
	{
		return static_cast<size_t>((values_ & RecordIDMask) >> RecordIDShift);
	}
	size_t linked_to() const
	{
		return static_cast<size_t>((values_ & LinkedToMask) >> LinkedToShift);
	}

private:
	// Values are packed into one unsigned integer.
	base_type values_;
};

// An element of a double-array.
template <typename CharType, typename UCharType,
	typename ValueType, typename BaseType>
class DoubleArrayExtra
{
public:
	typedef BaseType base_type;

public:
	DoubleArrayExtra() : lo_values_(0), hi_values_(0) {}

	void set_is_used()
	{
		assert(!is_used());
		lo_values_ |= static_cast<base_type>(1);
		assert(is_used());
	}
	void set_next(size_t next)
	{
		lo_values_ &= static_cast<base_type>(1);
		lo_values_ |= (static_cast<base_type>(next) << 1);
		assert(this->next() == next);
	}
	void set_is_used_offset()
	{
		assert(!is_used_offset());
		hi_values_ |= static_cast<base_type>(1);
		assert(is_used_offset());
	}
	void set_prev(size_t prev)
	{
		hi_values_ &= static_cast<base_type>(1);
		hi_values_ |= (static_cast<base_type>(prev) << 1);
		assert(this->prev() == prev);
	}

	bool is_used() const
	{
		return (lo_values_ & static_cast<base_type>(1))
			== static_cast<base_type>(1);
	}
	size_t next() const
	{
		return static_cast<size_t>(lo_values_ >> 1);
	}
	bool is_used_offset() const
	{
		return (hi_values_ & static_cast<base_type>(1))
			== static_cast<base_type>(1);
	}
	size_t prev() const
	{
		return static_cast<size_t>(hi_values_ >> 1);
	}

private:
	base_type lo_values_;
	base_type hi_values_;
};

template <typename CharType, typename UCharType,
	typename ValueType, typename BaseType>
class DoubleArrayRange
{
private:
	typedef DoubleArrayKey<
		CharType, UCharType, ValueType, BaseType> key_type;

public:
	typedef typename std::vector<key_type>::iterator iterator;

	DoubleArrayRange(size_t index, iterator begin, iterator end)
		: index_(index), begin_(begin), end_(end) {}

	size_t size() const
	{
		assert(begin_ != end_);
		return static_cast<size_t>(end_ - begin_);
	}

	size_t index() const { return index_; }
	iterator begin() { return begin_; }
	iterator end() { return end_; }

private:
	size_t index_;
	iterator begin_;
	iterator end_;
};

template <typename CharType, typename UCharType,
	typename ValueType, typename BaseType, typename ProgressFunc>
class DoubleArrayBuilder
{
private:
	typedef UCharType uchar_type;
	typedef ValueType value_type;

	typedef DoubleArrayUnit<
		CharType, UCharType, ValueType, BaseType> unit_type;
	typedef DoubleArrayExtra<
		CharType, UCharType, ValueType, BaseType> extra_type;
	typedef DoubleArrayKey<
		CharType, UCharType, ValueType, BaseType> key_type;
	typedef DoubleArrayRange<
		CharType, UCharType, ValueType, BaseType> range_type;

	static const size_t BlockSize = 1 << (sizeof(uchar_type) * 8);

public:
	static bool build(std::vector<key_type> &keys,
		std::vector<unit_type> &unit_vector,
		std::vector<uchar_type> &tail_vector, ProgressFunc progress_func)
	{
		DoubleArrayBuilder builder(keys, progress_func);

		if (!builder.build_dictionary())
			return false;

		builder.swap_vectors(unit_vector, tail_vector);
		return true;
	}

private:
	DoubleArrayBuilder(std::vector<key_type> &keys, ProgressFunc progress_func)
		: keys_(keys), unit_vector_(), tail_vector_(),
		progress_func_(progress_func), first_unused_index_(0),
		extra_vector_() {}

	bool build_dictionary()
	{
		try
		{
			build_tree();
			build_tail();

			return true;
		}
		catch (...)
		{
			return false;
		}
	}

	void swap_vectors(std::vector<unit_type> &unit_vector,
		std::vector<uchar_type> &tail_vector)
	{
		assert(!unit_vector_.empty());
		assert(!tail_vector_.empty());

		try
		{
			// Shrinks vectors if possible.
			std::vector<unit_type>(unit_vector_).swap(unit_vector_);
			std::vector<uchar_type>(tail_vector_).swap(tail_vector_);
		}
		catch (...) {}

		unit_vector_.swap(unit_vector);
		tail_vector_.swap(tail_vector);
	}

private:
	void build_tree()
	{
		expand_tree();

		reserve_index(0);
		extra_vector_[0].set_is_used_offset();

		if (keys_.empty())
		{
			unit_vector_[0].set_label(static_cast<uchar_type>(1));
			return;
		}

		build_tree_structure();

		solve_unused_indices();
	}

	void build_tree_structure()
	{
		size_t leaf_count = 0;
		std::stack<range_type> range_stack;
		range_stack.push(range_type(0, keys_.begin(), keys_.end()));

		typedef std::vector<std::pair<uchar_type, size_t> > SiblingList;
		SiblingList siblings;
		while (!range_stack.empty())
		{
			range_type range = range_stack.top();
			range_stack.pop();
			assert(range.index() < unit_vector_.size());

			// Marks a leaf node.
			if (range.index() != 0 && range.size() == 1)
			{
				key_type &key = *range.begin();
				key.decrement_pointer();
				assert(key.length() >= 0);

				key.set_leaf_index(range.index());
				unit_vector_[range.index()].set_is_leaf();

				progress_func_(++leaf_count, keys_.size());
				continue;
			}

			// Groups siblings by their labels.
			siblings.clear();
			for (typename range_type::iterator it = range.begin();
				it != range.end(); ++it)
			{
				if (siblings.empty() || siblings.back().first != it->key(0))
					siblings.push_back(std::make_pair(it->key(0), 1));
				else
					++siblings.back().second;
				it->increment_pointer();
			}

			// Arranges nodes.
			size_t offset = find_offset(siblings.begin(), siblings.end());
			assert(offset < unit_vector_.size() + BlockSize);
			unit_vector_[range.index()].set_offset(offset);
			for (size_t i = 0; i < siblings.size(); ++i)
			{
				size_t child_index = offset ^ siblings[i].first;
				reserve_index(child_index);
				unit_vector_[child_index].set_label(siblings[i].first);
			}
			extra_vector_[offset].set_is_used_offset();

			// Pushes the next ranges into a stack.
			typename range_type::iterator range_it = range.end();
			for (typename SiblingList::reverse_iterator it = siblings.rbegin();
				it != siblings.rend(); ++it)
			{
				size_t index = offset ^ it->first;
				range_stack.push(range_type(
					index, range_it - it->second, range_it));
				range_it -= it->second;
			}
			assert(range_it == range.begin());
		}
	}

	void solve_unused_indices()
	{
		size_t unused_index = first_unused_index_;
		size_t unused_offset = 0;
		size_t prev_block_id = static_cast<size_t>(-1);
		do
		{
			size_t block_id = unused_index >> (sizeof(uchar_type) * 8);
			if (block_id != prev_block_id)
			{
				size_t begin = block_id << (sizeof(uchar_type) * 8);
				size_t end = begin + (1 << (sizeof(uchar_type) * 8));
				for (size_t i = begin; i != end; ++i)
				{
					if (!extra_vector_[i].is_used_offset())
					{
						unused_offset = i;
						break;
					}
				}
				assert(!extra_vector_[unused_offset].is_used_offset());
			}

			unit_vector_[unused_index].set_label(
				static_cast<uchar_type>(unused_index ^ unused_offset));
			unused_index = extra_vector_[unused_index].next();
		} while (unused_index != first_unused_index_);
	}

	template <typename Iterator>
	size_t find_offset(Iterator begin, Iterator end)
	{
		uchar_type label = begin->first;
		if (first_unused_index_ >= extra_vector_.size())
			return first_unused_index_ ^ label;

		assert(!extra_vector_[first_unused_index_].is_used());

		++begin;
		size_t unused_index = first_unused_index_;
		do
		{
			assert(unused_index < extra_vector_.size());
			size_t offset = unused_index ^ label;
			if (!extra_vector_[offset].is_used_offset())
			{
				bool has_collision = false;
				for (Iterator it = begin; it != end; ++it)
				{
					if (extra_vector_[offset ^ it->first].is_used())
					{
						has_collision = true;
						break;
					}
				}
				if (!has_collision)
					return offset;
			}
			unused_index = extra_vector_[unused_index].next();
		} while (unused_index != first_unused_index_);

		return extra_vector_.size() ^ label;
	}

	void reserve_index(size_t index)
	{
		if (index >= extra_vector_.size())
			expand_tree();

		assert(!extra_vector_[index].is_used());
		extra_type &extra = extra_vector_[index];
		if (index == first_unused_index_)
		{
			first_unused_index_ = extra.next();
			if (first_unused_index_ == index)
				first_unused_index_ = extra_vector_.size();
		}
		extra_vector_[extra.prev()].set_next(extra.next());
		extra_vector_[extra.next()].set_prev(extra.prev());
		extra.set_is_used();
	}

	void expand_tree()
	{
		size_t former_size = unit_vector_.size();
		size_t later_size = former_size + BlockSize;
		assert(former_size < later_size);

		unit_vector_.resize(later_size);
		extra_vector_.resize(later_size);
		assert(unit_vector_.size() == later_size);
		assert(extra_vector_.size() == later_size);

		// First, creates a circular linked list in the new block.
		for (size_t i = former_size + 1; i < later_size; ++i)
		{
			extra_vector_[i - 1].set_next(i);
			extra_vector_[i].set_prev(i - 1);
		}
		extra_vector_[former_size].set_prev(later_size - 1);
		extra_vector_[later_size - 1].set_next(former_size);

		extra_vector_[former_size].set_prev(
			extra_vector_[first_unused_index_].prev());
		extra_vector_[later_size - 1].set_next(first_unused_index_);

		extra_vector_[extra_vector_[first_unused_index_].prev()].set_next(
			former_size);
		extra_vector_[first_unused_index_].set_prev(later_size - 1);
	}

private:
	void build_tail()
	{
		if (keys_.empty())
		{
			tail_vector_.push_back(static_cast<uchar_type>(0));
			return;
		}

		std::stable_sort(keys_.begin(), keys_.end(),
			ReverseSuffixComparisonFunc<key_type>());

		const size_t CharsPerValue = sizeof(value_type) / sizeof(uchar_type);

		size_t base_linked_to = 0;
		size_t record_id = unit_type::MaxRecordID + 1;
		for (size_t i = 0; i < keys_.size(); ++i)
		{
			if (record_id > unit_type::MaxRecordID ||
				!keys_[i].is_suffix_of(keys_[i - 1]))
			{
				record_id = 0;
				base_linked_to = tail_vector_.size() + CharsPerValue
					+ (keys_[i].length() & ~(CharsPerValue - 1));
				assert(!(base_linked_to & (CharsPerValue - 1)));

				// Appends a suffix into a tail array.
				tail_vector_.resize(
					base_linked_to, static_cast<uchar_type>(0));
				for (size_t j = 0; j < keys_[i].length(); ++j)
				{
					tail_vector_[base_linked_to - j - 2] =
						keys_[i].key(keys_[i].length() - j - 1);
				}
			}

			assert(record_id <= unit_type::MaxRecordID);
			unit_vector_[keys_[i].leaf_index()].set_record_id(record_id++);
			unit_vector_[keys_[i].leaf_index()].set_linked_to(
				base_linked_to - keys_[i].length() - 1);

			// Appends a record into a tail array.
			const value_type value = keys_[i].value();
			const uchar_type *pointer_to_value =
				static_cast<const uchar_type *>(
					static_cast<const void *>(&value));
			for (size_t j = 0; j < CharsPerValue; ++j)
				tail_vector_.push_back(pointer_to_value[j]);
		}
		assert(!tail_vector_.empty());
	}

private:
	std::vector<key_type> &keys_;
	std::vector<unit_type> unit_vector_;
	std::vector<uchar_type> tail_vector_;
	ProgressFunc progress_func_;

	size_t first_unused_index_;
	std::vector<extra_type> extra_vector_;

	// Disallows copies.
	DoubleArrayBuilder(const DoubleArrayBuilder &);
	DoubleArrayBuilder &operator=(const DoubleArrayBuilder &);
};

template <typename CharType, typename UCharType,
	typename ValueType, typename BaseType>
class DoubleArrayQuery
{
public:
	typedef CharType char_type;
	typedef UCharType uchar_type;

	DoubleArrayQuery(const char_type *key, size_t length)
		: key_(static_cast<const uchar_type *>(
			static_cast<const void *>(key))), length_(length) {}

	uchar_type operator[](size_t index) const
	{
		return index < length_ ? key_[index] : static_cast<uchar_type>(0);
	}
	size_t length() const { return length_; }

private:
	const uchar_type *key_;
	size_t length_;
};

template <typename CharType, typename UCharType,
	typename ValueType, typename BaseType,
	typename LengthFunc = DefaultLengthFunc<CharType> >
class DoubleArrayImpl
{
private:
	// Invalid sizes and signs will cause a compile error.
	static const char CharSizeCheck[sizeof(CharType) == sizeof(UCharType)];
	static const char CharSignCheck[
		static_cast<UCharType>(-1) > static_cast<UCharType>(0)];

	// The size of the ValueType must be (n * the size of the CharType).
	static const char ValueSizeCheck[sizeof(ValueType)
		== sizeof(ValueType) / sizeof(CharType) * sizeof(CharType)];

	// The BaseType must be large enough compared with the CharType.
	// Also, the BaseType must be unsigned.
	static const char BaseSizeCheck[
		sizeof(BaseType) > sizeof(CharType) * 2];
	static const char BaseSignCheck[
		static_cast<BaseType>(-1) > static_cast<BaseType>(0)];

	typedef DoubleArrayUnit<
		CharType, UCharType, ValueType, BaseType> unit_type;
	typedef DoubleArrayQuery<
		CharType, UCharType, ValueType, BaseType> query_type;

public:
	typedef CharType char_type;
	typedef UCharType uchar_type;
	typedef ValueType value_type;
	typedef BaseType base_type;

	// For compatibility.
	typedef CharType key_type;
	typedef ValueType result_type;

	struct result_pair_type
	{
		value_type value;
		size_t length;
	};

	DoubleArrayImpl() : unit_(0), tail_(0),
		unit_vector_(), tail_vector_(), array_vector_() {}

	// The second argument is not used.
	void set_array(const void *ptr, size_t = 0)
	{
		clear();

		const value_type *array = static_cast<const value_type *>(ptr);
		value_type unit_num = *array++;

		unit_ = static_cast<const unit_type *>(
			static_cast<const void *>(array));
		tail_ = static_cast<const uchar_type *>(
			static_cast<const void *>(unit_ + unit_num));
	}
	// For compativility.
	// Please use unit_array() and tail_array() instead.
	const void *array() const
	{
		if (array_vector_.empty())
		{
			array_vector_.resize(size());
			array_vector_[0] = static_cast<base_type>(unit_num());
			memcpy(&array_vector_[1], unit_, unit_total_size());
			memcpy(&array_vector_[1 + unit_num()], tail_, tail_total_size());
		}
		return &array_vector_[0];
	}
	// For compativility.
	// Please use unit_num() and tail_length() instead.
	size_t size() const
	{
		return 1 + unit_num()
			+ (tail_total_size() + sizeof(base_type) - 1) / sizeof(base_type);
	}
	// For compativility.
	// Please unused_unit_num() instead.
	size_t nonzero_size() const
	{
		return size() - unused_unit_num();
	}

	void set_unit_array(const unit_type *unit)
	{
		unit_ = unit;
		std::vector<unit_type>(0).swap(unit_vector_);

		std::vector<base_type>(0).swap(array_vector_);
	}
	void set_tail_array(const uchar_type *tail)
	{
		tail_ = tail;
		std::vector<uchar_type>(0).swap(tail_vector_);

		std::vector<base_type>(0).swap(array_vector_);
	}

	const unit_type *unit_array() const { return unit_; }
	const uchar_type *tail_array() const { return tail_; }

	void clear()
	{
		unit_ = 0;
		std::vector<unit_type>(0).swap(unit_vector_);

		tail_ = 0;
		std::vector<uchar_type>(0).swap(tail_vector_);

		std::vector<base_type>(0).swap(array_vector_);
	}

	size_t unit_size() const { return sizeof(unit_type); }
	size_t unit_num() const { return unit_vector_.size(); }
	size_t unit_total_size() const { return unit_size() * unit_num(); }
	size_t unused_unit_num() const
	{
		size_t unused_unit_count = 0;
		for (size_t i = 0; i < unit_num(); ++i)
		{
			if (!unit_[i].offset())
				++unused_unit_count;
		}
		return unused_unit_count;
	}

	size_t tail_size() const { return sizeof(uchar_type); }
	size_t tail_length() const { return tail_vector_.size(); }
	size_t tail_total_size() const { return tail_size() * tail_length(); }

	size_t total_size() const { return unit_total_size() + tail_total_size(); }

	int build(size_t num_of_keys, const char_type * const *keys,
		const size_t *lengths = 0, const value_type *values = 0)
	{
		return build(num_of_keys, keys,
			lengths, values, DefaultProgressFunc());
	}

	template <typename ProgressFunc>
	int build(size_t num_of_keys, const char_type * const *keys,
		const size_t *lengths, const value_type *values,
		ProgressFunc progress_func)
	{
		return build_da(num_of_keys, keys, lengths, values, progress_func);
	}

	// For compatibility (to skip an integer zero).
	int build(size_t num_of_keys, const char_type * const *keys,
		const size_t *lengths, const value_type *values, int null_pointer)
	{
		assert(!null_pointer);
		return build(num_of_keys, keys, lengths, values,
			DefaultProgressFunc());
	}

	// For compatibility (to skip a null pointer).
	template <typename ProgressFunc>
	int build(size_t num_of_keys, const char_type * const *keys,
		const size_t *lengths, const value_type *values,
		ProgressFunc *progress_func)
	{
		if (!progress_func)
			return build(num_of_keys, keys, lengths, values);
		return build_da(num_of_keys, keys, lengths, values, progress_func);
	}

	int open(const char *file, const char *mode = "rb", size_t offset = 0,
		size_t size = 0)
	{
		DoubleArrayFile fp(file, mode);
		if (!fp.is_open())
			return -1;
		if (!fp.seek(static_cast<long>(offset), SEEK_SET))
			return -1;

		if (!size)
		{
			long file_size = fp.size();
			if (file_size < 0)
				return -1;
			size = static_cast<size_t>(file_size);
		}

		base_type unit_num = 0;
		if (!fp.read(&unit_num, 1))
			return -1;
		size -= sizeof(base_type);
		if (size < unit_num * unit_size())
			return -1;
		size_t tail_length =
			size - static_cast<size_t>(unit_num) * unit_size();

		std::vector<unit_type> temp_unit_vector(
			static_cast<size_t>(unit_num));
		std::vector<uchar_type> temp_tail_vector(tail_length);

		if (!fp.read(&temp_unit_vector[0], static_cast<size_t>(unit_num)) ||
			!fp.read(&temp_tail_vector[0], tail_length))
			return -1;

		clear();
		temp_unit_vector.swap(unit_vector_);
		temp_tail_vector.swap(tail_vector_);
		unit_ = &unit_vector_[0];
		tail_ = &tail_vector_[0];

		return 0;
	}

	int save(const char *file, const char *mode = "wb",
		size_t offset = 0) const
	{
		if (!total_size())
			return -1;

		DoubleArrayFile fp(file, mode);
		if (!fp.is_open())
			return -1;
		if (!fp.seek(static_cast<long>(offset), SEEK_SET))
			return -1;

		base_type unit_num_copy = static_cast<base_type>(unit_num());
		if (!fp.write(&unit_num_copy, 1) ||
			!fp.write(unit_, unit_num()) || !fp.write(tail_, tail_length()))
			return -1;

		return 0;
	}

	template <typename ResultType>
	int exactMatchSearch(const char_type *key, ResultType &result,
		size_t length = 0, base_type da_index = 0) const
	{
		if (!length)
			length = LengthFunc()(key);
		return exactMatchSearch(query_type(key, length), result, da_index);
	}
	template <typename ResultType>
	ResultType exactMatchSearch(const char_type *key,
		size_t length = 0, base_type da_index = 0) const
	{
		ResultType result;
		exactMatchSearch(key, result, length, da_index);
		return result;
	}

	template <typename ResultType>
	size_t commonPrefixSearch(const char_type *key,
		ResultType *results, size_t max_num_of_results,
		size_t length = 0, base_type da_index = 0) const
	{
		if (!length)
			length = LengthFunc()(key);
		return commonPrefixSearch(query_type(key, length),
			results, max_num_of_results, da_index);
	}

	// For compatibility (size_t).
	template <typename IndexType>
	value_type traverse(const char_type *key, IndexType &da_index,
		size_t &key_index, size_t length = 0) const
	{
		assert(typeid(IndexType) == typeid(size_t));
		base_type base_type_index = static_cast<base_type>(da_index);
		value_type value =
			traverse(key, base_type_index, key_index, length);
		da_index = static_cast<IndexType>(base_type_index);
		return value;
	}

	value_type traverse(const char_type *key, base_type &da_index,
		size_t &key_index, size_t length = 0) const
	{
		if (!length)
			length = LengthFunc()(key);
		unit_type unit(!da_index ? unit_[0] : da_index);
		value_type value = traverse(query_type(key, length), unit, key_index);
		da_index = unit.values();
		return value;
	}

private:
	template <typename ProgressFunc>
	int build_da(size_t num_of_keys, const char_type * const *keys,
		const size_t *lengths, const value_type *values,
		ProgressFunc progress_func)
	{
		typedef DoubleArrayKey<
			CharType, UCharType, ValueType, BaseType> KeyType;
		std::vector<KeyType> keys_list;
		keys_list.reserve(num_of_keys);
		for (size_t i = 0; i < num_of_keys; ++i)
		{
			KeyType key(
				KeyType::create(keys, lengths, values, i, LengthFunc()));
			if (keys_list.empty() || !key.equals_to(keys_list.back()))
				keys_list.push_back(key);
		}

		std::vector<unit_type> temp_unit_vector;
		std::vector<uchar_type> temp_tail_vector;

		typedef DoubleArrayBuilder<CharType, UCharType,
			ValueType, BaseType, ProgressFunc> BuilderType;
		if (BuilderType::build(
			keys_list, temp_unit_vector, temp_tail_vector, progress_func))
		{
			clear();
			temp_unit_vector.swap(unit_vector_);
			temp_tail_vector.swap(tail_vector_);
			unit_ = &unit_vector_[0];
			tail_ = &tail_vector_[0];
			return 0;
		}

		return -1;
	}

	static void set_result(value_type *result, value_type value, size_t)
	{
		*result = value;
	}
	static void set_result(result_pair_type *result_pair,
		value_type value, size_t length)
	{
		result_pair->value = value;
		result_pair->length = length;
	}

	template <typename ResultType>
	int exactMatchSearch(const query_type &query,
		ResultType &result, base_type da_index) const
	{
		size_t key_index = 0;
		unit_type unit(!da_index ? unit_[0] : da_index);

		set_result(&result, static_cast<value_type>(-1), 0);

		if (!unit.is_leaf())
		{
			for ( ; ; ++key_index)
			{
				unit = unit_[unit.offset() ^ query[key_index]];
				if (unit.is_leaf())
					break;
				else if (unit.label() != query[key_index])
					return -1;
			}
		}

		const uchar_type *tail = &tail_[unit.linked_to()] - key_index;
		for ( ; tail[key_index] == query[key_index]; ++key_index)
		{
			if (tail[key_index] == static_cast<uchar_type>(0))
			{
				const value_type *records = static_cast<const value_type *>(
					static_cast<const void *>(tail + key_index + 1));
				set_result(&result, records[unit.record_id()], key_index);
				return 0;
			}
		}
		return -1;
	}

	template <typename ResultType>
	size_t commonPrefixSearch(const query_type &query, ResultType *results,
		size_t max_num_of_results, base_type da_index) const
	{
		size_t key_index = 0;
		size_t num_of_results = 0;
		unit_type unit(!da_index ? unit_[0] : da_index);

		for ( ; ; ++key_index)
		{
			if (query[key_index] != static_cast<uchar_type>(0))
				take_prefix_key(results, num_of_results,
					max_num_of_results, unit, key_index);

			unit = unit_[unit.offset() ^ query[key_index]];
			if (unit.is_leaf())
				break;
			else if (unit.label() != query[key_index])
				return num_of_results;
		}

		const uchar_type *tail = &tail_[unit.linked_to()] - key_index;
		if (tail[key_index] != query[key_index])
			return num_of_results;

		for ( ; ; ++key_index)
		{
			if (tail[key_index] == static_cast<uchar_type>(0))
			{
				if (num_of_results < max_num_of_results)
				{
					const value_type *records =
						static_cast<const value_type *>(
							static_cast<const void *>(tail + key_index + 1));
					set_result(&results[num_of_results],
						records[unit.record_id()], key_index);
				}
				++num_of_results;
				break;
			}
			else if (tail[key_index] != query[key_index])
				break;
		}
		return num_of_results;
	}

	template <typename ResultType>
	void take_prefix_key(ResultType *results, size_t &num_of_results,
		size_t max_num_of_results, unit_type unit, size_t length) const
	{
		unit = unit_[unit.offset()];
		if (!unit.is_leaf())
			return;

		const uchar_type *tail = &tail_[unit.linked_to()];
		if (*tail != static_cast<uchar_type>(0))
			return;

		if (num_of_results < max_num_of_results)
		{
			const value_type *records = static_cast<const value_type *>(
				static_cast<const void *>(tail + 1));
			set_result(&results[num_of_results],
				records[unit.record_id()], length);
		}
		++num_of_results;
	}

	value_type traverse(const query_type &query,
		unit_type &unit, size_t &key_index) const
	{
		if (!unit.is_leaf())
		{
			for ( ; query[key_index] != static_cast<char_type>(0); ++key_index)
			{
				unit_type next_unit = unit_[unit.offset() ^ query[key_index]];
				if (next_unit.is_leaf())
				{
					unit = next_unit;
					break;
				}
				else if (query[key_index] == static_cast<char_type>(0))
					return static_cast<value_type>(-1);
				else if (next_unit.label() != query[key_index])
					return static_cast<value_type>(-2);
				unit = next_unit;
			}

			if (query[key_index] == static_cast<char_type>(0))
			{
				unit_type leaf = unit_[unit.offset()];
				if (!leaf.is_leaf())
					return static_cast<value_type>(-1);

				const uchar_type *tail = &tail_[leaf.linked_to()];
				if (*tail != static_cast<uchar_type>(0))
					return static_cast<value_type>(-1);

				const value_type *records = static_cast<const value_type *>(
					static_cast<const void *>(tail + 1));
				return records[leaf.record_id()];
			}
		}

		const uchar_type *tail = &tail_[unit.linked_to()] - key_index;
		for ( ; tail[key_index] == query[key_index]; ++key_index)
		{
			if (tail[key_index] == static_cast<uchar_type>(0))
			{
				const value_type *records = static_cast<const value_type *>(
					static_cast<const void *>(tail + key_index + 1));
				unit.set_linked_to(static_cast<size_t>(
					tail + key_index - tail_));
				return records[unit.record_id()];
			}
		}
		unit.set_linked_to(static_cast<size_t>(tail + key_index - tail_));
		if (query[key_index] == static_cast<uchar_type>(0))
			return static_cast<value_type>(-1);
		return static_cast<value_type>(-2);
	}

private:
	const unit_type *unit_;
	const uchar_type *tail_;
	std::vector<unit_type> unit_vector_;
	std::vector<uchar_type> tail_vector_;
	// For compatibility.
	mutable std::vector<base_type> array_vector_;

	// Disallows copies.
	DoubleArrayImpl(const DoubleArrayImpl &);
	DoubleArrayImpl &operator=(const DoubleArrayImpl &);
};

typedef DoubleArrayImpl<char, unsigned char,
	int, unsigned int> DoubleArray;
typedef DoubleArrayImpl<char, unsigned char,
	int, unsigned long long> HugeDoubleArray;
typedef DoubleArrayImpl<short, unsigned short,
	int, unsigned long long> UnicodeDoubleArray;

}  // namespace Darts

#endif  // DARTS_H_
