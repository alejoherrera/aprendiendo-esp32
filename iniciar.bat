@echo off
rem Abre el panel de aprendizaje ESP32-CAM. No requiere instalar nada (ver ADR-0001).
rem -ExecutionPolicy Bypass aplica SOLO a este proceso: no cambia la configuracion de Windows.
title Panel ESP32-CAM
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0panel\servidor.ps1"
