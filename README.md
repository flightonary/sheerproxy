** This is an experimental project for study **

# sheerproxy

Sheerproxy is a preload library for proxying system-wide.

- **Robust and quiet**: Sheerproxy is designed for proxying system-wide on the operating system. It is took care to ensure that applications do not crash and the operating system does not become unstable. It does not output any logs to stdout or stderr.
- **IPv6 support**: Sheerproxy supports IPv6 completely for IPv6 adoption.

## Limitation
Sheerproxy uses LD_PRELOAD trick and that hooks network-related libc functions. It only works for dynamically linked programs.

## Installation
Please compile and install the library. /etc/ld.so.preload will be configured automatically.
```
./configure
sudo make install
```

## Usage
At least one proxy have to be added into /etc/sheerproxy.conf.
```
http_proxy   squid.example.com   3128
```

By default enable localnet for loopback, linklocal and private address ranges but not for domains and hosts. Please change /etc/sheerproxy.conf to avoid proxying for the private network domain and hosts like below;
```
always_direct   local.example.com
always_direct   certain.host.example.com
```

## Uninstsall
It's easy to uninstall.
```
sudo make uninstall
```

## Development
Start the development environment.
```
cd ./devenv/
docker-compose up -d
```

Run all the tests.
```
./tools/run-on-centos.sh ./tools/run-tests.sh
```


## Acknowledgement
Sheerproxy is inspired by proxychains-ng.

## License
[GPLv3](./LICENSE)
