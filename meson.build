project('GPT Parser', 'c')

fort_inc = include_directories('libfort/lib')

src = ['main.c', 'libfort/lib/fort.c', 'readint.c', 'crc32.c']

deps = [dependency('uuid')]

executable('parse_gpt', sources : src, 
    include_directories : fort_inc,
    dependencies : deps)
