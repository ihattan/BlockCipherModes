import AES

import imageio as im
import numpy as np
import math

block_size = 16
encrypt = 0
decrypt = 1

def create_gif ( pt, en, de ):
	animated = []

	anim_row_block = 575
	anim_col_block = 30
	total_rows = int(pt.shape[0] / anim_row_block)
	total_cols = int(pt.shape[1] / anim_col_block)
	
	for i in range(total_rows):
		for j in range(total_cols):
			en_col_slice = en[: (anim_row_block*i) ]
			en_row_slice = en[ (anim_row_block*i):(anim_row_block*(i+1)), :(j*anim_col_block) ]
			
			pt_col_slice = pt[ (anim_row_block*(i+1)): ]
			pt_row_slice = pt[ (anim_row_block*i):(anim_row_block*(i+1)), (j*anim_col_block): ]
			row = np.append(en_row_slice, pt_row_slice, axis=1)
			frame = np.vstack((en_col_slice, row, pt_col_slice))
			
			animated.append(frame)

	for i in range(total_rows):
		for j in range(total_cols):
			de_col_slice = de[: (anim_row_block*i) ]
			de_row_slice = de[ (anim_row_block*i):(anim_row_block*(i+1)), :(j*anim_col_block) ]
			
			en_col_slice = en[ (anim_row_block*(i+1)): ]
			en_row_slice = en[ (anim_row_block*i):(anim_row_block*(i+1)), (j*anim_col_block): ]
			row = np.append(de_row_slice, en_row_slice, axis=1)
			frame = np.vstack((de_col_slice, row, en_col_slice))
			
			animated.append(frame)

	return animated

key = 0x123456789abcdef
iv = 0xfedcba987654321
nonce = iv & 0xffffffff00000000

pic = im.imread('C:\\Users\\Ian\\Desktop\\blockcipher\\resources\\lolli.png')
data = list(pic.flatten())

ecb_en = AES.ecb(key, data, 0)
ecb_de = AES.ecb(key, ecb_en, 1)
np_ecb_en = np.asarray(ecb_en, dtype=np.uint8).reshape(pic.shape)
np_ecb_de = np.asarray(ecb_de, dtype=np.uint8).reshape(pic.shape)
ecb_animated = create_gif(pic, np_ecb_en, np_ecb_de)

im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\en_ecb.png', np_ecb_en)
im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\de_ecb.png', np_ecb_de)
im.mimwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_ecb.gif', ecb_animated, duration=0.01)


cbc_en = AES.cbc(key, iv, data, 0)
cbc_de = AES.cbc(key, iv, cbc_en, 1)
np_cbc_en = np.asarray(cbc_en, dtype=np.uint8).reshape(pic.shape)
np_cbc_de = np.asarray(cbc_de, dtype=np.uint8).reshape(pic.shape)
cbc_animated = create_gif(pic, np_cbc_en, np_cbc_de)
im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\en_cbc.png', np_cbc_en)
im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\de_cbc.png', np_cbc_de)
im.mimwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_cbc.gif', cbc_animated, duration=0.01)

pcbc_encrypted = AES.pcbc(key, iv, data, 0)
pcbc_decrypted = AES.pcbc(key, iv, pcbc_encrypted, 1)
np_pcbc_en = np.asarray(pcbc_encrypted, dtype=np.uint8).reshape(pic.shape)
np_pcbc_de = np.asarray(pcbc_decrypted, dtype=np.uint8).reshape(pic.shape)
pcbc_animated = create_gif(pic, np_pcbc_en, np_pcbc_de)

im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\en_pcbc.png', np_pcbc_en)
im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\de_pcbc.png', np_pcbc_de)
im.mimwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_pcbc.gif', pcbc_animated, duration=0.01)

cfb_en = AES.cfb(key, iv, data, 0)
cfb_de = AES.cfb(key, iv, cfb_en, 1)
np_cfb_en = np.asarray(cfb_en, dtype=np.uint8).reshape(pic.shape)
np_cfb_de = np.asarray(cfb_de, dtype=np.uint8).reshape(pic.shape)
cfb_animated = create_gif(pic, np_cfb_en, np_cfb_de)

im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\en_cfb.png', np_cfb_en)
im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\de_cfb.png', np_cfb_de)
im.mimwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_cfb.gif', cfb_animated, duration=0.01)

ofb_en = AES.ofb(key, iv, data, 0)
ofb_de = AES.ofb(key, iv, ofb_en, 1)
np_ofb_en = np.asarray(ofb_en, dtype=np.uint8).reshape(pic.shape)
np_ofb_de = np.asarray(ofb_de, dtype=np.uint8).reshape(pic.shape)
ofb_animated = create_gif(pic, np_ofb_en, np_ofb_de)

im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\en_ofb.png', np_ofb_en)
im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\de_ofb.png', np_ofb_de)
im.mimwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_ofb.gif', ofb_animated, duration=0.01)

ctr_en = AES.ctr(key, nonce, data, 0)
ctr_de = AES.ctr(key, nonce, ctr_en, 1)
np_ctr_en = np.asarray(ctr_en, dtype=np.uint8).reshape(pic.shape)
np_ctr_de = np.asarray(ctr_de, dtype=np.uint8).reshape(pic.shape)
ctr_animated = create_gif(pic, np_ctr_en, np_ctr_de)

im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\en_ctr.png', np_ctr_en)
im.imwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\crypted\\de_ctr.png', np_ctr_de)
im.mimwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_ctr.gif', ctr_animated, duration=0.01)
