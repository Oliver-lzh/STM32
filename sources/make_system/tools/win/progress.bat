@echo off
SET /A P = 100 * %1 / %2
IF %P% LSS 10 SET P= %P%
IF %P% LSS 100 SET P= %P%
echo [%P%%%] %3