import AES

key = 0x123456789abcdef
iv = 0xfedcba987654321
f_path = 'C:\\Users\\Ian\\Desktop\\blockcipher\\in.txt'
file_input = b'testing123456789987654321'

file = open(f_path, mode='w+b')
file.write(file_input)
file.close()

print('encrypt...')
AES.ecb( key, f_path, 0 )
print('done')

print('decrypt...')
AES.ecb( key, f_path, 1 )
print('done')

# print('encrypt...')
# AES.cbc( key, iv, f_path, 0 )
# print('done')
#
# print('decrypt...')
# AES.cbc( key, iv, f_path, 1 )
# print('done')

# print('encrypt...')
# AES.pcbc( key, iv, f_path, 0 )
# print('done')
#
# print('decrypt...')
# AES.pcbc( key, iv, f_path, 1 )
# print('done')

# print('encrypt...')
# AES.cfb( key, iv, f_path, 0 )
# print('done')
#
# print('decrypt...')
# AES.cfb( key, iv, f_path, 1 )
# print('done')

# print('encrypt...')
# AES.ofb( key, iv, f_path, 0 )
# print('done')
#
# print('decrypt...')
# AES.ofb( key, iv, f_path, 1 )
# print('done')

# nonce = 0xfedcba98
# print('encrypt...')
# AES.ctr( key, nonce, f_path, 0 )
# print('done')
#
# print('decrypt...')
# AES.ctr( key, nonce, f_path, 1 )
# print('done')
