nohup valgrind --leak-check=full --leak-resolution=high --show-leak-kinds=all --track-origins=yes --log-file=/tmp/sta_valgrind.log --verbose ../bin/stad -t appl -d &