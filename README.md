# nelvana

Nelvana is a system to allow for configured users to SSH into a server and be placed into a preconfigured OCI container. 

## Build

```sh
make nelvana-keys
make nelvana-exec
make nelvana-server
```

## System Configuration

Create a new user called `container` and add it to the `wheel` group.

```sh
pw useradd -n username -m -s /usr/local/bin/bash -G wheel -c "User Real Name"
```

Update `/etc/ssh/sshd_config`

```sh
Match User container
    AuthorizedKeysCommand /usr/local/libexec/nelvana-keys %f
    AuthorizedKeysCommandUser root
    PermitTTY yes
    AllowTcpForwarding no
    AllowAgentForwarding no
    X11Forwarding no
    PasswordAuthentication no
    KbdInteractiveAuthentication no
```

## Contributing

Please feel free to open a PR!

## Contact

Brian Downs [@bdowns328](http://twitter.com/bdowns328)

## License

BSD 2 Clause [License](/LICENSE).

