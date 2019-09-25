@ECHO OFF

REM Determine the title based on the third variable
set "display_title=Pandas Login-Server"
if "%2" == "re" (
    set display_title=%display_title% - Renewal Mode
) else if "%2" == "prere" (
    set display_title=%display_title% - Pre-Renewal Mode
)
title %display_title%

CALL tools\batches\serv.bat login-server.exe Login-Server %*
