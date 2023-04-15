import re

input_file = 'input.txt'
output_file = 'output.txt'

allowed_domains = [
    'rr.com',
    'roadrunner.cm',
    'Charter.net',
    'brighthouse.com',
    'twc.com',
]

allowed_email_pattern = r'@({})'.format('|'.join(allowed_domains))

with open(input_file, 'r') as infile, open(output_file, 'w') as outfile:
    for line in infile:
        if re.search(allowed_email_pattern, line, re.IGNORECASE):
            outfile.write(line)
