# [Report](../master/FinalReport.pdf)
# Compiling the code
``` make ```

# Starting the server
``` ./Server <directory name> <port> ```

# Requesting content
Open any browser and enter the path name of the file taking the above <directory name> as the base path, also include the <port>
``` <path>:<port> ```

# Sample files
For example use the [sample_webpage](../master/sample_webpage)

# Test Example

``` sudo httperf --server 127.0.0.1 --port 8080 --num-calls 1 --num-conn 10000 --rate 10000 --send-buffer 1024 --recv-buffer 1024 --uri /sample_webpage/index.html ```

## Result
``` Total: connections 8649 requests 8649 replies 8649 test-duration 1.877 s

Connection rate: 4607.5 conn/s (0.2 ms/conn, <=1022 concurrent connections)
Connection time [ms]: min 0.2 avg 137.0 max 1869.4 median 24.5 stddev 331.0
Connection time [ms]: connect 89.6
Connection length [replies/conn]: 1.000
```
