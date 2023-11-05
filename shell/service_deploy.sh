./service_remove.sh
cp ../sta.service /etc/systemd/system/sta.service
chmod a+rwx /etc/systemd/system/sta.service
systemctl daemon-reload
systemctl disable sta
systemctl enable sta
systemctl start sta
