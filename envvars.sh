#!/bin/sh

echo "NELVANA_USER_ID=${NELVANA_USER_ID:-<NOT SET>}"
echo "USER=${USER:-<NOT SET>}"
echo "SSH_USER=${SSH_USER:-<NOT SET>}"
echo "SSH_CONNECTION=${SSH_CONNECTION:-<NOT SET>}"

env

exit 0

