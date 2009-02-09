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
#endif  // _MSC_VER

#include <algorithm>
#include <cstdio>
#include <exception>
#include <stack>
#include <vector>

// Defines macros for debugging.
#ifdef _DEBUG
#include <iostream>
#define LOG (std::cerr << "DEBUG:" << __LINE__ << ": ")
#define LOG_VALUE(value) (LOG << # value << " = " << value << '\n')
#else  // _DEBUG
#define LOG
#define LOG_VALUE(value)
#endif  // _DEBUG

// Defines macros for throwing exceptions with line numbers.
#define THROW(msg) THROW_RELAY(__LINE__, msg)
#define THROW_RELAY(line, msg) THROW_FINAL(line, msg)
#define THROW_FINAL(line, msg) throw \
	DoubleArrayException("darts-clone-" DARTS_CLONE_VERSION \
		" [" # line "]: " msg)

namespace Darts
{

// Basic types are defined here.
class DoubleArrayBasicTypes
{
public:
	typedef char char_type;
	typedef unsigned char uchar_type;

	// Must be a 32-bit unsigned integer type.
	typedef unsigned int base_type;
	typedef std::size_t size_type;

	// Must be a 32-bit signed integer type.
	typedef int value_type;

	// For compatibility.
	typedef char_type key_type;
	typedef value_type result_type;
};

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

// File I/O.
class DoubleArrayFile
{
public:
	typedef DoubleArrayBasicTypes::size_type size_type;

private:
	std::FILE *file_;

	// Copies are not allowed.
	DoubleArrayFile(const DoubleArrayFile &);
	DoubleArrayFile &operator=(const DoubleArrayFile &);

public:
	DoubleArrayFile(const char *file_name, const char *mode) : file_(0)
	{
		if (!file_name)
			THROW("null file name");
		else if (!mode)
			THROW("null file mode");

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
		if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
			THROW("invalid whence");

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

// An edge of a trie for building a double-array.
class DoubleArrayEdge
{
public:
	typedef DoubleArrayBasicTypes::base_type base_type;

private:
	base_type base_;

public:
	explicit DoubleArrayEdge(bool is_child) : base_(is_child ? 1 : 0) {}

	void set_is_child(bool is_child)
	{ base_ = is_child ? (base_ | 1) : (base_ & ~static_cast<base_type>(1)); }
	void set_index(base_type index) { base_ = (index << 1) | (base_ & 1); }

	bool is_child() const { return (base_ & 1) != 0; }
	bool is_sibling() const { return !is_child(); }
	base_type index() const { return base_ >> 1; }

	bool operator==(const DoubleArrayEdge &edge) const
	{ return base_ == edge.base_; }
};

// A node of a trie for building a double-array.
class DoubleArrayNode
{
public:
	typedef DoubleArrayBasicTypes::uchar_type uchar_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::value_type value_type;
	typedef DoubleArrayEdge edge_type;

private:
	edge_type to_child_;
	edge_type to_sibling_;
	edge_type from_;

	base_type value_;
	base_type index_;
	uchar_type label_;

public:
	DoubleArrayNode() : to_child_(true), to_sibling_(false), from_(true),
		value_(0), index_(0), label_(0) {}

	edge_type &mutable_to_child() { return to_child_; }
	edge_type &mutable_to_sibling() { return to_sibling_; }
	edge_type &mutable_from() { return from_; }

	const edge_type &to_child() const { return to_child_; }
	const edge_type &to_sibling() const { return to_sibling_; }
	const edge_type &from() const { return from_; }

	void set_value(value_type value)
	{ value_ = (static_cast<base_type>(value) << 1) | 1; }
	void set_offset(base_type offset) { index_ = offset; }
	void set_depth(base_type depth) { index_ = depth; }
	void set_label(uchar_type label) { label_ = label; }

	bool has_value() const { return (value_ & 1) != 0; }
	value_type value() const { return static_cast<value_type>(value_ >> 1); }
	base_type offset() const { return index_; }
	base_type depth() const { return index_; }
	uchar_type label() const { return label_; }
};

// A pool buffer to store many nodes.
class DoubleArrayNodePool
{
public:
	typedef DoubleArrayBasicTypes::size_type size_type;
	typedef DoubleArrayNode node_type;

	// Number of nodes in each block.
	enum { BLOCK_SIZE = 1024 };

private:
	std::vector<node_type *> node_blocks_;
	size_type num_of_nodes_;

	// Copies are not allowed.
	DoubleArrayNodePool(const DoubleArrayNodePool &);
	DoubleArrayNodePool &operator=(const DoubleArrayNodePool &);

public:
	DoubleArrayNodePool() : node_blocks_(), num_of_nodes_(0) {}
	~DoubleArrayNodePool()
	{
		for (size_type i = 0; i < node_blocks_.size(); ++i)
			delete [] node_blocks_[i];
	}

	// Gets a pointer to a new node.
	node_type *add_node()
	{
		if (num_of_nodes_ == BLOCK_SIZE * node_blocks_.size())
			node_blocks_.push_back(new node_type[BLOCK_SIZE]);

		return (*this)[num_of_nodes_++];
	}

	// Returns the number of added nodes.
	size_type num_of_nodes() const { return num_of_nodes_; }

	// Gets a pointer to an existing node.
	node_type *operator[](size_type index)
	{
		if (index > num_of_nodes_)
			THROW("too large node ID");

		return &node_blocks_[index / BLOCK_SIZE][index % BLOCK_SIZE];
	}
	const node_type *operator[](size_type index) const
	{
		if (index > num_of_nodes_)
			THROW("too large node ID");

		return &node_blocks_[index / BLOCK_SIZE][index % BLOCK_SIZE];
	}
};

// A unit of a double-array.
class DoubleArrayUnit
{
public:
	typedef DoubleArrayBasicTypes::uchar_type uchar_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::value_type value_type;
	static const base_type OFFSET_MAX = static_cast<base_type>(1) << 22;

private:
	base_type base_;

public:
	DoubleArrayUnit() : base_(0) {}

	// Updates members.
	void set_value(value_type value)
	{ base_ = (static_cast<base_type>(value) << 1) | 1; }
	void set_label(uchar_type label)
	{
		base_ &= ~static_cast<base_type>(0x1FE);
		base_ |= label << 1;
	}
	void set_offset(base_type offset)
	{
		if (offset >= (OFFSET_MAX << 8))
			THROW("too large offset");

		base_ &= 0x1FF;
		if (offset < OFFSET_MAX)
			base_ |= offset << 10;
		else if ((offset & 0xFF) != 0)
			THROW("invalid offset");
		else
			base_ |= (offset << 2) | 0x200;
	}

	// Reads members.
	bool is_leaf() const { return (base_ & 1) != 0; }
	value_type value() const { return static_cast<value_type>(base_ >> 1); }
	uchar_type label() const
	{ return static_cast<uchar_type>((base_ >> 1) & 0xFF); }
	base_type offset() const
	{
		if (base_ & 0x200)
			return (base_ >> 10) << 8;
		return base_ >> 10;
	}
};

// An extra information for building a double-array.
class DoubleArrayExtra
{
public:
	typedef DoubleArrayBasicTypes::base_type base_type;

private:
	base_type lo_values_;
	base_type hi_values_;

public:
	DoubleArrayExtra() : lo_values_(0), hi_values_(0) {}

	void clear() { lo_values_ = hi_values_ = 0; }

	// Updates members.
	void set_is_fixed() { lo_values_ |= 1; }
	void set_next(base_type next)
	{ lo_values_ = (lo_values_ & 1) | (next << 1); }
	void set_is_used() { hi_values_ |= 1; }
	void set_prev(base_type prev)
	{ hi_values_ = (hi_values_ & 1) | (prev << 1); }

	// Reads members.
	bool is_fixed() const { return (lo_values_ & 1) == 1; }
	base_type next() const { return lo_values_ >> 1; }
	bool is_used() const { return (hi_values_ & 1) == 1; }
	base_type prev() const { return hi_values_ >> 1; }
};

// A class for building a double-array.
class DoubleArrayBuilder
{
public:
	typedef DoubleArrayBasicTypes::char_type char_type;
	typedef DoubleArrayBasicTypes::uchar_type uchar_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::size_type size_type;
	typedef DoubleArrayBasicTypes::value_type value_type;
	typedef DoubleArrayEdge edge_type;
	typedef DoubleArrayNode node_type;
	typedef DoubleArrayNodePool node_pool_type;
	typedef DoubleArrayUnit unit_type;
	typedef DoubleArrayExtra extra_type;

	enum
	{
		BLOCK_SIZE = 256,
		NUM_OF_UNFIXED_BLOCKS = 16,
		UNFIXED_SIZE = BLOCK_SIZE * NUM_OF_UNFIXED_BLOCKS,
	};

private:
	// For sorting nodes.
	class NodeValueComparer
	{
	public:
		explicit NodeValueComparer(const node_pool_type &node_pool)
			: node_pool_(node_pool) {}

		bool operator()(base_type lhs_index, base_type rhs_index) const
		{
			const node_type *lhs = node_pool_[lhs_index];
			const node_type *rhs = node_pool_[rhs_index];

			if (lhs->depth() != rhs->depth())
				return lhs->depth() < rhs->depth();
			else if (lhs->label() != rhs->label())
				return lhs->label() < rhs->label();
			else if (lhs->has_value() != rhs->has_value())
				return rhs->has_value();
			return lhs->value() < rhs->value();
		}

	private:
		const node_pool_type &node_pool_;

		// Assignment is not allowed.
		NodeValueComparer &operator=(const NodeValueComparer &);
	};

	// For sorting nodes.
	class NodeReferenceComparer
	{
	public:
		explicit NodeReferenceComparer(const node_pool_type &node_pool)
			: node_pool_(node_pool) {}

		bool operator()(base_type lhs_index, base_type rhs_index) const
		{
			const node_type *lhs = node_pool_[lhs_index];
			const node_type *rhs = node_pool_[rhs_index];

			if (lhs->to_child().index() != rhs->to_child().index())
				return lhs->to_child().index() < rhs->to_child().index();
			return lhs->to_sibling().index() < rhs->to_sibling().index();
		}

	private:
		const node_pool_type &node_pool_;

		// Assignment is not allowed.
		NodeReferenceComparer &operator=(const NodeReferenceComparer &);
	};

	static const base_type LOWER_MASK = unit_type::OFFSET_MAX - 1;
	static const base_type UPPER_MASK = ~LOWER_MASK;

private:
	size_type num_of_keys_;
	const char_type * const *keys_;
	const size_type *lengths_;
	const value_type *values_;

	int (*progress_func_)(size_type, size_type);
	size_type progress_;
	size_type max_progress_;

	node_pool_type node_pool_;
	size_type num_of_merged_nodes_;

	std::vector<unit_type> units_;
	std::vector<std::vector<extra_type> *> extras_;
	std::vector<uchar_type> labels_;
	base_type unfixed_index_;
	size_type num_of_unused_units_;

	// Copies are not allowed.
	DoubleArrayBuilder(const DoubleArrayBuilder &);
	DoubleArrayBuilder &operator=(const DoubleArrayBuilder &);

public:
	DoubleArrayBuilder() : num_of_keys_(0), keys_(0), lengths_(0), values_(0),
		progress_func_(0), progress_(0), max_progress_(0),
		node_pool_(), num_of_merged_nodes_(0), units_(), extras_(),
		labels_(), unfixed_index_(0), num_of_unused_units_(0) {}
	~DoubleArrayBuilder()
	{
		for (size_type i = 0; i < extras_.size(); ++i)
			delete extras_[i];
	}

	// Copies parameters and builds a double-array.
	bool build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths, const value_type *values,
		int (*progress_func)(size_type, size_type))
	{
		// Copies parameters.
		num_of_keys_ = num_of_keys;
		keys_ = keys;
		lengths_ = lengths;
		values_ = values;

		progress_func_ = progress_func;
		progress_ = 0;
		max_progress_ = num_of_keys_ * 2;

		LOG_VALUE(num_of_keys_);

#ifdef _DEBUG
		test_keys();
#endif

		// Builds a naive trie.
		build_trie();
		LOG_VALUE(node_pool_.num_of_nodes());
		LOG_VALUE(node_pool_.num_of_nodes() * sizeof(node_type));

#ifdef _DEBUG
		test_naive_trie();
#endif

		// Compresses a trie.
		compress_trie();
		LOG_VALUE(node_pool_[0]->depth());
		LOG_VALUE(num_of_merged_nodes_);
		LOG_VALUE(node_pool_.num_of_nodes() - num_of_merged_nodes_);

#ifdef _DEBUG
		test_compressed_trie();
#endif

		// Builds a double-array.
		build_double_array();
		LOG_VALUE(num_of_unused_units_);
		LOG_VALUE(num_of_units());

		return true;
	}

	// Returns a reference to a built double-array.
	std::vector<unit_type> &units_buf() { return units_; }

private:
	// Builds a trie.
	void build_trie()
	{
		node_pool_.add_node();

		// Keys are inserted in reversed order.
		size_type key_id = num_of_keys_;
		while (key_id--)
		{
			const char_type *key = keys_[key_id];
			size_type length = lengths_ ? lengths_[key_id] : length_of(key);
			value_type value = values_ ? values_[key_id]
				: static_cast<value_type>(key_id);

			// Node 0 is reserved for the root.
			insert_key(0, key, length, value);

			progress();
		}
	}

	// Inserts a key recursively.
	void insert_key(base_type parent_index, const char_type *key,
		size_type length, value_type value)
	{
		node_type *parent = node_pool_[parent_index];

		uchar_type label = length ? static_cast<uchar_type>(*key) : 0;
		base_type child_index = parent->to_child().index();
		node_type *child = child_index ? node_pool_[child_index] : 0;
		if (!child || child->label() != label)
		{
			// Adds a new node.
			base_type new_node_index =
				static_cast<base_type>(node_pool_.num_of_nodes());
			node_type *new_node = node_pool_.add_node();
			new_node->set_label(label);
			new_node->mutable_to_sibling().set_index(child_index);
			new_node->mutable_from().set_index(parent_index);

			// Updates links.
			parent->mutable_to_child().set_index(new_node_index);
			if (child)
			{
				child->mutable_from().set_is_child(false);
				child->mutable_from().set_index(new_node_index);
			}

			child_index = new_node_index;
			child = new_node;
		}

		if (length)
			insert_key(child_index, key + 1, length - 1, value);
		else
			child->set_value(value);
	}

private:
	// Compresses a trie.
	void compress_trie()
	{
		// Nothing to do.
		if (!values_)
			return;

		calculate_depth(0);

		// Creates a list of indices.
		std::vector<base_type> nodes;
		nodes.resize(node_pool_.num_of_nodes());
		for (size_type i = 0; i < nodes.size(); ++i)
			nodes[i] = static_cast<base_type>(i);

		NodeValueComparer comparer(node_pool_);
		std::sort(nodes.begin(), nodes.end(), comparer);

		// Merges common subtrees.
		merge_common_subtrees(nodes);

		// Clears shared areas (depths and offsets).
		for (size_type i = 0; i < node_pool_.num_of_nodes(); ++i)
			node_pool_[i]->set_offset(0);
	}

	// Calculates depths recursively.
	base_type calculate_depth(base_type parent_index)
	{
		node_type *parent = node_pool_[parent_index];

		base_type child_depth = !parent->to_child().index() ? 0
			: calculate_depth(parent->to_child().index());
		base_type sibling_depth = !parent->to_sibling().index() ? 0
			: calculate_depth(parent->to_sibling().index());

		parent->set_depth(std::max(child_depth, sibling_depth));
		return parent->depth() + 1;
	}

	// Merges common subtrees.
	void merge_common_subtrees(std::vector<base_type> &nodes)
	{
		for (base_type node_id = 0;
			node_id < static_cast<base_type>(nodes.size()); )
		{
			base_type begin = node_id;
			while (++node_id < static_cast<base_type>(nodes.size()))
			{
				const node_type *node_prev = node_pool_[nodes[node_id - 1]];
				const node_type *node_current = node_pool_[nodes[node_id]];

				// Finds nodes which have the same values.
				if (node_prev->depth() != node_current->depth()
					|| node_prev->has_value() != node_current->has_value()
					|| node_prev->value() != node_current->value()
					|| node_prev->label() != node_current->label())
					break;
			}
			merge_common_subtrees(nodes, begin, node_id);
		}
	}

	// Merges common subtrees.
	void merge_common_subtrees(std::vector<base_type> &nodes,
		base_type begin, base_type end)
	{
		if (begin + 1 == end)
			return;

		NodeReferenceComparer comparer(node_pool_);
		std::sort(&nodes[begin], &nodes[end], comparer);

		base_type base_node_id = begin;
		node_type *base_node = node_pool_[nodes[base_node_id]];
		for (base_type node_id = begin + 1; node_id != end; ++node_id)
		{
			node_type *node = node_pool_[nodes[node_id]];

			if (node->to_child() == base_node->to_child()
				&& node->to_sibling() == base_node->to_sibling())
			{
				// Merges nodes which have the same references.
				node_type *from = node_pool_[node->from().index()];
				if (node->from().is_child())
					from->mutable_to_child().set_index(nodes[base_node_id]);
				else
					from->mutable_to_sibling().set_index(nodes[base_node_id]);
				++num_of_merged_nodes_;
			}
			else
			{
				base_node_id = node_id;
				base_node = node;
			}
		}
	}

private:
	// Builds a double-array.
	void build_double_array()
	{
		// 0 is reserved for the root.
		reserve_unit(0);

		// To avoid invalid transitions.
		extras(0).set_is_used();
		units(0).set_offset(1);
		units(0).set_label(0);

		progress_ = node_pool_.num_of_nodes() - num_of_merged_nodes_;
		max_progress_ = progress_ * 2;
		build_double_array(node_pool_[0], 0);

		// Fixes remaining blocks.
		fix_all_blocks();
	}

	// Builds a double-array.
	void build_double_array(node_type *parent, base_type parent_index)
	{
		progress();
		if (!parent->to_child().index())
			return;

		base_type child_index = parent->to_child().index();
		node_type *child = node_pool_[child_index];
		base_type offset = child->offset();
		if (offset != 0)
		{
			offset ^= parent_index;
			if (!(offset & LOWER_MASK) || !(offset & UPPER_MASK))
			{
				units(parent_index).set_offset(offset);
				return;
			}
		}

		// Finds an offset.
		offset = arrange_child_nodes(parent, parent_index);
		child->set_offset(offset);

		// Builds a double-array in depth-first order.
		do
		{
			child_index = offset ^ child->label();
			build_double_array(child, child_index);

			child_index = child->to_sibling().index();
			child = node_pool_[child_index];
		} while (child_index);
	}

	// Arranges child nodes.
	base_type arrange_child_nodes(node_type *parent, base_type parent_index)
	{
		labels_.clear();

		base_type child_index = parent->to_child().index();
		node_type *child = node_pool_[child_index];
		while (child_index)
		{
			labels_.push_back(child->label());

			child_index = child->to_sibling().index();
			child = node_pool_[child_index];
		}

		// Finds a good offset.
		base_type offset = find_offset(parent_index);
		units(parent_index).set_offset(parent_index ^ offset);

		child_index = parent->to_child().index();
		child = node_pool_[child_index];
		for (size_type i = 0; i < labels_.size(); ++i)
		{
			child_index = offset ^ labels_[i];
			reserve_unit(child_index);

			if (child->has_value())
				units(child_index).set_value(child->value());
			else
				units(child_index).set_label(labels_[i]);

			child_index = child->to_sibling().index();
			child = node_pool_[child_index];
		}
		extras(offset).set_is_used();

		return offset;
	}

	// Finds a good offset.
	base_type find_offset(base_type index) const
	{
		static const base_type LOWER_MASK = unit_type::OFFSET_MAX - 1;
		static const base_type UPPER_MASK = ~LOWER_MASK;

		if (unfixed_index_ >= num_of_units())
			return num_of_units() | (index & 0xFF);

		// Scans empty units.
		base_type unfixed_index = unfixed_index_;
		do
		{
			base_type offset_index = unfixed_index ^ labels_[0];
			base_type offset = index ^ offset_index;

			if (!extras(offset_index).is_used()
				&& (!(offset & LOWER_MASK) || !(offset & UPPER_MASK)))
			{
				// Finds a collision.
				bool has_collision = false;
				for (size_type i = 1; i < labels_.size(); ++i)
				{
					if (extras(offset_index ^ labels_[i]).is_fixed())
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
	void reserve_unit(base_type index)
	{
		if (index >= num_of_units())
			expand_trie();

		if (extras(index).is_fixed())
			THROW("unit collision");

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

	// Adjusts labels of unused units in a given block.
	void fix_block(base_type block_id)
	{
		base_type begin = block_id * BLOCK_SIZE;
		base_type end = begin + BLOCK_SIZE;

		// Finds an unused offset.
		base_type unused_offset_for_label = 0;
		for (base_type offset = begin; offset != end; ++offset)
		{
			if (!extras(offset).is_used())
			{
				unused_offset_for_label = offset;
				break;
			}
		}

		// Labels of empty units and offsets of end units are changed.
		for (base_type index = begin; index != end; ++index)
		{
			if (!extras(index).is_fixed())
			{
				reserve_unit(index);
				units(index).set_label(static_cast<uchar_type>(
					index ^ unused_offset_for_label));
				++num_of_unused_units_;
			}
		}
	}

#ifdef _DEBUG
private:
	// Tests a given set of keys.
	void test_keys() const
	{
		for (size_type i = 0; i < num_of_keys_; ++i)
		{
			// Finds a null pointer.
			if (!keys_[i])
				THROW("null pointer in key set");

			// Finds a null character.
			if (lengths_)
			{
				if (!lengths_)
					THROW("zero length key");

				for (size_type j = 0; j < lengths_[i]; ++j)
				{
					if (!keys_[i][j])
						THROW("null character in key");
				}
			} else if (!keys_[i][0])
				THROW("zero length key");

			// Finds a negative value.
			if (values_ && values_[i] < 0)
				THROW("negative value");

			// Finds a wrong order.
			if (i > 0)
			{
				int comparison_result = lengths_ ? compare_keys(
					keys_[i - 1], lengths_[i - 1], keys_[i], lengths_[i])
					: compare_keys(keys_[i - 1], keys_[i]);
				if (comparison_result > 0)
					THROW("invalid key order");
			}
		}
	}

	// Compares a pair of keys and returns a result like strcmp().
	static int compare_keys(const char_type *lhs, const char_type *rhs)
	{
		while (*lhs != 0 && *lhs == *rhs)
			++lhs, ++rhs;

		return static_cast<uchar_type>(*lhs) - static_cast<uchar_type>(*rhs);
	}

	// Compares a pair of keys and returns a result like strcmp().
	static int compare_keys(const char_type *lhs_key, size_type lhs_length,
		const char_type *rhs_key, size_type rhs_length)
	{
		size_type min_length = std::min(lhs_length, rhs_length);
		size_type key_pos = 0;

		while (key_pos < min_length && lhs_key[key_pos] == rhs_key[key_pos])
			++key_pos;

		if (key_pos == min_length)
			return static_cast<int>(lhs_length - rhs_length);
		return static_cast<uchar_type>(lhs_key[key_pos])
			- static_cast<uchar_type>(rhs_key[key_pos]);
	}

private:
	// Tests a naive trie.
	void test_naive_trie()
	{
		if (!node_pool_.num_of_nodes())
			THROW("empty naive trie");

		base_type num_of_nodes = scan_naive_trie(0);
		if (num_of_nodes != node_pool_.num_of_nodes())
			THROW("unreferenced nodes in naive trie");
	}

	// Scans a trie recursively.
	base_type scan_naive_trie(base_type parent_index)
	{
		base_type num_of_nodes = 0;
		node_type *parent = node_pool_[parent_index];

		if (!parent->to_child().is_child())
			THROW("wrong flag of child edge");
		base_type child_index = parent->to_child().index();
		if (child_index != 0)
		{
			if (node_pool_[child_index]->from().index() != parent_index)
				THROW("invalid reversed path from child");
			num_of_nodes += scan_naive_trie(child_index);
		}

		if (!parent->to_sibling().is_sibling())
			THROW("wrong flag of sibling edge");
		base_type sibling_index = parent->to_sibling().index();
		if (sibling_index != 0)
		{
			if (node_pool_[sibling_index]->from().index() != parent_index)
				THROW("invalid reversed path from sibling");
			num_of_nodes += scan_naive_trie(sibling_index);
		}

		return num_of_nodes + 1;
	}

private:
	// Tests a compressed trie.
	void test_compressed_trie()
	{
		if (num_of_merged_nodes_ >= node_pool_.num_of_nodes())
			THROW("empty compressed trie");

		base_type num_of_nodes = scan_compressed_trie(0);
		if (num_of_nodes != node_pool_.num_of_nodes())
			THROW("unreferenced nodes in compressed trie");
	}

	// Scans a trie recursively.
	base_type scan_compressed_trie(base_type parent_index)
	{
		base_type num_of_nodes = 0;
		node_type *parent = node_pool_[parent_index];

		if (!parent->to_child().is_child())
			THROW("wrong flag of child edge");
		base_type child_index = parent->to_child().index();
		if (child_index != 0)
			num_of_nodes += scan_compressed_trie(child_index);

		if (!parent->to_sibling().is_sibling())
			THROW("wrong flag of sibling edge");
		base_type sibling_index = parent->to_sibling().index();
		if (sibling_index != 0)
			num_of_nodes += scan_compressed_trie(sibling_index);

		return num_of_nodes + 1;
	}
#endif  // _DEBUG

private:
	unit_type &units(base_type index)
	{
		if (index >= num_of_units())
			THROW("out of range");
		return units_[index];
	}
	const unit_type &units(base_type index) const
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
	const extra_type &extras(base_type index) const
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

private:
	// Notifies a progress.
	void progress()
	{
		if (progress_ >= max_progress_)
			return;

		++progress_;
		if (progress_func_)
			progress_func_(progress_, max_progress_);
	}

	// Scans a given key and returns its length.
	static size_type length_of(const char_type *key)
	{
		size_type count = 0;
		while (key[count])
			++count;
		return count;
	}
};

// The base class of the DoubleArray.
class DoubleArray
{
public:
	typedef DoubleArrayBasicTypes::char_type char_type;
	typedef DoubleArrayBasicTypes::uchar_type uchar_type;
	typedef DoubleArrayBasicTypes::base_type base_type;
	typedef DoubleArrayBasicTypes::size_type size_type;
	typedef DoubleArrayBasicTypes::value_type value_type;
	typedef DoubleArrayFile file_type;
	typedef DoubleArrayUnit unit_type;

	struct result_pair_type
	{
		value_type value;
		size_type length;
	};

private:
	const unit_type *units_;
	size_type size_;
	std::vector<unit_type> units_buf_;

	// Copies are not allowed.
	DoubleArray(const DoubleArray &);
	DoubleArray &operator=(const DoubleArray &);

public:
	DoubleArray() : units_(0), size_(0), units_buf_() {}

	// Builds a double-array from a set of keys.
	int build(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths = 0, const value_type *values = 0,
		int (*progress_func)(size_type, size_type) = 0)
	{
		DoubleArrayBuilder builder;
		if (!builder.build(num_of_keys, keys, lengths, values, progress_func))
			return -1;
		replace_units_buf(builder.units_buf());

#ifdef _DEBUG
		test(num_of_keys, keys, lengths, values);
#endif

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

		LOG_VALUE(total_size());
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

		std::vector<unit_type> new_units_buf(num_of_units);
		if (!file.read(&new_units_buf[0], num_of_units))
			return -1;

		replace_units_buf(new_units_buf);
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
		size_type length = 0, size_type node_pos = 0) const
	{ result = exactMatchSearch<ResultType>(key, length, node_pos); }

	// Searches a double-array for a given key.
	template <typename ResultType>
	ResultType exactMatchSearch(const char_type *key,
		size_type length = 0, size_type node_pos = 0) const
	{
		if (!length)
			length = length_of(key);

		ResultType result;
		set_result(result, -1, 0);

		// Transitions.
		base_type index = static_cast<base_type>(node_pos);
		size_type key_pos = 0;
		while (key_pos < length)
		{
			index ^= units_[index].offset()
				^ static_cast<uchar_type>(key[key_pos]);
			if (units_[index].is_leaf() || units_[index].label()
				!= static_cast<uchar_type>(key[key_pos]))
				return result;
			++key_pos;
		}

		index ^= units_[index].offset();
		if (!units_[index].is_leaf())
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

		// Transitions.
		base_type index = static_cast<base_type>(node_pos);
		for (size_type key_pos = 0; key_pos < length; ++key_pos)
		{
			index ^= units_[index].offset()
				^ static_cast<uchar_type>(key[key_pos]);
			if (units_[index].is_leaf() || units_[index].label()
				!= static_cast<uchar_type>(key[key_pos]))
				break;

			const unit_type &stray = units_[index ^ units_[index].offset()];
			if (stray.is_leaf())
			{
				if (num_of_results < max_num_of_results)
				{
					set_result(results[num_of_results],
						stray.value(), key_pos + 1);
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

		// Transitions.
		base_type index = static_cast<base_type>(node_pos);
		for ( ; key_pos < length; ++key_pos)
		{
			index ^= units_[index].offset()
				^ static_cast<uchar_type>(key[key_pos]);
			if (units_[index].is_leaf() || units_[index].label()
				!= static_cast<uchar_type>(key[key_pos]))
				return static_cast<value_type>(-2);

			node_pos = static_cast<size_type>(index);
		}

		index ^= units_[static_cast<base_type>(node_pos)].offset();
		if (units_[index].is_leaf())
			return units_[index].value();

		return static_cast<value_type>(-1);
	}

#ifdef _DEBUG
private:
	// Tests a built double-array.
	void test(size_type num_of_keys, const char_type * const *keys,
		const size_type *lengths = 0, const value_type *values = 0) const
	{
		if (!size())
			THROW("empty double-array");

		for (size_type i = 0; i < num_of_keys; ++i)
		{
			result_pair_type result;
			exactMatchSearch(keys[i], result, lengths ? lengths[i] : 0);

			// Checks a value and a length of a result.
			if (result.value == -1)
				THROW("not found in test");

			const value_type correct_value =
				values ? values[i] : static_cast<value_type>(i);
			if (result.value != correct_value)
				THROW("wrong value in test");

			const size_type correct_length =
				lengths ? lengths[i] : length_of(keys[i]);
			if (result.length != correct_length)
				THROW("wrong length in test");
		}
	}
#endif  // _DEBUG

private:
	// Replaces a current vector with a given vector.
	void replace_units_buf(std::vector<unit_type> &new_units_buf)
	{
		if (new_units_buf.empty())
			THROW("empty unit vector");

		set_array(&new_units_buf[0], new_units_buf.size());
		units_buf_.swap(new_units_buf);
	}

	// Scans a given key and returns its length.
	static size_type length_of(const char_type *key)
	{
		size_type count = 0;
		while (key[count])
			++count;
		return count;
	}

	// Sets a result.
	static void set_result(result_pair_type &result,
		value_type value, size_type length)
	{
		result.value = value;
		result.length = length;
	}
	static void set_result(value_type &result, value_type value, size_type)
	{ result = value; }
};

}  // namespace Darts

// Undefines internal macros.
#undef THROW
#undef THROW_RELAY
#undef THROW_FINAL

#undef LOG
#undef LOG_VALUE

#endif  // DARTS_H_
