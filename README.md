# VCF2EDS

## Dependencies

* vcflib and hts
  * included in build process
* libz
* liblzma
* libbz2
* libcurl
* libm
  * should be already installed on your system

## Build

```
> make vcf2eds
```

## Run

```
> ./bin/vcf2eds -r ./data/input/Homo_sapiens.GRCh38.dna.chromosome.22.fa.gz -v ./data/input/ALL.chr22.phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf.gz -o ./data/output/output.eds
```