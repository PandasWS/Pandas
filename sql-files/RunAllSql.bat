:: 請修改成自己環境的MYSQL的帳號
set user=mysql_account
:: 請修改成自己環境的MYSQL的密碼
set password=mysql_password
:: 請修改成自己環境的MYSQL的連線資訊(本機為127.0.0.1或是localhost)
set host_name=localhost
:: db的名稱，建議將資料表集中成一個db比較好下sql指令，但若是希望分開DB，可以將需要分開放的SQL資料表分資料夾，再將此檔案複製貼到不同的資料夾，修改下面的資料庫名稱，執行即可
set db_name=ro_pandas_big5
for %%f in (%~dp0*.sql) do (   
   echo 正在執行 %%f file...
   @ mysql -u %user% -p%password% -D %db_name% -h %host_name%  < %%f   
)
PAUSE