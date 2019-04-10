#include "eds.h"

#include <unordered_set>
#include <vector>
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
    os << "{" << segment.reference;
    for (const auto & s : segment.variants)
      os << "," << s;
    os << "}";
  }
  else
  {
    os << segment.reference;
  }

  return os;
}

Segment::Segment(size_t position)
  : position(position)
{ }

Segment::Segment(size_t position, std::string && reference)
  : position(position), reference(std::move(reference))
{ }

void Segment::add_reference(const std::string & ref)
{
  reference = ref;
}

void Segment::add_variant(const std::string & variant)
{
  variants.insert(variant);
}

size_t Segment::start_position() const
{
  return position;
}

size_t Segment::end_position() const
{
  return position + reference.length() - 1;
}

size_t Segment::length() const
{
  return reference.length();
}

bool Segment::is_degenerate() const
{
  return !variants.empty();
}

void Segment::merge(const Segment & segment)
{
  if (end_position() < segment.start_position() || segment.end_position() < start_position())
  {
    throw std::exception();
  }

  int prefix_id = 0;
  int suffix_id = 0;
  if (start_position() < segment.start_position())
  {
    // this has some prefix
    prefix_id = 1;
  }
  else if (start_position() > segment.start_position())
  {
    // segment has some prefix
    prefix_id = 2;
  }

  if (end_position() < segment.end_position())
  {
    // segment has some suffix
    suffix_id = 2;

  }
  else if (end_position() > segment.end_position())
  {
    // this has some suffix
    suffix_id = 1;
  }

  std::string prefix;
  std::string suffix;
  std::string new_reference;
  if (prefix_id)
  {
    assert(prefix_id == 1 || prefix_id == 2);

    if (prefix_id == 1)
      prefix = reference.substr(0, (segment.start_position() - start_position()));
    else
      prefix = segment.reference.substr(0, (start_position() - segment.start_position()));
  }

  if (suffix_id)
  {
    assert(suffix_id == 1 || suffix_id == 2);

    if (suffix_id == 1)
      suffix = reference.substr(reference.length() - (end_position() - segment.end_position()));
    else
      suffix = segment.reference.substr(segment.reference.length() - (segment.end_position() - end_position()));
  }

  // new reference
  if (prefix_id || suffix_id)
  {
    if (prefix_id == 1)
    {
      if (suffix_id == 2)
        new_reference = reference + suffix;
      else
        new_reference = reference;
    }
    else if (prefix_id == 2)
    {
      if (suffix_id == 1)
        new_reference = segment.reference + suffix;
      else
        new_reference = segment.reference;
    }
    else // prefix_id == 0
    {
      if (suffix_id == 1)
        new_reference = reference;
      else
        new_reference = segment.reference;
    }
  }
  else
  {
    new_reference = reference;
  }

  // generate new variants
  std::vector<std::string> copy_variants1(variants.begin(), variants.end());
  std::vector<std::string> copy_variants2(segment.variants.begin(), segment.variants.end());
  if (prefix.length() > 0) // musime vsechny prvky z mnoziny rozsirit o prefix
  {
    if (prefix_id == 1)
    {
      // prefix add pro vsechny prvky z id = 2
      std::transform(copy_variants2.begin(), copy_variants2.end(), copy_variants2.begin(),
              [&](const auto & variant) { return prefix + variant; });
    }
    else // prefix_id == 2
    {
      std::transform(copy_variants1.begin(), copy_variants1.end(), copy_variants1.begin(),
                     [&](const auto & variant) { return prefix + variant; });
    }
  }

  if (suffix.length() > 0) // musime vsechny prvky z mnoziny rozsirit o suffix
  {
    if (suffix_id == 1)
    {
      // suffix add pro vsechny prvky z id = 2
      std::transform(copy_variants2.begin(), copy_variants2.end(), copy_variants2.begin(),
                     [&](const auto & variant) { return variant + suffix; });
    }
    else // prefix_id == 2
    {
      std::transform(copy_variants1.begin(), copy_variants1.end(), copy_variants1.begin(),
                     [&](const auto & variant) { return variant + suffix; });
    }
  }

  reference = new_reference;
  std::unordered_set<std::string> new_variants;
  std::for_each(copy_variants1.begin(), copy_variants1.end(), [&](const auto & variant){ new_variants.insert(variant); });
  std::for_each(copy_variants2.begin(), copy_variants2.end(), [&](const auto & variant){ new_variants.insert(variant); });

  if (new_variants.count(reference) > 0)
    new_variants.erase(reference);

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

      segment->add_reference(data.substr(current_pos, pun - current_pos));
      current_pos = pun + 1;

      while (current_pos < closing_bracket)
      {
        pun = data.find_first_of(',', current_pos);
        pun = (pun >= closing_bracket) ? closing_bracket : pun;

        segment->add_variant(data.substr(current_pos, pun - current_pos));

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
                data.substr(current_pos, count)
        ));
      }
      current_pos = next_pos;
    }
  }
}

SegmentList EDS::get_segments() const
{
  return segments;
}