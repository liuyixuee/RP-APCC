import matplotlib.pyplot as plt
import numpy as np
import os
import time
#from scipy.interpolate import make_interp_spline
rtt_mode_3=[]
rtt_mode_4=[]
x_mode_3=[]
x_mode_4=[]
plt.figure(figsize=(10, 5))
filepath='../result/'

with open(filepath+'rtt.data','r',encoding='UTF-8') as input:
    for line in input.readlines():
        line=line.strip('\n')
        line=line.split(' ')
        if float(line[0])>7:
            continue
        rtt_mode_3.append(float(line[1]))
        x_mode_3.append(float(line[0]))

#print(throughput)
#print(estimate_delay)
plt.scatter(x_mode_3, rtt_mode_3,color='darksalmon', label='rtt_mode3',s=5)
# plt.plot(x_mode_4, rtt_mode_4,color='mediumpurple', label='RTT_Zhuge',linewidth=1.5)
plt.plot(x_mode_3, rtt_mode_3,color='darksalmon', label='RTT',linewidth=1.5)
# plt.scatter(x_mode_4, rtt_mode_4,color='mediumpurple',s=5)
# plt.scatter(x_mode_3, rtt_mode_3,color='darksalmon',s=5)



# plt.ylim(0.04,0.15)
plt.legend()
plt.ylabel('RTT(s)')
plt.xlabel("time(s)")
now = time.strftime("%Y-%m-%d-%H_%M_%S",time.localtime(time.time()))
plt.savefig('pic/fig_'+now+'.png')
plt.show()

plt.cla()
x=[]
throughput=[]