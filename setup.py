from distutils.core import setup, Extension

sorteddict = Extension('sorteddict',
                       sources=['sorteddict.cpp'],
                       extra_compile_args=['-std=c++11'])

setup(name='SortedDict', version='0.1', ext_modules=[sorteddict])
