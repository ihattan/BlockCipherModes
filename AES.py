import blockcipher

import math

block_size = 16
encrypt = 0
decrypt = 1

def ecb( key_in, data, mode ):
	key_l = list(key_in.to_bytes(16, byteorder='big'))

	if mode not in (encrypt, decrypt):
		raise ValueError('mode_in MUST be 0(encrypt) or 1(decrypt)')

	crypted = []
	total_blocks = math.floor(len(data) / block_size)
	iterator = 0

	while iterator < total_blocks:
		block = data[iterator*block_size : (iterator+1)*block_size]
		crypted.extend(blockcipher.blockcipher(key_l, block, mode))
		iterator += 1

	return crypted

def cbc( key_in, iv_in, data, mode ):
	key_l = list(key_in.to_bytes(16, byteorder='big'))
	iv_l = list(iv_in.to_bytes(16, byteorder='big'))

	if mode not in (encrypt, decrypt):
		raise ValueError('mode_in MUST be 0(encrypt) or 1(decrypt)')

	crypted = []
	total_blocks = math.floor(len(data) / block_size)
	iterator = 0
	curr_iv = iv_l

	if mode == encrypt:
		while iterator < total_blocks:
			block = data[iterator*block_size : (iterator+1)*block_size]
			xor = [a ^ b for a, b in zip(block, curr_iv)]
			ciphertext = blockcipher.blockcipher(key_l, xor, mode)
			crypted.extend(ciphertext)
			curr_iv = ciphertext
			iterator += 1
	if mode == decrypt:
		while iterator < total_blocks:
			block = data[iterator*block_size : (iterator+1)*block_size]
			ciphertext = blockcipher.blockcipher(key_l, block, mode)
			xor = [a ^ b for a, b in zip(ciphertext, curr_iv)]
			crypted.extend(xor)
			curr_iv = block
			iterator += 1

	return crypted

def pcbc( key_in, iv_in, data, mode ):
	key_l = list(key_in.to_bytes(16, byteorder='big'))
	iv_l = list(iv_in.to_bytes(16, byteorder='big'))

	if mode not in (encrypt, decrypt):
		raise ValueError('mode_in MUST be 0(encrypt) or 1(decrypt)')

	crypted = []
	total_blocks = math.floor(len(data) / block_size)
	iterator = 0
	curr_iv = iv_l

	if mode == encrypt:
		while iterator < total_blocks:
			block = data[iterator*block_size : (iterator+1)*block_size]
			xor = [a ^ b for a, b in zip(block, curr_iv)]
			ciphertext = blockcipher.blockcipher(key_l, xor, mode)
			crypted.extend(ciphertext)
			curr_iv = [a ^ b for a, b in zip(block, ciphertext)]
			iterator += 1

	if mode == decrypt:
		while iterator < total_blocks:
			block = data[iterator*block_size : (iterator+1)*block_size]
			ciphertext = blockcipher.blockcipher(key_l, block, mode)
			xor = [a ^ b for a, b in zip(ciphertext, curr_iv)]
			crypted.extend(xor)
			curr_iv = [a ^ b for a, b in zip(block, xor)]
			iterator += 1

	return crypted

def cfb( key_in, iv_in, data, mode ):
	key_l = list(key_in.to_bytes(16, byteorder='big'))
	iv_l = list(iv_in.to_bytes(16, byteorder='big'))

	if mode not in (encrypt, decrypt):
		raise ValueError('mode_in MUST be 0(encrypt) or 1(decrypt)')

	crypted = []
	total_blocks = math.floor(len(data) / block_size)
	iterator = 0
	curr_iv = iv_l

	if mode == encrypt:
		while iterator < total_blocks:
			block = data[iterator*block_size : (iterator+1)*block_size]
			ciphertext = blockcipher.blockcipher(key_l, curr_iv, encrypt)
			curr_iv = [a ^ b for a, b in zip(block, ciphertext)]
			crypted.extend(curr_iv)
			iterator += 1

	if mode == decrypt:
		while iterator < total_blocks:
			block = data[iterator*block_size : (iterator+1)*block_size]
			ciphertext = blockcipher.blockcipher(key_l, curr_iv, encrypt)
			xor = [a ^ b for a, b in zip(block, ciphertext)]
			crypted.extend(xor)
			curr_iv = block
			iterator += 1

	return crypted

def ofb( key_in, iv_in, data, mode ):
	key_l = list(key_in.to_bytes(16, byteorder='big'))
	iv_l = list(iv_in.to_bytes(16, byteorder='big'))

	if mode not in (encrypt, decrypt):
		raise ValueError('mode_in MUST be 0(encrypt) or 1(decrypt)')

	crypted = []
	total_blocks = math.floor(len(data) / block_size)
	iterator = 0
	curr_iv = iv_l

	while iterator < total_blocks:
		block = data[iterator*block_size : (iterator+1)*block_size]
		ciphertext = blockcipher.blockcipher(key_l, curr_iv, encrypt)
		xor = [a ^ b for a, b in zip(block, ciphertext)]
		crypted.extend(xor)
		curr_iv = ciphertext
		iterator += 1

	return crypted

def ctr( key_in, nonce_in, data, mode ):
	key_l = list(key_in.to_bytes(16, byteorder='big'))

	if mode not in (encrypt, decrypt):
		raise ValueError('mode_in MUST be 0(encrypt) or 1(decrypt)')

	crypted = []
	total_blocks = math.floor(len(data) / block_size)
	iterator = 0
	nonce_ctr = (nonce_in & 0xffffffffffffffff) << 64

	while iterator < total_blocks:
		nonce_ctr_l = list(nonce_ctr.to_bytes(16, byteorder='big'))
		block = data[iterator*block_size : (iterator+1)*block_size]
		ciphertext = blockcipher.blockcipher(key_l, nonce_ctr_l, encrypt)
		xor = [a ^ b for a, b in zip(block, ciphertext)]
		crypted.extend(xor)
		nonce_ctr += 1
		iterator += 1

	return crypted
