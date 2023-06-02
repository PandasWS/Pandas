@ECHO OFF

set "display_title=Generate-Navi-Files"
title %display_title%

map-server-generator.exe /generate-navi
ECHO.
pause