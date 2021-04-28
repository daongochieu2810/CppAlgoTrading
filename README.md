API sources: Binance (Spot + Futures + Testnet), IBKR  
Libraries used: cpprestsdk, boost, stl  
Technical indicators used: EMA, SMA, RSI, Bollinger Bands, Parabolic SAR  
Settings: Heikin Ashi Candlesticks

# Technical Indicators:

## Moving Averages (MA):

- All MA commonly used are lagging indicators
- MAs should only be used to confirm a market move or to indicate ts strength. By the time a MA shows trend changes, the optimal time to enter the trade has already passed
- MAs are suitable for trending markets. The direction of MAs and the rate of changes from 1 candlestick to the next can be considered to predict future trends

### SMA (Simple Moving Average):

- A simple moving average (SMA) calculates the average of a selected range of prices, usually closing prices, by the number of periods in that range
- SMA smooths out volatility, makes it easier to view the price trend
- A shortet-term SMA is more volatile but its reading is closer to the source data
- Considerations:
  - Simple usage: to quickly identify a trend
  - Advance usage: to compare a pair of SMA (each covering diff timeframes). If a shorter-term SMA is above a longer-term SMA, an uptrend is expected, and the reverse for a downtrend
  - Popular patterns:
    - The death cross: 50-day SMA falls below 200-day SMA -> bearish signal
    - The golden cross: shorter-term SMA breaks above a long-term SMA -> bullish signal (can be reinforced by high volumes)

### EMA (Exponential Moving Average):

- EMA places more weight on the most recent data compared to SMA
- Common timeframes for EMA:
  - Short-term: 12-day, 26-day, 8-day, 20-day
  - Long-term: 50-day, 200-day
  - 12 and 26-day EMA are often used to create Moving Average Convergence Divergence (MACD) and Percentage Price Oscillator (PPO)
  - In general, when a stock price crosses its 200-day EMA, it is a technical signal that a reversal has occurred
- Formula: EMA_today = EMA_yesterday + (Value_today - EMA_yesterday) \* (Smoothing/(1 + Days))
- Most common choice for Smoothing is 2. The significance of recent data points increases proportionally wrt this factor
- EMA reacts to changes faster than SMA -> it alleviates the negative impact of lags to some extent -> more applicable to fast-moving markets
