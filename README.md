# Notes:

- [Dev guide](https://github.com/daongochieu2810/CppAlgoTrading/blob/master/DEVGUIDE.md)
- API sources: Binance (Spot + Futures + Testnet), IBKR
- Libraries used: cpprestsdk, boost, stl, gperftools
- Technical indicators used: EMA, SMA, RSI, Bollinger Bands, Parabolic SAR
- Settings: Heikin Ashi, Candlesticks

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
  - Curr_Avg_gain = (Prev_Avg_gain x 13 + Curr_gain)/14
  - Step 2: RSI_step2 = 100 - \[100/(1 + Curr_Avg_gain/Curr_Avg_loss)\]
- RSI will rise as the number and size of positive closes increase, and it will fall as the number and size of losses increase
- The 2nd part smooths the result so the RSI will only be near 100 or 0 in a strongly trending market
- Notes: RSI can stay in the overbought region for extended periods while the stock is an uptrend, the same goes for oversold and downtrend
- Overbought and oversold levels usually do not require modification through the usage of horizontal channel in the long-term
- During an uptrend, RSI tends to stay above 30 and should frequently hit 70; during a downtrend, it is rare to see the RSI exceed 70, and it should frequently hits <= 30 e.g if the RSI isnt able to reach 70 on a number of price swings during an uptrend, but then drop below 30 -> the trend has weakened and could be reversing lower
- The above is true for a downtrend. If the downtrend is unable to reach 30 or below and then rallies above 70, that downtrend has weakened and could be reversing to the upside (trendlines + MAs can be used together with RSI to reinforce this pattern)

#### RSI Divergences:

- A bullish divergence occurs when RSI creates an oversold reading followed by a higher low (in RSI) that matches correspondingly lower lows in the stock's price -> this indicates rising bullish momentum, and a break above oversold territory could be used to trigger a new long position
- A bearish divergence occurs when the RSI creates an overbought reading followed by a lower high that matches corresponding higher highs in the price
- Divergences can be rare when a stock is in a stable long-term trend

#### RSI Swing Rejections:

- RSI's behaviours when it re-emerges from overbought/oversold conditions -> this is called swing rejection. It has 4 parts:
  | Bullish | Bearish |
  | --------|---------|
  | RSI falls into oversold territory | RSI rises into the overbought territory |
  | RSI crosses back above 30% | RSI crosses back below 70% |
  | RSI forms another dip without crossing back into oversold territory | RSI forms another high without crossing back into overbought territory |
  | RSI then breaks its most recent high | RSI then breaks its most recent low |
- Bearish signals during downward trends are less likely to generate false alarms

###### RSI is most useful in an oscillating market where the price is alternating between bullish and bearish movements

### Moving Averages Covergence Divergence (MACD):

- MACD shows the relationship between 2 MAs. 9-day EMA of MACD is called the `signal line`, which is plotted on top of the MACD line -> functions as a trigger for buy/sell signals e.g buy when MACD crosses above the signal line and sell/short when it goes below the signal line
- Crossovers are more reliable when they conform to the prevailing trend
- If the MACD crosses above its signal line following a brief correction within a longer-term uptrend, it qualifies as a bullish confirmation
- If the MACD crosses below its signal line following a brief move higher within a longer-term downtrend, it qualifies as a bearish confirmation
- Formula: MACD = 12-day EMA - 26-day EMA
- The more distant MACD is above/below its baseline the more distant the 2 MAs are from each other
- MACD is often displayed with a histogram showing the distance between MACD and its signal line. If MACD is above the signal line, the histogram will be above MACD's baseline -> this can be used to identify when bullish/bearish momentum is high
- Both MACD and RSI can signal an upcoming trend change by showing divergence from price. However these signals can be false positives, and they often occur when the price of an asset moves sidewat (e.g in a range or triangle pattern following a trend). A slowdown in the momentum (sideways momentum or slow trending movement) of the price will cause the MACD to pull away from its prior extremes and gravitate towards the 0 lines even in the absence of a true reversal

#### MACD Divergences:

- A bullish divergence appears when the MACD forms 2 rising lows that correspond with 2 falling lows on the price. This is a valid bullish signal when the long-term trend is still positive (less reliable otherwise)
- A bearish divergence appears when the MACD forms 2 falling highs that corresponds to 2 rising highs on the price. A bearish divergence that appears during a long-term bearish trend is considered a confirmation that the trend is likely to continue

#### MACD Rapid Rises/Falls:

- When the MACD rises/falls rapidly, it is a signal that the security is overbought/oversold and will soon return to normal levels (this should be combined with RSI for more reliable predictions)

## Trend Indicators:

### Parabolic Stop and Reverse (Parabolic SAR):

- Parabolic SAR is used to determine the price direction of an asset & draw attention to when the price direction is changing
- Formula:

  - Uptrend: RSAR = Prev PSAR + Prior AF(Prior EP - Prior PSAR)
  - Downtrend: FPSAR = Prev PSAR - Prior AF(Prior EP - Prior PSAR)
  - AF: Acceleration Factor: starts at 0.02 and increases by 0.02, up to a max of 0.2, each time the extreme point makes a new low (falling SAR - FSAR) or high (rising SAR - RSAR)
  - EP: Extreme Point: the lowest low in the current downtrend or the highest high in the current uptrend
  - Notes:
    - If the SAR is initially rising and the price as a close below the rising SAR value, then the trend is now down and the FSAR formula will be used. If the price rises above the FSAR, then switch to the rising formula
    - Monitor prices for at least 5 periods, recording the EPs
    - If the price is rising, use the lowest low of the periods above as the initial PSAR value; if the price is falling, use the highest high as the initial PSAR value

- A dot below the price is considered a bullish signal
- When the dot flips, it indicates a potential change in price
- Stochastic + MA + ADX can complement SAR to avoid false signals
- SAR sell signals are more convincing when the price is trading below a long-term MA. This suggests that the sellers are in control of the direction and that the recent SAR sell signal could be the beginning of another way lower
- Similarly, if the price is above the MA, focus on taking the buy signals
- SAR performs best in markets with a steady trend. In ranging markets, the SAR tends to whipsaw back and forth, generating false trading signals

##### The parabolic SAR is used to gauge a stock's direction and for placing stop-loss orders

# Heikin-Ashi:

- Heikin-Ashi is useful to make charts more readable and make trends easier to analyze
- Formula:
  - Close = 0.25(OldOpen + OldHigh + OldLow + OldClose)
  - Open = 0.5(OldOpen + OldClose)
  - High = max(OldHigh, OldOpen, OldClose)
  - Low = min(OldLow, OldOpen, OldClose)
- 5 primary signals:
  - Hollow/green candles with no lower shadows indicate a strong uptrend i.e Low = Open
  - Hollow/green candles signify an uptrend (less reliable than the above)
  - Candles with a small body surrounded by upper and lower shadows indicate a trend change i.e Close and High are close
  - Filled/red candles indicate a downtrend
  - Filled/red candles indicate a downtrend with no higher shadows identify a strong downtrend (more reliable than the above)
