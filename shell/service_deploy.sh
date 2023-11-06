./service_remove.sh
if [ -f "../cfg/dev.db" ];
then
    echo "dev.db exist"
else
    ./dev_db.sh
    echo "build dev.db"
fi
./prepare.sh
./sta_db.sh $1
./proj_db.sh $1
cp ./sta.service /etc/systemd/system/sta.service
chmod a+rwx /etc/systemd/system/sta.service
rm timestamp.out
systemctl daemon-reload
systemctl disable sta
systemctl enable sta
systemctl start sta