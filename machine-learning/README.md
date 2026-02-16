# Data Logging
Make sure device is connected. Run this line to log data:
```
JLinkRTTLogger -Device NRF54L15_M33 -If SWD -Speed 4000 -RTTChannel 0 ./machine-learning/rtt_log_$(date +%Y%m%d_%H%M%S).csv
```
Remove any header lines, if needed.