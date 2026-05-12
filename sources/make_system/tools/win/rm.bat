@echo off
SET file=%1
del /F /Q %file:/=\% 2>nul