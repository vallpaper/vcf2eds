#ifndef __BIT_VECTOR_H__
#define __BIT_VECTOR_H__

// #include <cstddef>
#include <memory>
#include <iostream>

template <class data_type>
class BitVector {
public:
  static constexpr int BITS_PER_BYTE = 8;
  static constexpr std::size_t BITS_PER_DATA_TYPE = sizeof(data_type) * BITS_PER_BYTE;

  BitVector ();
  // BitVector & operator=(const BitVector&) = delete;

  BitVector (std::size_t size);
  BitVector (const BitVector & bit_vector);
  BitVector (BitVector && bit_vector);

  ~BitVector();

  void set (const std::size_t pos, const bool value);

  // Updates current data with given bit vector
  void intersect (const BitVector<data_type> & rhs);

  bool operator [] (const std::size_t pos) const;
  bool get (const std::size_t pos) const;

  std::size_t size() const;

  friend std::ostream & operator << (std::ostream & os, const BitVector<data_type> & bit_vector)
  {
    os << "[" << bit_vector.bit_array_size << "|";
    std::copy(bit_vector.bit_array_storage.begin(), bit_vector.bit_array_storage.end(),
              std::ostream_iterator<data_type>(os));
    os << "]";

    return os;
  }

  friend std::istream & operator >> (std::istream & is, BitVector<data_type> & bit_vector)
  {
    char dummy;
    is >> dummy;
    if (dummy != '[')
      throw std::logic_error("Expected [ while loading bit vector");

    is >> bit_vector.bit_array_size;
    bit_vector.bit_array_storage.reserve(bit_vector.bit_array_size);

    is >> dummy;
    if (dummy != '|')
      throw std::logic_error("Expected | while loading bit vector");

    std::copy(std::istream_iterator<data_type>(is), std::istream_iterator<data_type>(),
              std::back_inserter(bit_vector.bit_array_storage));
    
    is >> dummy;
    if (dummy != ']')
      throw std::logic_error("Expected ] while loading bit vector");

    return is;
  }

  void print_read(std::ostream & os) const;

  void reset (bool reset_value=false);
private:
  std::size_t bit_array_size;
  std::vector<data_type> bit_array_storage;
};

template <class data_type>
BitVector<data_type>::BitVector ()
  : bit_array_size(0)
{ }

template <class data_type>
BitVector<data_type>::BitVector (const std::size_t size)
  : bit_array_size((size + (BITS_PER_DATA_TYPE - 1)) / BITS_PER_DATA_TYPE),
    bit_array_storage(bit_array_size, 0)
{ }

template <class data_type>
BitVector<data_type>::BitVector (const BitVector & bit_vector)
  : bit_array_size(bit_vector.bit_array_size), bit_array_storage(bit_vector.bit_array_storage)
{ }

template <class data_type>
BitVector<data_type>::BitVector (BitVector && bit_vector)
{
  std::swap(bit_array_size, bit_vector.bit_array_size);
  std::swap(bit_array_storage, bit_vector.bit_array_storage);
}

template <class data_type>
BitVector<data_type>::~BitVector()
{
  // delete [] bit_array_storage;
}

template <class data_type>
void BitVector<data_type>::set (const std::size_t pos, const bool value)
{
  // TODO: removed - not needed just for sanity checks in development
  if (pos < 0 || pos >= this->bit_array_size * BITS_PER_DATA_TYPE) {
    throw std::range_error("Attempted to access an illegal position in BitVector.");
  }

  const std::size_t index = pos / BITS_PER_DATA_TYPE;
  // TODO: we may support only set TRUE values and remove one IF statement
  if (value) {
    this->bit_array_storage[index] |= 1 << (BITS_PER_DATA_TYPE - pos - 1);
  } else {
    this->bit_array_storage[index] &= ~(1 << (BITS_PER_DATA_TYPE - pos - 1));
  }
}

template <class data_type>
bool BitVector<data_type>::operator[] (const std::size_t pos) const
{
  return this->get(pos);
};

template <class data_type>
bool BitVector<data_type>::get (const std::size_t pos) const
{
  // TODO: removed - not needed just for sanity checks in development
  if (pos < 0 || pos >= this->bit_array_size * BITS_PER_DATA_TYPE) {
    throw std::range_error("Attempted to access an illegal position in BitVector.");
  }

  const std::size_t index = pos / BITS_PER_DATA_TYPE;
  const int data = this->bit_array_storage[index];

  return (data >> (BITS_PER_DATA_TYPE - pos - 1)) & 1;
}

template <class data_type>
std::size_t BitVector<data_type>::size () const
{
  assert((bit_array_size * BITS_PER_DATA_TYPE) == bit_array_storage.size());

  return bit_array_storage.size();
}

template <class data_type>
void BitVector<data_type>::reset (bool reset_value)
{
  std::fill(bit_array_storage.begin(), bit_array_storage.end(),
            static_cast<data_type>(reset_value));
}

template <class data_type>
void BitVector<data_type>::print_read(std::ostream & os) const
{
  for (const auto & data_byte : bit_array_storage)
  {
    for (std::size_t i = 0; i < BITS_PER_DATA_TYPE; i++)
    {
      std::cout << static_cast<int>((data_byte >> (BITS_PER_DATA_TYPE - i - 1)) & 1);
    }
  }
  std::cout << std::endl;
}

template <class data_type>
void BitVector<data_type>::intersect (const BitVector<data_type> & rhs)
{
  for (std::size_t i = 0; i < bit_array_size; i++)
    bit_array_storage[i] |= rhs.bit_array_storage[i];
}

#endif