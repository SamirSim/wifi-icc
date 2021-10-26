import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker

sns.set_style("whitegrid")

data = pd.read_csv("surveillance/success-rate/Success-Rate-point-copie.csv", delimiter=";")

#print(data)

data = data.melt('Nsta', var_name='Configurations',  value_name='Success Rate (%)')
g = sns.catplot(x="Nsta", y="Success Rate (%)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of stations', ylabel='Success Rate (%)')

plt.show()

data = pd.read_csv("surveillance/energy/Ratio-Global-point-tronque-inverse.csv", delimiter=";")

#print(data)

data = data.melt('Nsta', var_name='Configurations',  value_name='Energy efficiency (Megabyte/J)')
g = sns.catplot(x="Nsta", y="Energy efficiency (Megabyte/J)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of stations', ylabel='Energy efficiency (Megabyte/J)')

plt.show()

data = pd.read_csv("surveillance/energy/Lifetime-Global-point-tronque.csv", delimiter=";")

data = data.melt('Nsta', var_name='Configurations',  value_name='Battery lifetime (Days)')
g = sns.catplot(x="Nsta", y="Battery lifetime (Days)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of stations', ylabel='Battery lifetime (Days)')

plt.show()

data = pd.read_csv("surveillance-latency/latency-copie/Latency-Global-points-9-all-points-copie-step3.csv", delimiter=";")

data = data.melt('Nsta', var_name='Configurations',  value_name='Latency (µS)')
g = sns.catplot(x="Nsta", y="Latency (µS)", hue='Configurations', data=data, kind='point')

g.set(xlabel='Number of stations', ylabel='Latency (µS)')
g.set(ylim=(0, 6))

#plt.xticks(rotation="45")
plt.show()
