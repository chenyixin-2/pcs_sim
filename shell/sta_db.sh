mv ../cfg/sta.db ../cfg/sta.db.bak
sqlite3 ../cfg/sta.db < ../cfg/sta_$1.sql
