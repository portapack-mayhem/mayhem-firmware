import math

m = 64 - 1
ft = 300.0 / 24000.0

taps = []
window = []

print("Normalized ft = " + str(ft))

for n in range(0, 64):
	taps.append(math.sin(2 * math.pi * ft * (n - (m / 2.0))) / (math.pi * (n - (m / 2.0))))

for n in range(0, 64):
	window.append(0.5 - 0.5 * math.cos(2 * math.pi * n / m))

for n in range(0, 64):
	taps[n] = int(taps[n] * window[n] * 32768)

print(taps)
