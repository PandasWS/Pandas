mysql -uroot -p$MYSQL_ROOT_PASSWORD -e"CREATE DATABASE ragnarok;"
# mysql -uroot -p$MYSQL_ROOT_PASSWORD -e"CREATE DATABASE logs;"
mysql -uroot -p$MYSQL_ROOT_PASSWORD ragnarok < /sql-files/main.sql
mysql -uroot -p$MYSQL_ROOT_PASSWORD ragnarok < /sql-files/logs.sql
mysql -uroot -p$MYSQL_ROOT_PASSWORD -e"CREATE USER 'ragnarok'@'%' IDENTIFIED BY 'ragnarok';"
mysql -uroot -p$MYSQL_ROOT_PASSWORD -e"grant all privileges on *.* to 'ragnarok'@'%' with grant option;"
mysql -uroot -p$MYSQL_ROOT_PASSWORD -e"flush privileges;"
