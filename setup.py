from distutils.core import setup, Extension

module = Extension('blockcipher', sources = ['blockcipher.c'])

setup(name = 'blockcipher',
	  version = '0.2',
	  description = 'Package to implement a block cipher',
	  ext_modules = [module])