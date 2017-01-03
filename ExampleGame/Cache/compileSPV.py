import os
import struct
from subprocess import call

for filename in os.listdir('Shaders/'):
    if filename.endswith('_.spv.cse'):
        continue
    if filename.endswith('.spv.cse'):
        f = open(os.path.join('Shaders/', filename), 'r')
        p2 = ''
        p1 = ''
        watstage = ''
        wat = ''
        start = False
        for line in f:
            if line == '}\n':
                start = False
                if watstage == 'vs\n':
                    g = open('./temp.vert', 'w')
                    g.write(wat)
                    g.close()
                    wat = ''
                if watstage == 'fs\n':
                    g = open('./temp.frag', 'w')
                    g.write(wat)
                    g.close()
                    wat = ''
            if start:
                wat = wat + line
            if line == '{\n':
                watstage = p2
                start = True
            p2 = p1
            p1 = line
        f.close()

        call(['glslangValidator', '-V', 'temp.vert'])
        call(['glslangValidator', '-V', 'temp.frag'])

        outfilename = os.path.splitext(os.path.splitext(filename)[0])[0]+'_'+'.spv.cse'
        f = open(os.path.join('Shaders/', outfilename), 'w')
        # Vertex shader
        f.write('vs\n')
        f.write('binary\n')
        f.write('{\n')
        with open('vert.spv', 'rb') as vs:
            for chunk in iter(lambda: vs.read(4), ''):
                f.write(str(struct.unpack('<i', chunk)[0]))
                f.write(',')
        f.seek(-1, os.SEEK_END)
        f.truncate()
        f.write('\n')
        f.write('}\n')
        f.write('\n');
        
        # Fragment shader
        f.write('fs\n')
        f.write('binary\n')
        f.write('{\n')
        with open('frag.spv', 'rb') as fs:
            for chunk in iter(lambda: fs.read(4), ''):
                f.write(str(struct.unpack('<i', chunk)[0]))
                f.write(',')
        f.seek(-1, os.SEEK_END)
        f.truncate()
        f.write('\n')
        f.write('}\n')
        f.write('\n');
        f.close()

