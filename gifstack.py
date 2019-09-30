import numpy as np
import imageio as im

ecb = im.mimread('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_ecb.gif')
ctr = im.mimread('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\an_ctr.gif')

stack = np.hstack((ecb, ctr))
im.mimwrite('C:\\Users\\Ian\\Desktop\\blockcipher\\animated\\stack.gif', stack, duration=0.01)