# nohup socat -v tcp-listen:12345,fork,reuseaddr,nodelay tcp:192.168.0.137:22 &
# rsync -e `ssh -p 60$2` -avR --delete --progress bin/ ./bin data/ ./data/ shell ./shell/ cfg/ ./cfg/ root@10.8.0.$1:/program/ctnd/
cd ../
rsync -avzRW --update --progress bin/ ./bin shell ./shell/ cfg/ ./cfg/ root@10.8.0.$1:/program/sta/