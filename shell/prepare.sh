# apt install ntpdate
# ntpdate cn.pool.ntp.org
timedatectl set-timezone Asia/Shanghai   
mkdir ../log/
touch -a ../log/sta.log
chown $USER:$USER ../log/sta.log
rm -f /dev/shm/sem.sta_sem_p*
rm -f /tmp/stalog.lock
chmod 777 /dev/ttymxc*
mkdir /tmp/crash/

ifconfig can0 down
ip link set can0 type can bitrate 250000
ifconfig can0 up