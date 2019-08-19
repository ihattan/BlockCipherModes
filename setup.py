from distutils.core import setup, Extension

blockcipher = Extension('blockcipher', sources = ['blockcipher.c'])

setup (	name = 'BlockCipher',
		version = '1.0',
		description = 'testing blockcipher',
		ext_modules = [blockcipher])
