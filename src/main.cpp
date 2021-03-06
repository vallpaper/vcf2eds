#include "eds.h"
#include "utils/cxxopts.h"
#include "utils/kseq.h"

#include <zlib.h>
//#include <stdio.h>
#include <vcflib/Variant.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <map>

KSEQ_INIT(gzFile, gzread)

void experiments(const cxxopts::ParseResult & result, std::vector<std::string> & vcf_files)
{
  std::string reference_file = result["r"].as<std::string>();
  std::string output_file = result["o"].as<std::string>();

  std::cout << "vcf2eds - header\n";
  std::cout << "ref: " << reference_file << " out: " << output_file << "\nvcf:\n";
  std::copy(vcf_files.begin(), vcf_files.end(),
            std::ostream_iterator<std::string>(std::cout, "\n"));
  std::cout << "--------------------------" << std::endl;

  std::map<size_t, std::unique_ptr<Segment>> variants_pos;

  long int weird = 0;
  long int skipped = 0;
  long int same_as_ref = 0;
  long int number_of_variants = 0;
  long int number_of_samples = 0;

  for (auto & vcf_filename : vcf_files)
  {
    vcflib::VariantCallFile vcf_file;
    vcf_file.open(vcf_filename);
    if (!vcf_file.is_open())
    {
      std::cout << "Could not open given VCF file: " << std::endl;
      return;
    }

    vcflib::Variant variant(vcf_file);
    while (vcf_file.getNextVariant(variant))
    {
      // TODO: check that alt does not start with '<'
      if (variant.alt[0][0] == '<')
      {
        weird++;
        continue;
      }

      number_of_variants++;

      // std::cout << "--------------" << std::endl;
      // std::cout << "num samples = " << variant.getNumSamples() << std::endl;
      for (const auto & sample : variant.samples)
      {
        if (sample.second.size() > 1)
        {
          skipped++;
          continue;
        }

        for (const auto & wtf : sample.second)
        {
          if (wtf.first != "GT" || wtf.second.size() > 1)
          {
            skipped++;
            continue;
          }

          for (const auto & wwtf : wtf.second)
          {
            if (wwtf == "0|0" || wwtf == "0/0")
              same_as_ref++;
            else
              number_of_samples++;
          }
        }
      }

      std::unique_ptr<Segment> segment = std::make_unique<Segment>(variant.position);
      segment->add_reference(variant.ref);
      segment->add_variants(begin(variant.alt), end(variant.alt));

      auto segment_in_map = variants_pos.find(segment->start_position());
      if (segment_in_map != variants_pos.end())
      {
        segment_in_map->second->merge(*segment);
      }
      else
      {
        variants_pos.insert(std::make_pair(segment->start_position(), std::move(segment)));
      }
    }
  }

  std::cout << "Stats" << std::endl;
  std::cout << "weird = " << weird << std::endl;
  std::cout << "skipped = " << skipped << std::endl;
  std::cout << "same as ref = " << same_as_ref << std::endl;
  std::cout << "number of variants = " << number_of_variants << std::endl;
  std::cout << "number of samples = " << number_of_samples << std::endl;
  std::cout << "avg per variant = " << static_cast<double>(number_of_samples) / number_of_variants << std::endl;
}

void vcf2eds_exec(const cxxopts::ParseResult & result, std::vector<std::string> & vcf_files)
{
  std::string reference_file = result["r"].as<std::string>();
  std::string output_file = result["o"].as<std::string>();

  std::cout << "vcf2eds - header\n";
  std::cout << "ref: " << reference_file << " out: " << output_file << "\nvcf:\n";
  std::copy(vcf_files.begin(), vcf_files.end(),
            std::ostream_iterator<std::string>(std::cout, "\n"));
  std::cout << "--------------------------" << std::endl;

  std::map<size_t, std::unique_ptr<Segment>> variants_pos;

  for (auto & vcf_filename : vcf_files)
  {
    vcflib::VariantCallFile vcf_file;
    vcf_file.open(vcf_filename);
    if (!vcf_file.is_open())
    {
      std::cout << "Could not open given VCF file: " << std::endl;
      return;
    }

    vcflib::Variant variant(vcf_file);
    while (vcf_file.getNextVariant(variant))
    {
      if (variant.alt[0][0] == '<')
        continue;

      std::unique_ptr<Segment> segment = std::make_unique<Segment>(variant.position);
      segment->add_reference(variant.ref);
      segment->add_variants(begin(variant.alt), end(variant.alt));

      auto segment_in_map = variants_pos.find(segment->start_position());
      if (segment_in_map != variants_pos.end())
      {
        segment_in_map->second->merge(*segment);
      }
      else
      {
        variants_pos.insert(std::make_pair(segment->start_position(), std::move(segment)));
      }
    }
  }

  std::cout << "--------------- creating EDS -----------------" << std::endl;
  EDS eds;

  // read reference sequence
  gzFile file_ptr;
  kseq_t *sequence;
  int l;
  file_ptr = gzopen(reference_file.c_str(), "r");
  sequence = kseq_init(file_ptr);

  std::string reference_buffer;
  size_t processed_pos = 1;

  while ((l = kseq_read(sequence)) >= 0)
  {
    reference_buffer.append(sequence->seq.s, sequence->seq.l);
  }

  // merge all overlapping segments in variants_pos
  std::cout << "count " << variants_pos.size() << std::endl;
  for (auto iter = variants_pos.begin(); iter != variants_pos.end(); ++iter)
  {
    auto segment_ptr = std::move(iter->second);

    // 1. create segment of preceeding normal reference and add to EDS
    if (processed_pos < segment_ptr->start_position())
    {
      eds.add_segment(std::make_unique<Segment>(
              processed_pos,
              reference_buffer.substr(processed_pos - 1, segment_ptr->start_position() - processed_pos)
      ));
    }

    // 2. if any following segments overlap - merge
    auto iter_tmp = iter;
    while (++iter_tmp != variants_pos.end()
            && iter_tmp->second->start_position() <= segment_ptr->end_position())
    {
      segment_ptr->merge(*(iter_tmp->second));
      iter = iter_tmp;
    }

    processed_pos = segment_ptr->end_position() + 1;
    eds.add_segment(std::move(segment_ptr));
  }

  kseq_destroy(sequence);
  gzclose(file_ptr);

  // save to output file
  std::ofstream output(output_file);
  eds.save(output);
}

int main(int argc, char * argv[])
{
  cxxopts::Options options("vcf2eds", "Converts VCF files to elastic degenerate string.");

  std::vector<std::string> vcf_files;

  options.add_options()
          ("o,output", "File name of resulting EDS file", cxxopts::value<std::string>())
          ("r,reference", "File name of reference", cxxopts::value<std::string>())
          ("v,vcf", "Input VCF files", cxxopts::value<std::vector<std::string>>(vcf_files))
          ("m,merge", "Length of gaps that will be merged", cxxopts::value<int>())
          ("t,test", "Test features and statistics - dev", cxxopts::value<bool>()->default_value("false"))
          ;

  auto result = options.parse(argc, argv);

  if (result["t"].as<bool>())
    experiments(result, vcf_files);
  else
    vcf2eds_exec(result, vcf_files);
}