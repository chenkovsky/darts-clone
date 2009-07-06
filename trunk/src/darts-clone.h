#ifndef DARTS_H_
#define DARTS_H_

// A clone of the Darts (Double-ARray Trie System)
//
// Copyright (C) 2008-2009 Susumu Yata <syata@acm.org>
// All rights reserved.

#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include <dawgdic/dawg-builder.h>
#include <dawgdic/dictionary-builder.h>

#define DARTS_VERSION "0.32"

namespace Darts {

// Base types of dawgdic.
typedef dawgdic::CharType CharType;
typedef dawgdic::UCharType UCharType;
typedef dawgdic::BaseType BaseType;
typedef dawgdic::SizeType SizeType;
typedef dawgdic::DictionaryUnit UnitType;

// The base class of the DoubleArray.
template <typename A, typename B, typename ValueType, typename D>
class DoubleArrayImpl
{
public:
	typedef CharType char_type;
	typedef UCharType uchar_type;
	typedef BaseType base_type;
	typedef SizeType size_type;
	typedef ValueType value_type;
	typedef UnitType unit_type;

	typedef CharType key_type;
	typedef ValueType result_type;

	struct ResultPairType
	{
		ValueType value;
		SizeType length;
	};

	typedef ResultPairType result_pair_type;

public:
	DoubleArrayImpl() : dic_(), units_buf_() {}

	// Builds a double-array from a set of keys.
	int build(SizeType num_keys, const CharType * const *keys,
		const SizeType *lengths = NULL, const ValueType *values = NULL,
		int (*progress_func)(SizeType, SizeType) = NULL)
	{
		dawgdic::DawgBuilder dawg_builder;
		for (SizeType i = 0; i < num_keys; ++i)
		{
			ValueType value = (values != NULL) ?
				values[i] : static_cast<ValueType>(i);
			if (lengths != NULL)
			{
				if (i > 0 && lengths[i - 1] == lengths[i] &&
					!std::strncmp(keys[i - 1], keys[i], lengths[i]))
					continue;
				if (!dawg_builder.Insert(keys[i], lengths[i], value))
					return -1;
			}
			else
			{
				if (i > 0 && !std::strcmp(keys[i - 1], keys[i]))
					continue;
				if (!dawg_builder.Insert(keys[i], value))
					return -1;
			}

			if (progress_func != NULL)
				progress_func(i + 1, num_keys);
		}
		dawgdic::Dawg dawg;
		dawg_builder.Finish(&dawg);

		if (!dawgdic::DictionaryBuilder::Build(dawg, &dic_))
			return -1;
		return 0;
	}

	// Returns the number of units.
	SizeType size() const { return dic_.size(); }
	// Returns the size of each unit.
	SizeType unit_size() const { return sizeof(dawgdic::DictionaryUnit); }
	// Returns the num of units (for compatibility).
	SizeType nonzero_size() const { return dic_.size(); }
	// Returns the number of bytes allocated to units.
    SizeType total_size() const { return dic_.total_size(); }

	// Sets the start address of an array of units.
	void set_array(const void *address)
	{
		dic_.Map(address, 0);
	}
	void set_array(const void *address, SizeType size)
	{
		dic_.Map(address, size);
	}
	// Returns the start address of an array of units.
	const void *array() const
	{
		return dic_.units();
	}

	// Frees allocated memory.
	void clear()
	{
		dic_.Clear();
		std::vector<UnitType>(0).swap(units_buf_);
	}

	// Loads a double-array from a file.
	int open(const char *file_name, const char *mode = "rb",
		SizeType offset = 0, SizeType size = 0)
	{
		std::ifstream file(file_name, std::ios::binary);
		if (!file || !file.seekg(offset))
			return -1;
		if (size == 0)
		{
			if (!file.seekg(0, std::ios::end))
				return -1;
			size = static_cast<SizeType>(file.tellg()) - offset;
			if (!file.seekg(offset))
				return -1;
		}
		units_buf_.resize(size);
		if (!file.read(reinterpret_cast<char *>(&units_buf_[0]), size))
			return -1;
		set_array(&units_buf_[0], size);
		return 0;
	}
	// Saves a double-array to a file.
	int save(const char *file_name, const char *mode = "wb",
		SizeType offset = 0) const
	{
		std::ofstream file(file_name, (std::strchr(mode, 'a') != NULL
			? (std::ios::binary | std::ios::app) : std::ios::binary));
		if (!file || !file.seekp(offset))
			return -1;
		if (!file.write(reinterpret_cast<const char *>(
			array()), total_size()))
			return -1;
		return 0;
	}

	// Searches a double-array for a given key.
	template <typename ResultType>
	void exactMatchSearch(const CharType *key, ResultType &result,
		SizeType length = 0, SizeType node_pos = 0) const
	{
		set_value(-1, &result);
		BaseType index = static_cast<BaseType>(node_pos);

		if (length != 0)
		{
			if (!dic_.Follow(key, length, &index))
				return;
		}
		else if (!dic_.Follow(key, &index, &length))
			return;

		if (dic_.has_value(index))
		{
			set_value(static_cast<ValueType>(dic_.value(index)), &result);
			set_length(length, &result);
		}
	}

	// Searches a double-array for a given key.
	template <typename ResultType>
	ResultType exactMatchSearch(const CharType *key,
		SizeType length = 0, SizeType node_pos = 0) const
	{
		ResultType result;
		exactMatchSearch(key, result, length, node_pos);
		return result;
	}

	// Searches a double-array for prefixes of a given key.
	template <typename ResultType>
	SizeType commonPrefixSearch(const CharType *key,
		ResultType *results, SizeType max_num_results,
		SizeType length = 0, SizeType node_pos = 0) const
	{
		BaseType index = static_cast<BaseType>(node_pos);
		SizeType num_results = 0;

		for (SizeType i = 0; length ? (i < length) : (key[i] != '\0'); ++i)
		{
			if (!dic_.Follow(key[i], &index))
				break;

			if (dic_.has_value(index))
			{
				if (num_results < max_num_results)
				{
					set_value(static_cast<ValueType>(dic_.value(index)),
						&results[num_results]);
					set_length(i + 1, &results[num_results]);
				}
				++num_results;
			}
		}
		return num_results;
	}

	// Searches a double-array for a given key.
	ValueType traverse(const CharType *key, SizeType &node_pos,
		SizeType &key_pos, SizeType length = 0) const
	{
		BaseType index = static_cast<BaseType>(node_pos);

		while (length ? (key_pos < length) : (key[key_pos] != '\0'))
		{
			if (!dic_.Follow(key[key_pos], &index))
				return static_cast<ValueType>(-2);
			node_pos = static_cast<SizeType>(index);
			++key_pos;
		}
		if (!dic_.has_value(index))
			return static_cast<ValueType>(-1);
		return static_cast<ValueType>(dic_.value(index));
	}

private:
	dawgdic::Dictionary dic_;
	std::vector<UnitType> units_buf_;

	// Disallows copies.
	DoubleArrayImpl(const DoubleArrayImpl &);
	DoubleArrayImpl &operator=(const DoubleArrayImpl &);

	// Modifies a result.
	static void set_value(ValueType value, ResultPairType *result)
	{
		result->value = value;
	}
	static void set_length(SizeType length, ResultPairType *result)
	{
		result->length = length;
	}
	static void set_value(ValueType value, ValueType *result)
	{
		*result = value;
	}
	static void set_length(SizeType, ValueType *) {}
};

// Basic double-array.
typedef DoubleArrayImpl<int, int, int, int> DoubleArray;

}  // namespace Darts

#endif  // DARTS_H_
