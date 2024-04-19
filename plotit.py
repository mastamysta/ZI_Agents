import matplotlib.pyplot as plt
import pandas as pd
import numpy

df = pd.read_csv('example.csv')
print(df)

asks = df.iloc[:,0]
bids = df.iloc[:,1]

print(asks)

asks.plot()
bids.plot()

plt.show()
