#include "eds.h"

#include <unordered_set>
#include <initializer_list>
#include <stdexcept>
#include <vector>
#include <sstream>
#include <iostream>
#include <htslib/hts.h>

std::ostream & operator << (std::ostream & os, const EDS & eds)
{
  eds.save(os);
  return os;
}

std::istream & operator >> (std::istream & is, EDS & eds)
{
  eds.load(is);
  return is;
}

std::ostream & operator << (std::ostream & os, const Segment & segment)
{
  if (segment.is_degenerate())
  {
    os << "{";
    const auto last_element = segment.variants.end() - 1;
    for (auto it = segment.variants.begin(); it != segment.variants.end(); ++it)
    {
      os << *it;
      if (it != last_element)
        os << ",";
    }
    os << "}";
  }
  else
  {
    os << segment.variants[0].str;
  }

  return os;
}

std::ostream & operator << (std::ostream & os, const Variant & variant)
{
  os << variant.samples;
  os << variant.str;

  return os;
}

std::istream & operator >> (std::istream & is, Variant & variant)
{
  is >> variant.samples;
  is >> variant.str;

  return is;
}

Segment::Segment(size_t position)
  : position(position)
{ }

Segment::Segment(size_t position, Variant && reference)
  : position(position), variants{reference}
{ }

void Segment::add_reference(Variant && reference)
{
  if (variants.empty())
    variants.emplace_back(reference);
  else
    throw std::logic_error("Reference already added!");
}

void Segment::add_variant(Variant && variant)
{
  variants.emplace_back(variant);
}

void Segment::add_sample_to_variant(const int variant_idx, const std::size_t sample_idx)
{
  variants[variant_idx].samples.set(sample_idx, true);
}

size_t Segment::start_position() const
{
  return position;
}

size_t Segment::end_position() const
{
  return position + variants[0].str.length() - 1;
}

size_t Segment::length() const
{
  return variants[0].str.length();
}

bool Segment::is_degenerate() const
{
  return variants.size() > 1;
}

const std::string & Segment::reference() const
{
  if (variants.size() <= 0)
    throw std::logic_error("No reference.");
  
  return variants[0].str;
}

const BitVectorType & Segment::reference_samples() const
{
  if (variants.size() <= 0)
    throw std::logic_error("No reference.");
  
  return variants[0].samples;
}

void Segment::merge(const Segment & segment)
{
  if (end_position() < segment.start_position() || segment.end_position() < start_position())
  {
    std::cout << "Into: " << *this << std::endl;
    std::cout << "Mergin: " << segment << std::endl;
    throw std::exception();
  }

  std::vector<Variant> copy_variants1(variants.begin(), variants.end());
  std::vector<Variant> copy_variants2(segment.variants.begin(), segment.variants.end());

  std::string new_reference;
  std::string suffix;
  std::string prefix;

  auto extend_variants_left = [&suffix](const Segment & suffix_from, const Segment & rhs, std::vector<Variant> & new_variants) {
    suffix = suffix_from.reference().substr(suffix_from.reference().length() - (suffix_from.end_position() - rhs.end_position()));

    for (auto & variant : new_variants)
      variant.new_str(variant.str + suffix);
  };

  auto extend_variants_right = [&prefix](const Segment & preffix_from, const Segment & rhs, std::vector<Variant> & new_variants) {
    prefix = preffix_from.reference().substr(0, rhs.start_position() - preffix_from.start_position());

    for (auto & variant : new_variants)
      variant.new_str(prefix + variant.str);
  };

  if (start_position() < segment.start_position())
  {
    new_reference = reference();
    extend_variants_right(*this, segment, copy_variants2);
  }
  else if (start_position() > segment.start_position())
  {
    new_reference = segment.reference();
    extend_variants_right(segment, *this, copy_variants1);
  }

  if (end_position() > segment.end_position())
  {
    extend_variants_left(*this, segment, copy_variants2);
  }
  else if (end_position() < segment.end_position())
  {
    extend_variants_left(segment, *this, copy_variants1);
    new_reference += suffix;
  }

  // new reference
  variants[0].new_str(new_reference);
  variants[0].intersect_samples(segment.reference_samples());

  std::vector<Variant> new_variants;
  std::for_each(copy_variants1.begin(), copy_variants1.end(), [&](const auto & variant){ new_variants.push_back(variant); });
  std::for_each(copy_variants2.begin(), copy_variants2.end(), [&](const auto & variant){ new_variants.push_back(variant); });

  variants.swap(new_variants);
}

void EDS::add_segment(std::unique_ptr<Segment> && segment_ptr)
{
  segments.push_back(std::move(segment_ptr));
}

void EDS::save(std::ostream & os) const
{
  for (const auto & segment : segments)
  {
    os << *segment;
  }
}

void EDS::load(std::istream & is)
{
  std::string data((std::istreambuf_iterator<char>(is)),
                  std::istreambuf_iterator<char>());
  size_t current_pos = 0;
  while (current_pos != std::string::npos && current_pos < data.length())
  {
    char peek = data[current_pos];

    if (peek == '{')
    {
      current_pos += 1;
      // parse {..., ..., ...}
      std::unique_ptr<Segment> segment = std::make_unique<Segment>(current_pos + 1);
      auto closing_bracket = data.find_first_of('}', current_pos);
      auto pun = data.find_first_of(',', current_pos);

      if (pun >= closing_bracket)
        throw std::exception();

      segment->add_reference(Variant(std::istringstream(data.substr(current_pos, pun - current_pos))));
      current_pos = pun + 1;

      while (current_pos < closing_bracket)
      {
        pun = data.find_first_of(',', current_pos);
        pun = (pun >= closing_bracket) ? closing_bracket : pun;

        // TODO: this is broken
        segment->add_variant(Variant(std::istringstream(data.substr(current_pos, pun - current_pos))));

        current_pos = pun + 1;
      }

      segments.push_back(std::move(segment));
    }
    else
    {
      auto next_pos = data.find_first_of('{', current_pos);
      if (next_pos == std::string::npos || current_pos < next_pos)
      {
        auto count = next_pos == std::string::npos ? std::string::npos : next_pos - current_pos;

        // create simple segment
        segments.push_back(std::make_unique<Segment>(
                current_pos + 1,
                Variant(data.substr(current_pos, count), 0)
        ));
      }
      current_pos = next_pos;
    }
  }
}

const EDS::SegmentList & EDS::get_segments() const
{
  return segments;
}