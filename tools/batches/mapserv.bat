@ECHO OFF

REM Determine the title based on the third variable
set "display_title=Pandas Map-Server"
if "%2" == "re" (
    set display_title=%display_title% - Renewal Mode
) else if "%2" == "prere" (
    set display_title=%display_title% - Pre-Renewal Mode
)
title %display_title%

CALL tools\batches\serv.bat map-server.exe Map-Server %*
