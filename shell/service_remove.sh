systemctl stop sta
systemctl disable sta
rm /etc/systemd/system/sta.service
rm /etc/systemd/system/multi-user.target.wants/sta.service
rm /etc/systemd/system/default.target.wants/sta.service
systemctl daemon-reload
systemctl reset-failed
systemctl status sta
