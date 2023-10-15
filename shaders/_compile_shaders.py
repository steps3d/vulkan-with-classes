import os
import subprocess
import pathlib
import glob

glslang_cmd = "glslangValidator"

def compile_shader ( shader ):
    print ( "Compiling", shader )
    return subprocess.run ( [ glslang_cmd, "-V", shader, "-o", "{}.spv".format(shader) ] ).returncode

if __name__ == '__main__':
    ext_list = ( "vert", "frag", "geom", "comp", "tesc", "tese" )
    for ext in ext_list:
        shaders = glob.glob ( "*." + ext )
        for shader in shaders:
            compile_shader ( shader )
