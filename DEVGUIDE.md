## Binance

[API Docs](https://binance-docs.github.io/apidocs/spot/en/)

1. Setting up API keys:

- Binance asks for 2 keys: secret key and API key, these can be obtained from the website
- For Future and Spot Testnets, the same procedure applies

2. For account-related endpoints, a signature needs to be computed and sent together as query param/body. This is done by `BotData::HMACsha256`

3. Technical Analysis (Only supporting single pairs as of Jun 11th 2021):

- Each technical indicator is computed in its own thread. These threads are then joined to be used as input for strategies. Another thread is used to pull candlestick data from Binance, which will be joined after the strategy is done calculating

4. Profiling:

- The project uses [gperftools](https://github.com/gperftools/gperftools) to profile performance. `set_env_vars` is used to customize the profiler

- `benchmarkPerformance` is also used to output time taken for the input method to run. For now I use this more often than gperftools

5. Notes:

- In case you want to use a callback, using lambdas is said to be more perfomant (examples can be found in `Binance.cpp`)

## IBKR

- IBKR is different from Binance that it needs [TWS](https://interactivebrokers.github.io/tws-api/introduction.html) to receive commands through sockets, so the implementation can be expected to be quite different in terms of getting candlestick data, but similar in terms of technical analysis and trading strategy

### Current strategies:

1. Momentum Trading Strategy (Taken from Alpaca's example):

This algorithm may buy stocks during a 45 minute period each day, starting 15 minutes after market open. (The first 15 minutes are avoided, as the high volatility can lead to poor performance.) The signals it uses to determine if it should buy a stock are if it has gained 4% from the previous day's close, is above the highest point seen during the first 15 minutes of trading, and if its MACD is positive and increasing. (It also checks that the volume is strong enough to make trades on reliably.) It sells when a stock drops to a stop loss level or increases to a target price level. If there are open positions in your account at the end of the day on a symbol the script is watching, those positions will be liquidated at market. (Potentially holding positions overnight goes against the concept of risk that the algorithm uses, and must be avoided for it to be effective.)

2. Scalping Strategy with Heikin-Ashi + Parabolic SAR:
