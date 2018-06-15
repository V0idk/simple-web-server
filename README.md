# Introdution
simple static web server.
* epoll(ET) + nonblocking
* thread pool
* keeplive

# build

```sh
mkdir build && cmake ..
```

# uasge

```sh
./httpsrv -p PORT -d DIR -t THREAD-NUM
```

# to to

* fastcgi
* cache
* ...