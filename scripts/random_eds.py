from optparse import OptionParser
import random

DNA_ALPHABET = 'ACGT'
PROTEIN_ALPHABET = 'ACDEFGHIKLMNPQRSTVWY'

def get_random_string(alphabet, length):
    return ''.join([random.choice(alphabet) for _ in range(length)])

if __name__ == "__main__":
    parser = OptionParser()
    parser.add_option('-a', '--alphabet', dest='alphabet',
                    help="Alphabet type (valid values: D for DNA, P for Protein")
    parser.add_option('-p', '--probability', dest="probability", default=0.18, metavar='NUMBER',
                    type='float', help="Probability of degenerate segment on each position")
    parser.add_option('-e', '--average', dest="average", default=4, metavar='NUMBER',
                    type='int', help="Gaussian distribution mean of the length of degenerate segment")
    parser.add_option('-d', '--deviation', dest="deviation", default=0.5, metavar='NUMBER',
                    type='float', help="Gaussian distribution standard deviation of the length of degenerate segment")
    parser.add_option('-n', '--number', dest="number", default=3, metavar='NUMBER',
                    type='int', help="Gaussian distribution mean of the number of string in degenerate segment")
    parser.add_option('-m', '--ndeviation', dest="number_deviation", default=0.5, metavar='NUMBER',
                    type='float', help="Gaussian distribution standard deviation of the number of string in degenerate segment")
    parser.add_option('-l', '--length', dest="length", default=1000, metavar='NUMBER',
                    type='int', help="Length of degenerate segment")
    parser.add_option('-o', '--output', dest="output", metavar='FILE', default='output.eds',
                    help='Store generated file to given file')

    (options, args) = parser.parse_args()

    print('Random EDS string generator 0.1.0')

    alphabet = DNA_ALPHABET
    if options.alphabet == 'P':
        alphabet = PROTEIN_ALPHABET
    
    if options.probability < 0 or options.probability > 1:
        raise Exception('Probability can be only from interval [0, 1] including 0 and 1.')

    print('  alphabet: {} [{}]'.format(options.alphabet, alphabet))
    print('  Degenerate segment pp: {}'.format(options.probability))
    print('  Gauss avg. length: {}'.format(options.average))
    print('  Gauss standard deviation length: {}'.format(options.deviation))
    print('  Gauss avg. number: {}'.format(options.number))
    print('  Gauss standard deviation number: {}'.format(options.number_deviation))
    print('  EDS length: {}'.format(options.length))
    print('  EDS output file: {}'.format(options.output))

    eds_str = ''
    i = 0
    while i < options.length:
        if random.random() < options.probability:
            # degenerate segment

            degenerate_segment = []
            for _ in range(int(random.gauss(options.number, options.number_deviation))):
                degenerate_segment.append(get_random_string(alphabet, int(random.gauss(options.average, options.deviation))))

            if len(degenerate_segment) == 0:
                degenerate_segment = get_random_string(alphabet, int(random.gauss(options.average, options.deviation)))

            eds_str += '{' + ','.join(degenerate_segment) + '}'
            i += len(degenerate_segment[0])
        else:
            # simple segment
            eds_str += random.choice(alphabet)
            i += 1

    with open(options.output, "w") as output_file:
        print(eds_str, file=output_file)
