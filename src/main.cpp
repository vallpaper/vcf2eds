#include "eds.h"
#include "utils/cxxopts.h"
#include "utils/kseq.h"
#include "utils/bit_vector.h"

#include <zlib.h>
//#include <stdio.h>
#include <vcflib/Variant.h>

#include <algorithm>
#include <iostream>
#include <string>
#include <map>

KSEQ_INIT(gzFile, gzread)

void parse_gt_record (const std::string & gt_record, int & allel_1, int & allel_2)
{
  // X|Y - X and Y refers to one of variants
  const auto divider_pos = gt_record.find_first_of("|/");

  allel_1 = std::stoi(gt_record.substr(0, divider_pos));
  allel_2 = std::stoi(gt_record.substr(divider_pos + 1));
}

void process_variant_samples (const vcflib::Variant & variant, Segment & segment)
{
  int allel_1, allel_2;
  std::size_t i = 0;
  for (const auto & sample : variant.samples)
  {
    // if (sample.second.size() > 1)
    // {
    //   skipped++;
    //   continue;
    // }

    for (const auto & genotype_field : sample.second)
    {
      if (genotype_field.first == "GT")
      {
        // check only genotype information of allels
        // For each sample
        // - X|Y ... X/Y
        //   - X and Y refers to alleles
        //! Each GT field should only contain one record
        if (genotype_field.second.size() > 1)
        {
          // should not occur;
        }

        const auto & gt_record = genotype_field.second[0];
        parse_gt_record(gt_record, allel_1, allel_2);

        // switch bit of this sample for allel_1 and allel_2
        segment.add_sample_to_variant(allel_1, i);
        segment.add_sample_to_variant(allel_2, i);
      }
      else
      {
        // other fields of genotype information
      }

      // for (const auto & wwtf : genotype_field.second)
      // {
      //   if (wwtf == "0|0" || wwtf == "0/0")
      //     same_as_ref++;
      //   else
      //     number_of_samples++;
      // }
    }
    i++;
  }
}

void experiments(const cxxopts::ParseResult & result, std::vector<std::string> & vcf_files)
{
  std::string reference_file = result["r"].as<std::string>();
  std::string output_file = result["o"].as<std::string>();

  std::cout << "Testing" << std::endl;
  EDS eds;
  
  std::ifstream input(reference_file);
  eds.load(input);

  std::ofstream output(output_file);
  eds.save(output);

  // std::cout << "vcf2eds - header\n";
  // std::cout << "ref: " << reference_file << " out: " << output_file << "\nvcf:\n";
  // std::copy(vcf_files.begin(), vcf_files.end(),
  //           std::ostream_iterator<std::string>(std::cout, "\n"));
  // std::cout << "--------------------------" << std::endl;

  // std::map<std::size_t, std::unique_ptr<Segment>> variants_pos;

  // long int weird = 0;
  // long int skipped = 0;
  // long int same_as_ref = 0;
  // long int number_of_variants = 0;
  // long int number_of_samples = 0;
  // std::size_t samples_cnt = 0;

  // for (auto & vcf_filename : vcf_files)
  // {
  //   vcflib::VariantCallFile vcf_file;
  //   vcf_file.open(vcf_filename);
  //   if (!vcf_file.is_open())
  //   {
  //     std::cout << "Could not open given VCF file: " << std::endl;
  //     return;
  //   }

  //   vcflib::Variant variant(vcf_file);
  //   while (vcf_file.getNextVariant(variant))
  //   {
  //     // TODO: check that alt does not start with '<'
  //     if (variant.alt[0][0] == '<')
  //     {
  //       weird++;
  //       continue;
  //     }

  //     samples_cnt = variant.samples.size();

  //     number_of_variants++;

  //     std::unique_ptr<Segment> segment = std::make_unique<Segment>(variant.position);
  //     segment->add_reference({variant.ref, samples_cnt});
  //     segment->add_variants(begin(variant.alt), end(variant.alt), samples_cnt);

  //     process_variant_samples(variant, *segment);

  //     auto segment_in_map = variants_pos.find(segment->start_position());
  //     if (segment_in_map != variants_pos.end())
  //     {
  //       segment_in_map->second->merge(*segment);
  //     }
  //     else
  //     {
  //       variants_pos.insert(std::make_pair(segment->start_position(), std::move(segment)));
  //     }
  //   }
  // }

  // std::cout << "Stats" << std::endl;
  // std::cout << "weird = " << weird << std::endl;
  // std::cout << "skipped = " << skipped << std::endl;
  // std::cout << "same as ref = " << same_as_ref << std::endl;
  // std::cout << "number of variants = " << number_of_variants << std::endl;
  // std::cout << "number of samples = " << number_of_samples << std::endl;
  // std::cout << "avg per variant = " << static_cast<double>(number_of_samples) / number_of_variants << std::endl;
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

  std::map<std::size_t, std::unique_ptr<Segment>> variants_pos;
  std::size_t samples_cnt = 0;

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
      {
        continue;
      }

      samples_cnt = variant.samples.size();

      std::unique_ptr<Segment> segment = std::make_unique<Segment>(variant.position);
      segment->add_reference(Variant(variant.ref, samples_cnt));
      segment->add_variants(begin(variant.alt), end(variant.alt), samples_cnt);

      process_variant_samples(variant, *segment);

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
  std::size_t processed_pos = 1;

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
              Variant(reference_buffer.substr(processed_pos - 1, segment_ptr->start_position() - processed_pos), 0)
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

void test_bit_vector(const cxxopts::ParseResult & result, std::vector<std::string> & vcf_files)
{
  BitVector<char> bit_vector(8);
  bit_vector.set(0, true);
  bit_vector.set(2, true);
  bit_vector.set(7, true);
  bit_vector.print_read(std::cout);
  assert(bit_vector.get(0) == true);
  assert(bit_vector.get(1) == false);
  assert(bit_vector.get(2) == true);
  assert(bit_vector.get(3) == false);
  assert(bit_vector.get(4) == false);
  assert(bit_vector.get(5) == false);
  assert(bit_vector.get(6) == false);
  assert(bit_vector.get(7) == true);

  bit_vector.set(2, false);
  bit_vector.print_read(std::cout);
  assert(bit_vector.get(0) == true);
  assert(bit_vector.get(1) == false);
  assert(bit_vector.get(2) == false);
  assert(bit_vector.get(3) == false);
  assert(bit_vector.get(4) == false);
  assert(bit_vector.get(5) == false);
  assert(bit_vector.get(6) == false);
  assert(bit_vector.get(7) == true);
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