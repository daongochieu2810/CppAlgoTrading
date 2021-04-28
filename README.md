API sources: Binance (Spot + Futures + Testnet), IBKR  
Libraries used: cpprestsdk, boost, stl  
Technical indicators used: EMA, SMA, RSI, Bollinger Bands, Parabolic SAR  
Settings: Heikin Ashi Candlesticks

# Technical Indicators:

## Moving Averages (MAs):

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

## Momentum Indicators (MIs):

- MIs are used to determine the strength or weakness of a stock's price. MIs measure the rate of change of a stock's price
- MIs are far more useful during rising markets than during falling markets (historically, markets rise more often than they fall i.e bull markets tend to last longer than bear markets)

### Relative Strength Index (RSI):

- RSI measures the magnitude of recent price changes to evaluate overbought/oversold conditions in the price of a stock. RSI is displayed as an oscillator (a line graph moving between 2 extremes 0 and 100)
- Common usage: RSI having values over 70 indicates an overbought condition and a trend reversal/corrective pullback is expected; RSI below 30 indicates an oversold/undervalued condition
- Formula: 2 steps:
  - Step 1: RSI_step1 = 100 - \[100/(1 + Avg_gain/Avg_loss)\]
  - Standard value for the lookback period is 14
  - Curr_Avg_gain = Prev_Avg_gain x 13 + Curr_gain
  - Step 2: RSI_step2 = 100 - \[100/(1 + Curr_Avg_gain/Curr_Avg_loss)\]
- RSI will rise as the number and size of positive closes increase, and it will fall as the number and size of losses increase
- The 2nd part smooths the result so the RSI will only be near 100 or 0 in a strongly trending market
- Notes: RSI can stay in the overbought region for extended periods while the stock is an uptrend, the same goes for oversold and downtrend
- Overbought and oversold levels usually require modification through the usage of horizontal channel in the long-term

#### Horizontal Channel:
