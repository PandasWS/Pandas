@ECHO OFF

set "display_title=Login-Server"
title %display_title%

CALL tools\batches\serv.bat login-server.exe Login-Server %*
