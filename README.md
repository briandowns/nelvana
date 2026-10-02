# nelvana

Update `/etc/ssh/sshd_config`

```
Match User nelvana
    AuthorizedKeysCommand /usr/local/libexec/nelvana-host-keys
    AuthorizedKeysCommandUser root

    ForceCommand /usr/local/libexec/nelvana-host
    PermitTTY yes

    AllowTcpForwarding no
    AllowAgentForwarding no
    X11Forwarding no
```
