gdb -cd=./ -directory=../ -ex 'set detach-on-fork off' -ex 'set print elements 0' ../bin/sta /tmp/crash/$(ls /tmp/crash/ -Art | tail -n 1)