import matplotlib.pyplot as plt
import numpy as np

def is_float(n):
    try:
        float(n)
        return True
    except:
        return False

f = open("output.txt", "r")
a = f.readline()
#plot 1
x = []
y = []

for i in range(1, 50):
    a = [float(n) for n in f.readline().split() if is_float(n)]
    x.append(a[0])
    y.append(a[1])

# print(x)
# print(y)

xpoints = np.array(x)
ypoints = np.array(y)

plt.subplot(1, 2, 1)
plt.plot(xpoints, ypoints)

plt.title("task_1", loc = "left")
plt.xlabel("thread_num")
plt.ylabel("time")

#plot 2
a = f.readline()
a = f.readline()
print(a)

x = []
y_1 = []
y_2 = []

for i in range(1, 50):
    a = [float(n) for n in f.readline().split() if is_float(n)]
    x.append(a[0])
    y_1.append(a[1])
    y_2.append(a[2])

xpoints = np.array(x)
ypoints_1 = np.array(y_1)
ypoints_2 = np.array(y_2)

plt.subplot(1, 2, 2)
plt.plot(xpoints, ypoints_1, xpoints, ypoints_2)

plt.title("task_2", loc = "left")
plt.xlabel("size")
plt.ylabel("time")



plt.show()

