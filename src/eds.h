#ifndef VCF2EDS_EDS_H
#define VCF2EDS_EDS_H

#include <cstddef>
#include <string>
#include <unordered_set>
#include <vector>
#include <ostream>

class Segment
{
  using VariantListType = std::unordered_set<std::string>;
public:
  Segment() = default;
  explicit Segment(size_t position);
  Segment(size_t position, std::string && reference);

  void add_reference(const std::string & ref);
  void add_variant(const std::string & variant);
  template<class InputIt>
  void add_variants(InputIt first, InputIt last)
  {
    variants.insert(first, last);
  }

  size_t start_position() const;
  size_t end_position() const;
  size_t length() const;
  void merge(const Segment & segment);

  bool is_degenerate() const;

  friend std::ostream & operator << (std::ostream & os, const Segment & segment);
private:
  size_t position = -1;
  std::string reference;
  std::unordered_set<std::string> variants;
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
