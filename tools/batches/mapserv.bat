@ECHO OFF

set "display_title=Pandas Map-Server"
title %display_title%

CALL tools\batches\serv.bat map-server.exe Map-Server %*
