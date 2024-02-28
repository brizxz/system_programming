import matplotlib.pyplot as plt
a = [1, 2,5,10,30,50,65,75,85 ,100]
r_time = [1.325,2.703,6.360,16.015,17.410,16.125,15.584,16.062,15.907,15.775]
u_time = [1.166,3.283,6.921,10.113,12.453,14.548,14.399,14.979,14.747,15.920]
s_time = [0.966,3.049,14.063,39.927,44.783,49.900,48.304,51.437,47.320,51.812]

# 繪製三條折線
plt.plot(a, r_time, label='real time', marker='.')
plt.plot(a, u_time, label='user time', marker='.')
plt.plot(a, s_time, label='system time', marker='.')

# 加入標題和圖例
plt.xlabel("thread numbers")
plt.ylabel("time(seconds)")
plt.title('t-n_threads graph')
plt.legend()

# 顯示圖表
plt.savefig("p5.png")
