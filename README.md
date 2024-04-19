Zero-Intelligence Agent-Based Modelling
 
This repo aims to implement the model laid out by Farmer et al. in their 2003 whitepaper on zero-intelligence agent-based models for double auction systems: https://sites.santafe.edu/~jdf/papers/zero.pdf. The key assumption is that market dynamics including price variance, price diffusion rate variance and market impact can be described by agents with no strategy. Rather, the behaviour of the zero-intelligence agents in this model can be described purely probablistically.

This model implements two kinds of agents:
 Impatient agents - place market orders according to a Poisson process.
 Patient agents - place limit orders according to a Poisson process. The price is selected from a uniform distribution but is no better than the current best ask/best bid. Cancel limit orders according to a Poisson process.

I was pleased to get some decent results immediately: 

price (Y) agaist time delta (X)![bland_market_data](https://github.com/mastamysta/ZI_Agents/assets/47383446/5d9e1b17-d11a-49ff-805f-666a4ed4873b)

It was also possible to introduce new agents part way through the experiment. In the below example, a new seller is introduced at time delta 500. The supply-side surpluss drives prices down rapidly over the next 500 deltas:

price (Y) agaist time delta (X)![new_seller](https://github.com/mastamysta/ZI_Agents/assets/47383446/1ae155c8-1714-4e4d-89b4-4def03bd026a)
