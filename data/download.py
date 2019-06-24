import urllib.request
from optparse import OptionParser


if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option("-f", "--folder", dest="folder",
                        help="Folder where to download chromosomes", metavar="FOLDER", default='./')
    (options, args) = parser.parse_args()
    download_folder = options.folder

    chrom_url = 'ftp://ftp.ensembl.org/pub/release-92/fasta/homo_sapiens/dna/'
    chrom_file = 'Homo_sapiens.GRCh38.dna.chromosome.{}.fa.gz'
    vcf_url = 'http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/release/20130502/'
    vcf_file = 'ALL.chr{}.phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf.gz'
    vcf_idx_url = 'http://ftp.1000genomes.ebi.ac.uk/vol1/ftp/release/20130502/'
    vcf_idx_file = 'ALL.chr{}.phase3_shapeit2_mvncall_integrated_v5a.20130502.genotypes.vcf.gz.tbi'

    chroms = ['1'] #, '2', '3', '4', '5', '6', '7', '8', '9', '10', '11', '12', '13', '14', '15', '16', '17', '18', '19', '20', '21', '22']

    for chrom in chroms:
        chrom_file_name = chrom_file.format(chrom)
        f_url = chrom_url + chrom_file_name
        vcf_file_name = vcf_file.format(chrom)
        f_vcf = vcf_url + vcf_file_name
        vcf_ifx_file_name = vcf_idx_file.format(chrom)
        f_vcf_idx = vcf_idx_url + vcf_ifx_file_name

        print('=========================================================')
        print('Downloading: ' + chrom_file_name + '\n\tfrom: ' + f_url)
        urllib.request.urlretrieve(f_url, download_folder + '/' + chrom_file_name)
        print('Downloading: ' + vcf_file_name + '\n\tfrom: ' + f_vcf)
        urllib.request.urlretrieve(f_vcf, download_folder + '/' + vcf_file_name)
        print('Downloading: ' + vcf_ifx_file_name + '\n\tfrom: ' + f_vcf_idx)
        urllib.request.urlretrieve(f_vcf_idx, download_folder + '/' + vcf_ifx_file_name)
    
    print('---- Finished download ----')
