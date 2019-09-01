#ifndef VCF2EDS_EDS_H
#define VCF2EDS_EDS_H

#include "utils/bit_vector.h"

#include <cstddef>
#include <string>
#include <unordered_set>
#include <algorithm>
#include <vector>
#include <ostream>

using BitVectorType = BitVector<char>;

struct Variant
{
  Variant(const std::string & str, const std::size_t samples_count)
    : str(str), samples(samples_count)
  { }

  Variant(const std::string & str, const BitVectorType & bit_vector)
    : str(str), samples(bit_vector)
  { }

  Variant(std::istream && is) {
    is >> *this;
  }

  void new_str(const std::string & new_str) {
    str = new_str;
  }

  Variant(const Variant & rhs)
    : str(rhs.str), samples(rhs.samples)
  { }

  void intersect_samples(const BitVectorType & bit_vector) {
    samples.intersect(bit_vector);
  }

  friend std::ostream & operator << (std::ostream & os, const Variant & variant);
  friend std::istream & operator >> (std::istream & is, Variant & variant);

  std::string str;
  BitVectorType samples;
};

class Segment
{
  using VariantListType = std::unordered_set<std::string>;
public:
  Segment() = default;
  explicit Segment(size_t position);
  Segment(size_t position, Variant && reference);

  template<class InputIt>
  void add_variants(InputIt first, InputIt last, const std::size_t samples_cnt)
  {
    for (InputIt i = first; i != last; i++)
    {
      variants.emplace_back(Variant(*i, samples_cnt));
    }
  }
  void add_variant(Variant && variant);
  void add_reference(Variant && reference);

  void add_sample_to_variant(const int variant_idx, const std::size_t sample_idx);

  const std::string & reference() const;
  const BitVectorType & reference_samples() const;

  size_t start_position() const;
  size_t end_position() const;
  size_t length() const;
  void merge(const Segment & segment);

  bool is_degenerate() const;

  friend std::ostream & operator << (std::ostream & os, const Segment & segment);
private:
  size_t position = -1;
  // reference sequence is at 0 position
  std::vector<Variant> variants;
};

class EDS
{
public:
  using SegmentList = std::vector<std::unique_ptr<Segment>>;

  EDS() = default;

  void save(std::ostream & os) const;
  void load(std::istream & os);

  void add_segment(std::unique_ptr<Segment> && segment_ptr);

  const SegmentList & get_segments() const;

  friend std::ostream & operator << (std::ostream & os, const EDS & eds);
  friend std::istream & operator >> (std::istream & is, EDS & eds);
private:
  SegmentList segments;
};

#endif //VCF2EDS_EDS_H
