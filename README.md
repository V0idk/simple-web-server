# INTRODUCTION
simple static web server.
* epoll(ET) + nonblocking
* thread pool
* keeplive

# BUILD

```sh
mkdir build && cmake ..
```

# USAGE

```sh
./httpsrv -p PORT -d DIR -t THREAD-NUM
```

# TO DO

* fastcgi
* cache
* ...


# TEST

Speed=516420 pages/min, 559455 bytes/sec.

