@ECHO OFF

set "display_title=Web-Server"
title %display_title%

CALL tools\batches\serv.bat web-server.exe Web-Server %*
