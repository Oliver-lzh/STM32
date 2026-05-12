@echo off
SET src=%1
SET dst=%2
copy /Y %src:/=\% %dst:/=\% >nul