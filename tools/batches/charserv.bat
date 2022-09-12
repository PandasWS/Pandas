@ECHO OFF

set "display_title=Char-Server"
title %display_title%

CALL tools\batches\serv.bat char-server.exe Char-Server %*
