set user=ragnarok 
set password=90opKLM. 
set host_name=localhost
set db_name=ro_pandas_big5
for %%f in (%~dp0*.sql) do (   
   echo Processing %%f file...
   mysql -u %user% -p%password% -D %db_name% -h %host_name%  < %%f   
)
PAUSE