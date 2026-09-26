| row | R21 b00 | R21 b01 | R21 b02 | R22 b00 | R22 b01 | R22 b02 | R22 mean | Δ mean | share R22 mean |
|---|---|---|---|---|---|---|---|---|---|
| 1 tower window static(5)+A(5) | 412.4 | 397.7 | 435.8 | 414.7 | 387.5 | 431.8 | 411.3 | -4.0 | 21.2 % |
| 2 q3/q4 host glue (untimed + host in calls) | 308.7 | 291.2 | 314.0 | 309.0 | 286.0 | 307.9 | 301.0 | -3.7 | 15.5 % |
| 3 front q3/q4 | 285.1 | 236.9 | 282.2 | 288.2 | 237.6 | 256.0 | 260.6 | -7.5 | 13.4 % |
| 4 census | 213.7 | 207.4 | 231.9 | 209.8 | 207.3 | 229.1 | 215.4 | -2.3 | 11.1 % |
| 5 device time of the 3 calls (kern+xfer) | 462.7 | 375.5 | 453.6 | 413.3 | 330.0 | 397.4 | 380.3 | -50.3 | 19.6 % |
| 6 tower tail (pop+img+bank+enc) | 147.8 | 140.3 | 165.1 | 142.5 | 142.9 | 177.5 | 154.3 | +3.2 | 7.9 % |
| 7 validation | 95.6 | 94.0 | 107.5 | 69.7 | 74.1 | 79.6 | 74.5 | -24.5 | 3.8 % |
| 8 merge+gen_index+tower_index+prepare+residuals | 168.3 | 170.7 | 190.3 | 144.0 | 144.6 | 148.1 | 145.6 | -30.9 | 7.5 % |
R21 sum rows [2094.5, 1913.7, 2180.3] chain [2094.5, 1913.7, 2180.3]
R22 sum rows [1991.2, 1810.0, 2027.5] chain [1991.2, 1810.0, 2027.5]
24 front 288.2 max_job 222.7 job_sum/48 150.8 glue 177.6 host_in_calls 131.4 filter_host 49.2 cert_host 36.6 lanes_host 45.6 lanes_convert 14.5 kern 387.8 xfer 25.5 q2cen 168.2 q2own 177.0
28 front 237.6 max_job 173.4 job_sum/48 137.5 glue 161.1 host_in_calls 124.8 filter_host 45.1 cert_host 35.3 lanes_host 44.4 lanes_convert 13.9 kern 306.6 xfer 23.5 q2cen 168.2 q2own 140.6
32 front 256.0 max_job 169.5 job_sum/48 156.5 glue 177.1 host_in_calls 130.9 filter_host 48.4 cert_host 36.3 lanes_host 46.2 lanes_convert 15.1 kern 371.2 xfer 26.3 q2cen 173.0 q2own 160.9
