/testing/guestbin/swan-prep
ipsec start
/testing/pluto/bin/wait-until-pluto-started
ipsec whack --impair send-bogus-sa-init-payload
ipsec auto --add westnet-eastnet-ipv4-psk-ikev2
echo "initdone"
