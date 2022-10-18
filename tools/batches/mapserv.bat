@ECHO OFF

set "display_title=Map-Server"
title %display_title%

CALL tools\batches\serv.bat map-server.exe Map-Server %*
