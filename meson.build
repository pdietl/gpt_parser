project('GPT Parser', 'c')

src = ['main.c', 'readint.c', 'crc32.c']

deps = [dependency('uuid'), dependency('libfort')]

executable('parse_gpt', sources : src, 
    dependencies : deps)
