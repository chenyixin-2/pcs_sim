apt install ntpdate
ntpdate cn.pool.ntp.org
timedatectl set-timezone Asia/Shanghai   
mkdir ../log/
touch -a ../log/sta.log
chown $USER:$USER ../log/ctn.log
rm -f /dev/shm/sem.ctn_sem_p*
rm -f /tmp/ctnlog.lock
chmod 777 /dev/ttymxc*
mkdir /tmp/crash/