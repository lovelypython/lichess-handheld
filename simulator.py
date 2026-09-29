#!/usr/bin/env python3
import os, sys, json, queue, threading, io, base64, time, subprocess, re
from urllib.parse import urlsplit
import requests
import pygame
import chess
import chess.pgn

BASE = "https://lichess.org"
TOKEN = os.environ.get("LICHESS_TOKEN", "").strip()

W, H = 480, 320
BOARD = 320
SQ = 40
SIDE_X = 320

BG = (25, 27, 31)
PANEL = (37, 39, 44)
BTN = (65, 69, 78)
BTN_ON = (104, 135, 78)
TEXT = (245, 245, 245)
MUTED = (184, 184, 184)
LIGHT = (238, 238, 210)
DARK = (118, 150, 86)
LAST = (205, 210, 106)
SEL = (246, 246, 105)
HINT = (255, 183, 64)
GOOD = (102, 187, 106)
BAD = (239, 83, 80)

# Cburnett chess pieces, embedded directly in this file.
# Source: ndg6/staunton copy of Cburnett pieces; artwork is multi-licensed.
# We use the BSD-3-Clause option for the artwork. See NOTICE.txt in this package.
PIECE_B64 = {
"wp": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PHBhdGggZD0iTTIyLjUgOWMtMi4yMSAwLTQgMS43OS00IDQgMCAuODkuMjkgMS43MS43OCAyLjM4QzE3LjMzIDE2LjUgMTYgMTguNTkgMTYgMjFjMCAyLjAzLjk0IDMuODQgMi40MSA1LjAzLTMgMS4wNi03LjQxIDUuNTUtNy40MSAxMy40N2gyM2MwLTcuOTItNC40MS0xMi40MS03LjQxLTEzLjQ3IDEuNDctMS4xOSAyLjQxLTMgMi40MS01LjAzIDAtMi40MS0xLjMzLTQuNS0zLjI4LTUuNjIuNDktLjY3Ljc4LTEuNDkuNzgtMi4zOCAwLTIuMjEtMS43OS00LTQtNHoiIGZpbGw9IiNmZmYiIHN0cm9rZT0iIzAwMCIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPjwvc3ZnPg==""",
"wn": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0ibm9uZSIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxwYXRoIGQ9Ik0yMiAxMGMxMC41IDEgMTYuNSA4IDE2IDI5SDE1YzAtOSAxMC02LjUgOC0yMSIgZmlsbD0iI2ZmZiIvPjxwYXRoIGQ9Ik0yNCAxOGMuMzggMi45MS01LjU1IDcuMzctOCA5LTMgMi0yLjgyIDQuMzQtNSA0LTEuMDQyLS45NCAxLjQxLTMuMDQgMC0zLTEgMCAuMTkgMS4yMy0xIDItMSAwLTQuMDAzIDEtNC00IDAtMiA2LTEyIDYtMTJzMS44OS0xLjkgMi0zLjVjLS43My0uOTk0LS41LTItLjUtMyAxLTEgMyAyLjUgMyAyLjVoMnMuNzgtMS45OTIgMi41LTNjMSAwIDEgMyAxIDMiIGZpbGw9IiNmZmYiLz48cGF0aCBkPSJNOS41IDI1LjVhLjUuNSAwIDEgMS0xIDAgLjUuNSAwIDEgMSAxIDB6bTUuNDMzLTkuNzVhLjUgMS41IDMwIDEgMS0uODY2LS41LjUgMS41IDMwIDEgMSAuODY2LjV6IiBmaWxsPSIjMDAwIi8+PC9nPjwvc3ZnPg==""",
"wb": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0ibm9uZSIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxnIGZpbGw9IiNmZmYiIHN0cm9rZS1saW5lY2FwPSJidXR0Ij48cGF0aCBkPSJNOSAzNmMzLjM5LS45NyAxMC4xMS40MyAxMy41LTIgMy4zOSAyLjQzIDEwLjExIDEuMDMgMTMuNSAyIDAgMCAxLjY1LjU0IDMgMi0uNjguOTctMS42NS45OS0zIC41LTMuMzktLjk3LTEwLjExLjQ2LTEzLjUtMS0zLjM5IDEuNDYtMTAuMTEuMDMtMTMuNSAxLTEuMzU0LjQ5LTIuMzIzLjQ3LTMtLjUgMS4zNTQtMS45NCAzLTIgMy0yeiIvPjxwYXRoIGQ9Ik0xNSAzMmMyLjUgMi41IDEyLjUgMi41IDE1IDAgLjUtMS41IDAtMiAwLTIgMC0yLjUtMi41LTQtMi41LTQgNS41LTEuNSA2LTExLjUtNS0xNS41LTExIDQtMTAuNSAxNC01IDE1LjUgMCAwLTIuNSAxLjUtMi41IDQgMCAwLS41LjUgMCAyeiIvPjxwYXRoIGQ9Ik0yNSA4YTIuNSAyLjUgMCAxIDEtNSAwIDIuNSAyLjUgMCAxIDEgNSAweiIvPjwvZz48cGF0aCBkPSJNMTcuNSAyNmgxME0xNSAzMGgxNW0tNy41LTE0LjV2NU0yMCAxOGg1IiBzdHJva2UtbGluZWpvaW49Im1pdGVyIi8+PC9nPjwvc3ZnPg==""",
"wr": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0iI2ZmZiIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxwYXRoIGQ9Ik05IDM5aDI3di0zSDl2M3ptMy0zdi00aDIxdjRIMTJ6bS0xLTIyVjloNHYyaDVWOWg1djJoNVY5aDR2NSIgc3Ryb2tlLWxpbmVjYXA9ImJ1dHQiLz48cGF0aCBkPSJNMzQgMTRsLTMgM0gxNGwtMy0zIi8+PHBhdGggZD0iTTMxIDE3djEyLjVIMTRWMTciIHN0cm9rZS1saW5lY2FwPSJidXR0IiBzdHJva2UtbGluZWpvaW49Im1pdGVyIi8+PHBhdGggZD0iTTMxIDI5LjVsMS41IDIuNWgtMjBsMS41LTIuNSIvPjxwYXRoIGQ9Ik0xMSAxNGgyMyIgZmlsbD0ibm9uZSIgc3Ryb2tlLWxpbmVqb2luPSJtaXRlciIvPjwvZz48L3N2Zz4=""",
"wq": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0iI2ZmZiIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxwYXRoIGQ9Ik04IDEyYTIgMiAwIDEgMS00IDAgMiAyIDAgMSAxIDQgMHptMTYuNS00LjVhMiAyIDAgMSAxLTQgMCAyIDIgMCAxIDEgNCAwek00MSAxMmEyIDIgMCAxIDEtNCAwIDIgMiAwIDEgMSA0IDB6TTE2IDguNWEyIDIgMCAxIDEtNCAwIDIgMiAwIDEgMSA0IDB6TTMzIDlhMiAyIDAgMSAxLTQgMCAyIDIgMCAxIDEgNCAweiIvPjxwYXRoIGQ9Ik05IDI2YzguNS0xLjUgMjEtMS41IDI3IDBsMi0xMi03IDExVjExbC01LjUgMTMuNS0zLTE1LTMgMTUtNS41LTE0VjI1TDcgMTRsMiAxMnoiIHN0cm9rZS1saW5lY2FwPSJidXR0Ii8+PHBhdGggZD0iTTkgMjZjMCAyIDEuNSAyIDIuNSA0IDEgMS41IDEgMSAuNSAzLjUtMS41IDEtMS41IDIuNS0xLjUgMi41LTEuNSAxLjUuNSAyLjUuNSAyLjUgNi41IDEgMTYuNSAxIDIzIDAgMCAwIDEuNS0xIDAtMi41IDAgMCAuNS0xLjUtMS0yLjUtLjUtMi41LS41LTIgLjUtMy41IDEtMiAyLjUtMiAyLjUtNC04LjUtMS41LTE4LjUtMS41LTI3IDB6IiBzdHJva2UtbGluZWNhcD0iYnV0dCIvPjxwYXRoIGQ9Ik0xMS41IDMwYzMuNS0xIDE4LjUtMSAyMiAwTTEyIDMzLjVjNi0xIDE1LTEgMjEgMCIgZmlsbD0ibm9uZSIvPjwvZz48L3N2Zz4=""",
"wk": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0ibm9uZSIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxwYXRoIGQ9Ik0yMi41IDExLjYzVjZNMjAgOGg1IiBzdHJva2UtbGluZWpvaW49Im1pdGVyIi8+PHBhdGggZD0iTTIyLjUgMjVzNC41LTcuNSAzLTEwLjVjMCAwLTEtMi41LTMtMi41cy0zIDIuNS0zIDIuNWMtMS41IDMgMyAxMC41IDMgMTAuNSIgZmlsbD0iI2ZmZiIgc3Ryb2tlLWxpbmVjYXA9ImJ1dHQiIHN0cm9rZS1saW5lam9pbj0ibWl0ZXIiLz48cGF0aCBkPSJNMTEuNSAzN2M1LjUgMy41IDE1LjUgMy41IDIxIDB2LTdzOS00LjUgNi0xMC41Yy00LTYuNS0xMy41LTMuNS0xNiA0VjI3di0zLjVjLTMuNS03LjUtMTMtMTAuNS0xNi00LTMgNiA1IDEwIDUgMTBWMzd6IiBmaWxsPSIjZmZmIi8+PHBhdGggZD0iTTExLjUgMzBjNS41LTMgMTUuNS0zIDIxIDBtLTIxIDMuNWM1LjUtMyAxNS41LTMgMjEgMG0tMjEgMy41YzUuNS0zIDE1LjUtMyAyMiAwIi8+PC9nPjwvc3ZnPg==""",
"bp": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PHBhdGggZD0iTTIyLjUgOWMtMi4yMSAwLTQgMS43OS00IDQgMCAuODkuMjkgMS43MS43OCAyLjM4QzE3LjMzIDE2LjUgMTYgMTguNTkgMTYgMjFjMCAyLjAzLjk0IDMuODQgMi40MSA1LjAzLTMgMS4wNi03LjQxIDUuNTUtNy40MSAxMy40N2gyM2MwLTcuOTItNC40MS0xMi40MS03LjQxLTEzLjQ3IDEuNDctMS4xOSAyLjQxLTMgMi40MS01LjAzIDAtMi40MS0xLjMzLTQuNS0zLjI4LTUuNjIuNDktLjY3Ljc4LTEuNDkuNzgtMi4zOCAwLTIuMjEtMS43OS00LTQtNHoiIHN0cm9rZT0iIzAwMCIgc3Ryb2tlLXdpZHRoPSIxLjUiIHN0cm9rZS1saW5lY2FwPSJyb3VuZCIvPjwvc3ZnPg==""",
"bn": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0ibm9uZSIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxwYXRoIGQ9Ik0yMiAxMGMxMC41IDEgMTYuNSA4IDE2IDI5SDE1YzAtOSAxMC02LjUgOC0yMSIgZmlsbD0iIzAwMCIvPjxwYXRoIGQ9Ik0yNCAxOGMuMzggMi45MS01LjU1IDcuMzctOCA5LTMgMi0yLjgyIDQuMzQtNSA0LTEuMDQyLS45NCAxLjQxLTMuMDQgMC0zLTEgMCAuMTkgMS4yMy0xIDItMSAwLTQuMDAzIDEtNC00IDAtMiA2LTEyIDYtMTJzMS44OS0xLjkgMi0zLjVjLS43My0uOTk0LS41LTItLjUtMyAxLTEgMyAyLjUgMyAyLjVoMnMuNzgtMS45OTIgMi41LTNjMSAwIDEgMyAxIDMiIGZpbGw9IiMwMDAiLz48cGF0aCBkPSJNOS41IDI1LjVhLjUuNSAwIDEgMS0xIDAgLjUuNSAwIDEgMSAxIDB6bTUuNDMzLTkuNzVhLjUgMS41IDMwIDEgMS0uODY2LS41LjUgMS41IDMwIDEgMSAuODY2LjV6IiBmaWxsPSIjZWNlY2VjIiBzdHJva2U9IiNlY2VjZWMiLz48cGF0aCBkPSJNMjQuNTUgMTAuNGwtLjQ1IDEuNDUuNS4xNWMzLjE1IDEgNS42NSAyLjQ5IDcuOSA2Ljc1UzM1Ljc1IDI5LjA2IDM1LjI1IDM5bC0uMDUuNWgyLjI1bC4wNS0uNWMuNS0xMC4wNi0uODgtMTYuODUtMy4yNS0yMS4zNC0yLjM3LTQuNDktNS43OS02LjY0LTkuMTktNy4xNmwtLjUxLS4xeiIgZmlsbD0iI2VjZWNlYyIgc3Ryb2tlPSJub25lIi8+PC9nPjwvc3ZnPg==""",
"bb": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0ibm9uZSIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxnIGZpbGw9IiMwMDAiIHN0cm9rZS1saW5lY2FwPSJidXR0Ij48cGF0aCBkPSJNOSAzNmMzLjM5LS45NyAxMC4xMS40MyAxMy41LTIgMy4zOSAyLjQzIDEwLjExIDEuMDMgMTMuNSAyIDAgMCAxLjY1LjU0IDMgMi0uNjguOTctMS42NS45OS0zIC41LTMuMzktLjk3LTEwLjExLjQ2LTEzLjUtMS0zLjM5IDEuNDYtMTAuMTEuMDMtMTMuNSAxLTEuMzU0LjQ5LTIuMzIzLjQ3LTMtLjUgMS4zNTQtMS45NCAzLTIgMy0yeiIvPjxwYXRoIGQ9Ik0xNSAzMmMyLjUgMi41IDEyLjUgMi41IDE1IDAgLjUtMS41IDAtMiAwLTIgMC0yLjUtMi41LTQtMi41LTQgNS41LTEuNSA2LTExLjUtNS0xNS41LTExIDQtMTAuNSAxNC01IDE1LjUgMCAwLTIuNSAxLjUtMi41IDQgMCAwLS41LjUgMCAyeiIvPjxwYXRoIGQ9Ik0yNSA4YTIuNSAyLjUgMCAxIDEtNSAwIDIuNSAyLjUgMCAxIDEgNSAweiIvPjwvZz48cGF0aCBkPSJNMTcuNSAyNmgxME0xNSAzMGgxNW0tNy41LTE0LjV2NU0yMCAxOGg1IiBzdHJva2U9IiNlY2VjZWMiIHN0cm9rZS1saW5lam9pbj0ibWl0ZXIiLz48L2c+PC9zdmc+""",
"br": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxwYXRoIGQ9Ik05IDM5aDI3di0zSDl2M3ptMy41LTdsMS41LTIuNWgxN2wxLjUgMi41aC0yMHptLS41IDR2LTRoMjF2NEgxMnoiIHN0cm9rZS1saW5lY2FwPSJidXR0Ii8+PHBhdGggZD0iTTE0IDI5LjV2LTEzaDE3djEzSDE0eiIgc3Ryb2tlLWxpbmVjYXA9ImJ1dHQiIHN0cm9rZS1saW5lam9pbj0ibWl0ZXIiLz48cGF0aCBkPSJNMTQgMTYuNUwxMSAxNGgyM2wtMyAyLjVIMTR6TTExIDE0VjloNHYyaDVWOWg1djJoNVY5aDR2NUgxMXoiIHN0cm9rZS1saW5lY2FwPSJidXR0Ii8+PHBhdGggZD0iTTEyIDM1LjVoMjFtLTIwLTRoMTltLTE4LTJoMTdtLTE3LTEzaDE3TTExIDE0aDIzIiBmaWxsPSJub25lIiBzdHJva2U9IiNlY2VjZWMiIHN0cm9rZS13aWR0aD0iMSIgc3Ryb2tlLWxpbmVqb2luPSJtaXRlciIvPjwvZz48L3N2Zz4=""",
"bq": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxnIHN0cm9rZT0ibm9uZSI+PGNpcmNsZSBjeD0iNiIgY3k9IjEyIiByPSIyLjc1Ii8+PGNpcmNsZSBjeD0iMTQiIGN5PSI5IiByPSIyLjc1Ii8+PGNpcmNsZSBjeD0iMjIuNSIgY3k9IjgiIHI9IjIuNzUiLz48Y2lyY2xlIGN4PSIzMSIgY3k9IjkiIHI9IjIuNzUiLz48Y2lyY2xlIGN4PSIzOSIgY3k9IjEyIiByPSIyLjc1Ii8+PC9nPjxwYXRoIGQ9Ik05IDI2YzguNS0xLjUgMjEtMS41IDI3IDBsMi41LTEyLjVMMzEgMjVsLS4zLTE0LjEtNS4yIDEzLjYtMy0xNC41LTMgMTQuNS01LjItMTMuNkwxNCAyNSA2LjUgMTMuNSA5IDI2eiIgc3Ryb2tlLWxpbmVjYXA9ImJ1dHQiLz48cGF0aCBkPSJNOSAyNmMwIDIgMS41IDIgMi41IDQgMSAxLjUgMSAxIC41IDMuNS0xLjUgMS0xLjUgMi41LTEuNSAyLjUtMS41IDEuNS41IDIuNS41IDIuNSA2LjUgMSAxNi41IDEgMjMgMCAwIDAgMS41LTEgMC0yLjUgMCAwIC41LTEuNS0xLTIuNS0uNS0yLjUtLjUtMiAuNS0zLjUgMS0yIDIuNS0yIDIuNS00LTguNS0xLjUtMTguNS0xLjUtMjcgMHoiIHN0cm9rZS1saW5lY2FwPSJidXR0Ii8+PHBhdGggZD0iTTExIDM4LjVhMzUgMzUgMSAwIDAgMjMgMCIgZmlsbD0ibm9uZSIgc3Ryb2tlLWxpbmVjYXA9ImJ1dHQiLz48cGF0aCBkPSJNMTEgMjlhMzUgMzUgMSAwIDEgMjMgMG0tMjEuNSAyLjVoMjBtLTIxIDNhMzUgMzUgMSAwIDAgMjIgMG0tMjMgM2EzNSAzNSAxIDAgMCAyNCAwIiBmaWxsPSJub25lIiBzdHJva2U9IiNlY2VjZWMiLz48L2c+PC9zdmc+""",
"bk": """PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSI0NSIgaGVpZ2h0PSI0NSI+PGcgZmlsbD0ibm9uZSIgZmlsbC1ydWxlPSJldmVub2RkIiBzdHJva2U9IiMwMDAiIHN0cm9rZS13aWR0aD0iMS41IiBzdHJva2UtbGluZWNhcD0icm91bmQiIHN0cm9rZS1saW5lam9pbj0icm91bmQiPjxwYXRoIGQ9Ik0yMi41IDExLjYzVjYiIHN0cm9rZS1saW5lam9pbj0ibWl0ZXIiLz48cGF0aCBkPSJNMjIuNSAyNXM0LjUtNy41IDMtMTAuNWMwIDAtMS0yLjUtMy0yLjVzLTMgMi41LTMgMi41Yy0xLjUgMyAzIDEwLjUgMyAxMC41IiBmaWxsPSIjMDAwIiBzdHJva2UtbGluZWNhcD0iYnV0dCIgc3Ryb2tlLWxpbmVqb2luPSJtaXRlciIvPjxwYXRoIGQ9Ik0xMS41IDM3YzUuNSAzLjUgMTUuNSAzLjUgMjEgMHYtN3M5LTQuNSA2LTEwLjVjLTQtNi41LTEzLjUtMy41LTE2IDRWMjd2LTMuNWMtMy41LTcuNS0xMy0xMC41LTE2LTQtMyA2IDUgMTAgNSAxMFYzN3oiIGZpbGw9IiMwMDAiLz48cGF0aCBkPSJNMjAgOGg1IiBzdHJva2UtbGluZWpvaW49Im1pdGVyIi8+PHBhdGggZD0iTTMyIDI5LjVzOC41LTQgNi4wMy05LjY1QzM0LjE1IDE0IDI1IDE4IDIyLjUgMjQuNWwuMDEgMi4xLS4wMS0yLjFDMjAgMTggOS45MDYgMTQgNi45OTcgMTkuODVjLTIuNDk3IDUuNjUgNC44NTMgOSA0Ljg1MyA5IiBzdHJva2U9IiNlY2VjZWMiLz48cGF0aCBkPSJNMTEuNSAzMGM1LjUtMyAxNS41LTMgMjEgMG0tMjEgMy41YzUuNS0zIDE1LjUtMyAyMiAwbS0yMSAzLjVjNS41LTMgMTUuNS0zIDIxIDAiIHN0cm9rZT0iI2VjZWNlYyIvPjwvZz48L3N2Zz4=""",
}

def load_piece_surfaces():
    result = {}
    for key, data in PIECE_B64.items():
        raw = base64.b64decode(data)
        try:
            surf = pygame.image.load(io.BytesIO(raw), f"{key}.svg").convert_alpha()
            surf = pygame.transform.smoothscale(surf, (36, 36))
        except Exception:
            # Fallback if the local SDL_image build has no SVG decoder.
            surf = pygame.Surface((36, 36), pygame.SRCALPHA)
            pygame.draw.circle(surf, (245,245,245) if key[0]=="w" else (35,35,35), (18,18), 15)
            pygame.draw.circle(surf, (30,30,30), (18,18), 15, 2)
        result[key] = surf
    return result

def headers():
    h = {"User-Agent": "ESP32-C5-Handheld-Chess-Prototype/0.54"}
    if TOKEN:
        h["Authorization"] = f"Bearer {TOKEN}"
    return h


# Built-in Wi-Fi profiles for the handheld.
# Priority: lower number = preferred.
# Passwords are base64-obfuscated only; this is not cryptographic protection.
def _wifi_secret(encoded):
    return base64.b64decode(encoded).decode("utf-8")

KNOWN_WIFI_PROFILES = [
    {"ssid": "REPLACE_WITH_YOUR_WIFI_SSID", "password_b64": "UkVQTEFDRV9XSVRIX1lPVVJfV0lGSV9QQVNTV09SRA==", "priority": 1},
    {"ssid": "REPLACE_WITH_YOUR_WIFI_SSID", "password_b64": "UkVQTEFDRV9XSVRIX1lPVVJfV0lGSV9QQVNTV09SRA==", "priority": 2},
    {"ssid": "REPLACE_WITH_YOUR_WIFI_SSID", "password_b64": "UkVQTEFDRV9XSVRIX1lPVVJfV0lGSV9QQVNTV09SRA==", "priority": 3},
]

# The Mac simulator follows the same priority logic and can switch Wi-Fi.
# Set LICHESS_AUTO_WIFI=0 before launch if you do not want the Mac to switch networks.
AUTO_SWITCH_MAC_WIFI = os.environ.get("LICHESS_AUTO_WIFI", "1").strip().lower() not in ("0", "false", "no")

EVQ = queue.Queue()

class Lichess:
    def __init__(self):
        self.s = requests.Session()
        self.s.headers.update(headers())
        self.account = None
        self.event_stop = threading.Event()
        self.game_stop = threading.Event()
        self.seek_stop = threading.Event()
        self.seek_response = None

    def check_network(self):
        """Measure the real requests path and, separately, a direct path.

        v5.3 had a measurement bug: it used stream=True and closed the response
        without consuming the body. That prevents urllib3 from safely returning
        the connection to the pool, so the supposed "warm" request could create
        another fresh connection and look almost identical to the cold request.

        Here each tiny API response is fully consumed. `Response.elapsed` gives
        time-to-response-headers, while consuming the body lets the next request
        reuse the same TCP/TLS connection.
        """
        probe_headers = {"User-Agent": headers()["User-Agent"]}
        if TOKEN:
            probe_headers["Authorization"] = f"Bearer {TOKEN}"
            url = BASE + "/api/account"
        else:
            url = BASE + "/api/tv/channels"

        def safe_proxy_label(proxy_url):
            if not proxy_url:
                return "direct"
            try:
                u = urlsplit(proxy_url)
                host = u.hostname or "proxy"
                if u.port:
                    host += f":{u.port}"
                return host
            except Exception:
                return "proxy"

        def run_probe(trust_env):
            samples = []
            statuses = []
            with requests.Session() as s:
                s.trust_env = trust_env
                for _ in range(3):
                    r = s.get(
                        url,
                        headers=probe_headers,
                        timeout=8,
                        stream=False,
                        allow_redirects=False,
                    )
                    # Body is already downloaded when stream=False. Touch it
                    # explicitly to make the connection-reuse requirement clear.
                    _ = r.content
                    samples.append(int(r.elapsed.total_seconds() * 1000))
                    statuses.append(r.status_code)
            return {
                "cold": samples[0],
                # Median-ish of two warm samples without importing statistics.
                "warm": min(samples[1:]) if len(samples) > 1 else samples[0],
                "samples": samples,
                "statuses": statuses,
            }

        try:
            proxies = requests.utils.get_environ_proxies(url)
            proxy_url = proxies.get("https") or proxies.get("http")
            routed = run_probe(True)

            direct = None
            direct_error = None
            try:
                direct = run_probe(False)
            except Exception as e:
                direct_error = str(e)

            online = all(status < 500 for status in routed["statuses"])
            return {
                "online": online,
                "latency": routed["warm"],
                "cold_latency": routed["cold"],
                "routed_samples": routed["samples"],
                "proxy_label": safe_proxy_label(proxy_url),
                "uses_proxy": bool(proxy_url),
                "direct_latency": None if direct is None else direct["warm"],
                "direct_cold_latency": None if direct is None else direct["cold"],
                "direct_samples": None if direct is None else direct["samples"],
                "direct_error": direct_error,
                "status": routed["statuses"][-1],
            }
        except Exception as e:
            return {
                "online": False,
                "latency": None,
                "cold_latency": None,
                "direct_latency": None,
                "direct_cold_latency": None,
                "proxy_label": "unknown",
                "uses_proxy": False,
                "error": str(e),
            }

    def get_account(self):
        if not TOKEN:
            return None
        r = self.s.get(BASE + "/api/account", timeout=15)
        r.raise_for_status()
        self.account = r.json()
        return self.account

    def start_event_stream(self):
        if not TOKEN:
            return
        self.event_stop.clear()
        def work():
            try:
                with self.s.get(BASE + "/api/stream/event", stream=True,
                                timeout=(15, None),
                                headers={**headers(), "Accept":"application/x-ndjson"}) as r:
                    r.raise_for_status()
                    for line in r.iter_lines(chunk_size=1, decode_unicode=True):
                        if self.event_stop.is_set(): break
                        if not line: continue
                        try:
                            EVQ.put(("event", json.loads(line)))
                        except Exception:
                            pass
            except Exception as e:
                EVQ.put(("status", f"Event stream: {e}"))
        threading.Thread(target=work, daemon=True).start()

    def create_ai(self, level, minutes, inc):
        data = {
            "level": level,
            "clock.limit": int(minutes * 60),
            "clock.increment": inc,
            "color": "random",
            "variant": "standard",
        }
        r = self.s.post(BASE + "/api/challenge/ai", data=data, timeout=15)
        r.raise_for_status()
        return r.json()

    def seek(self, minutes, inc, rated=False):
        # Board API random seeks: Rapid/Classical/Correspondence only.
        self.cancel_seek()
        self.seek_stop = threading.Event()
        stop = self.seek_stop
        def work():
            data = {
                "time": minutes,
                "increment": inc,
                "rated": "true" if rated else "false",
                "variant": "standard",
            }
            EVQ.put(("status", f"Seeking {minutes}+{inc}..."))
            try:
                with self.s.post(BASE + "/api/board/seek", data=data, stream=True,
                                 timeout=(15, None),
                                 headers={**headers(), "Accept":"application/x-ndjson"}) as r:
                    self.seek_response = r
                    if r.status_code >= 400:
                        EVQ.put(("status", f"Seek failed {r.status_code}: {r.text[:120]}"))
                        return
                    for _ in r.iter_lines(chunk_size=1):
                        if stop.is_set():
                            break
                if not stop.is_set():
                    EVQ.put(("status", "Seek finished."))
            except Exception as e:
                if not stop.is_set():
                    EVQ.put(("status", f"Seek error: {e}"))
            finally:
                self.seek_response = None
        threading.Thread(target=work, daemon=True).start()

    def cancel_seek(self):
        self.seek_stop.set()
        r = self.seek_response
        self.seek_response = None
        if r is not None:
            try:
                r.close()
            except Exception:
                pass

    def start_game_stream(self, gid):
        self.game_stop.set()
        self.game_stop = threading.Event()
        stop = self.game_stop
        def work():
            try:
                with self.s.get(BASE + f"/api/board/game/stream/{gid}", stream=True,
                                timeout=(15, None),
                                headers={**headers(), "Accept":"application/x-ndjson"}) as r:
                    r.raise_for_status()
                    for line in r.iter_lines(chunk_size=1, decode_unicode=True):
                        if stop.is_set(): break
                        if not line: continue
                        try: EVQ.put(("game", json.loads(line)))
                        except Exception: pass
            except Exception as e:
                EVQ.put(("status", f"Game stream: {e}"))
        threading.Thread(target=work, daemon=True).start()

    def move(self, gid, uci):
        def work():
            t0 = time.monotonic()
            try:
                r = self.s.post(BASE + f"/api/board/game/{gid}/move/{uci}", timeout=15)
                rtt = int((time.monotonic() - t0) * 1000)
                EVQ.put(("move_result", {
                    "ok": r.status_code == 200,
                    "uci": uci,
                    "rtt_ms": rtt,
                    "status_code": r.status_code,
                    "text": r.text[:100],
                }))
            except Exception as e:
                rtt = int((time.monotonic() - t0) * 1000)
                EVQ.put(("move_result", {
                    "ok": False,
                    "uci": uci,
                    "rtt_ms": rtt,
                    "status_code": None,
                    "text": str(e),
                }))
        threading.Thread(target=work, daemon=True).start()

    def resign(self, gid):
        if not gid: return
        threading.Thread(target=lambda: self.s.post(BASE + f"/api/board/game/{gid}/resign", timeout=15),
                         daemon=True).start()

    def _fetch_puzzle(self, difficulty="normal", exclude_id=None):
        """Fetch a puzzle, preferring the batch API so Next really advances."""
        # Authenticated batch endpoint can return several unseen puzzles. Ask for
        # three so we can avoid accidentally redisplaying the current one.
        if TOKEN:
            r = self.s.get(
                BASE + "/api/puzzle/batch/mix",
                params={"difficulty": difficulty, "nb": 3},
                timeout=15,
            )
            if r.status_code == 200:
                data = r.json()
                items = data.get("puzzles") or []
                for item in items:
                    if (item.get("puzzle") or {}).get("id") != exclude_id:
                        return item
                if items:
                    return items[0]
            elif r.status_code not in (401,403):
                r.raise_for_status()

        # Anonymous / limited-scope fallback.
        last = None
        for _ in range(2):
            r = requests.get(
                BASE + "/api/puzzle/next",
                params={"difficulty": difficulty},
                headers={"User-Agent": headers()["User-Agent"]},
                timeout=15,
            )
            r.raise_for_status()
            last = r.json()
            if (last.get("puzzle") or {}).get("id") != exclude_id:
                break
        return last

    def next_puzzle(self, difficulty="normal", exclude_id=None):
        def work():
            try:
                item = self._fetch_puzzle(difficulty, exclude_id)
                EVQ.put(("puzzle", item))
            except Exception as e:
                EVQ.put(("puzzle_error", f"Puzzle API: {e}"))
        threading.Thread(target=work, daemon=True).start()

    def finish_puzzle_and_next(self, puzzle_id, win, difficulty="normal"):
        """Record an unrated result when puzzle:write is available, then fetch next."""
        def work():
            try:
                if TOKEN and puzzle_id:
                    payload = {"solutions": [{"id": puzzle_id, "win": bool(win), "rated": False}]}
                    r = self.s.post(
                        BASE + "/api/puzzle/batch/mix",
                        params={"nb": 0},
                        json=payload,
                        timeout=15,
                    )
                    # 401/403 simply means this token lacks puzzle:write; next still works.
                    if r.status_code not in (200,401,403):
                        r.raise_for_status()
                item = self._fetch_puzzle(difficulty, puzzle_id)
                EVQ.put(("puzzle", item))
            except Exception as e:
                EVQ.put(("puzzle_error", f"Next puzzle: {e}"))
        threading.Thread(target=work, daemon=True).start()

class App:
    def __init__(self):
        pygame.init()
        pygame.display.set_caption("ESP32-C5 Lichess Handheld Simulator · v5.4")
        self.sc = pygame.display.set_mode((W,H))
        self.clock = pygame.time.Clock()
        self.f24 = pygame.font.SysFont("Arial",24,bold=True)
        self.f18 = pygame.font.SysFont("Arial",18,bold=True)
        self.f15 = pygame.font.SysFont("Arial",15)
        self.f12 = pygame.font.SysFont("Arial",12)
        self.pieces = load_piece_surfaces()
        self.api = Lichess()
        self.screen = "home"
        self.status = "Starting..."
        self.board = chess.Board()
        self.selected = None
        self.last = None
        self.my_color = chess.WHITE
        self.game_id = None
        self.game_status = "idle"
        self.opponent = "—"
        self.wtime = self.btime = None
        self.winc = self.binc = 0
        # Authoritative clock snapshots arrive in gameFull/gameState.
        # Between snapshots we interpolate locally with time.monotonic().
        self.clock_sync_mono = None
        self.clock_source_delay_ms = 0.0
        self.clock_last_sync_mono = None
        self.clock_last_correction_ms = 0
        self.move_post_rtt_ms = None
        self.move_stream_confirm_ms = None
        self.pending_move_uci = None
        self.pending_move_sent_mono = None
        self.pending_prev_board = None
        self.pending_prev_last = None
        self.pending_prev_clock = None
        self.network_cold_latency = None
        self.account_id = ""
        self.ai_level = 3
        self.ai_time = (5,3)
        self.online_time = (10,0)
        self.online_rated = False
        self.puzzle_diff = "normal"
        self.puzzle_solution = []
        self.puzzle_i = 0
        self.puzzle_color = chess.WHITE
        self.puzzle_meta = {}
        self.puzzle_start_board = None
        self.puzzle_answer_steps = []
        self.puzzle_hint_square = None
        self.puzzle_had_mistake = False
        self.puzzle_answer_revealed = False
        self.puzzle_loading = False
        # Puzzle solution review / step navigation state.
        self.puzzle_review_mode = False
        self.puzzle_review_ply = 0
        self.puzzle_saved_board = None
        self.puzzle_saved_i = 0
        self.puzzle_saved_last = None
        self.puzzle_saved_status = ""
        self.network_state = "checking"
        self.network_latency = None
        self.network_cold_latency = None
        self.network_direct_latency = None
        self.network_direct_cold_latency = None
        self.network_proxy_label = "unknown"
        self.network_uses_proxy = False
        self.network_detail = "Checking Lichess..."
        self.current_ssid = "—"
        self.wifi_device = None
        self.wifi_auto_state = "starting"
        self.wifi_last_target = None
        threading.Thread(target=self.login, daemon=True).start()
        self.refresh_network()

    def login(self):
        if not TOKEN:
            EVQ.put(("status","Offline login: set LICHESS_TOKEN for online/AI. Puzzle still works."))
            return
        try:
            a = self.api.get_account()
            self.account_id = (a.get("id") or a.get("username") or "").lower()
            EVQ.put(("status", f"Logged in: {a.get('username')}"))
            self.api.start_event_stream()
        except Exception as e:
            EVQ.put(("status", f"Login failed: {e}"))

    def _run_cmd(self, args, timeout=8):
        try:
            p = subprocess.run(
                args,
                capture_output=True,
                text=True,
                timeout=timeout,
            )
            return p.returncode, (p.stdout or "").strip(), (p.stderr or "").strip()
        except Exception as e:
            return 999, "", str(e)

    def _mac_wifi_device(self):
        if sys.platform != "darwin":
            return None
        rc, out, _ = self._run_cmd(["networksetup", "-listallhardwareports"])
        if rc != 0:
            return None
        lines = out.splitlines()
        for i, line in enumerate(lines):
            if line.strip() in ("Hardware Port: Wi-Fi", "Hardware Port: AirPort"):
                for j in range(i + 1, min(i + 4, len(lines))):
                    if lines[j].startswith("Device:"):
                        return lines[j].split(":", 1)[1].strip()
        return None

    def _mac_current_ssid(self, device=None):
        if sys.platform != "darwin":
            return None
        device = device or self.wifi_device or self._mac_wifi_device()
        if not device:
            return None
        rc, out, _ = self._run_cmd(
            ["networksetup", "-getairportnetwork", device],
            timeout=5,
        )
        if rc != 0:
            return None
        # Typical result: "Current Wi-Fi Network: SSID"
        if ":" in out:
            ssid = out.split(":", 1)[1].strip()
            if ssid and "not associated" not in ssid.lower():
                return ssid
        return None

    def _mac_scan_ssids(self):
        """Best-effort scan; returns None when the current macOS has no usable airport CLI."""
        if sys.platform != "darwin":
            return None
        candidates = [
            "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/Current/Resources/airport",
            "/System/Library/PrivateFrameworks/Apple80211.framework/Versions/A/Resources/airport",
        ]
        airport = next((x for x in candidates if os.path.exists(x)), None)
        if not airport:
            return None
        rc, out, _ = self._run_cmd([airport, "-s"], timeout=8)
        if rc != 0 or not out:
            return None

        found = set()
        # `airport -s` is column-based. Match our exact known SSIDs in each line
        # rather than trying to parse every macOS formatting variant.
        for line in out.splitlines():
            for profile in KNOWN_WIFI_PROFILES:
                ssid = profile["ssid"]
                if ssid in line:
                    found.add(ssid)
        return found

    def _mac_connect_profile(self, profile):
        if sys.platform != "darwin":
            return False, "not macOS"
        device = self.wifi_device or self._mac_wifi_device()
        if not device:
            return False, "Wi-Fi device not found"

        ssid = profile["ssid"]
        password = _wifi_secret(profile["password_b64"])
        rc, out, err = self._run_cmd(
            ["networksetup", "-setairportnetwork", device, ssid, password],
            timeout=18,
        )
        # Do not retain an extra plaintext reference longer than needed.
        password = None
        if rc != 0:
            return False, err or out or f"networksetup returned {rc}"

        # Give macOS a moment to associate and verify the actual current SSID.
        for _ in range(8):
            time.sleep(0.35)
            current = self._mac_current_ssid(device)
            if current == ssid:
                return True, ssid
        return False, "association was not confirmed"

    def _choose_known_wifi(self, visible_ssids):
        """Return the highest-priority built-in network that is visible."""
        if not visible_ssids:
            return None
        for p in sorted(KNOWN_WIFI_PROFILES, key=lambda x: x["priority"]):
            if p["ssid"] in visible_ssids:
                return p
        return None

    def auto_connect_known_wifi(self):
        """Follow the requested priority order.

        On macOS:
        - scan if the legacy airport CLI is available;
        - otherwise keep a working known network, or probe the profiles in order.

        On the future ESP32 build the same selection policy maps directly to
        Wi-Fi.scanNetworks() + WiFi.begin().
        """
        self.wifi_auto_state = "checking profiles"
        self.wifi_device = self._mac_wifi_device()
        current = self._mac_current_ssid(self.wifi_device)
        if current:
            self.current_ssid = current

        if sys.platform != "darwin":
            self.wifi_auto_state = "policy ready (ESP32 target)"
            return {
                "ssid": current,
                "wifi_auto_state": self.wifi_auto_state,
                "switched": False,
            }

        if not AUTO_SWITCH_MAC_WIFI:
            self.wifi_auto_state = "auto-switch disabled"
            return {
                "ssid": current,
                "wifi_auto_state": self.wifi_auto_state,
                "switched": False,
            }

        visible = self._mac_scan_ssids()
        if visible is not None:
            target = self._choose_known_wifi(visible)
            if target is None:
                self.wifi_auto_state = "no built-in network visible"
                return {
                    "ssid": current,
                    "wifi_auto_state": self.wifi_auto_state,
                    "switched": False,
                }

            self.wifi_last_target = target["ssid"]
            if current == target["ssid"]:
                self.wifi_auto_state = f"connected · priority {target['priority']}"
                return {
                    "ssid": current,
                    "wifi_auto_state": self.wifi_auto_state,
                    "switched": False,
                }

            self.wifi_auto_state = f"connecting priority {target['priority']}..."
            ok, detail = self._mac_connect_profile(target)
            current = self._mac_current_ssid(self.wifi_device) or current
            self.current_ssid = current or "—"
            self.wifi_auto_state = (
                f"connected · priority {target['priority']}"
                if ok else f"connect failed · {detail[:34]}"
            )
            return {
                "ssid": current,
                "wifi_auto_state": self.wifi_auto_state,
                "switched": ok,
            }

        # Fallback for newer macOS versions without `airport -s`.
        # If already on any built-in network, do not disrupt it just to test
        # whether a higher-priority SSID happens to be nearby.
        known_by_name = {p["ssid"]: p for p in KNOWN_WIFI_PROFILES}
        if current in known_by_name:
            p = known_by_name[current]
            self.wifi_auto_state = f"connected · priority {p['priority']} · scan unavailable"
            return {
                "ssid": current,
                "wifi_auto_state": self.wifi_auto_state,
                "switched": False,
            }

        # No scan and not currently on a built-in profile: try in strict order.
        for target in sorted(KNOWN_WIFI_PROFILES, key=lambda x: x["priority"]):
            self.wifi_last_target = target["ssid"]
            self.wifi_auto_state = f"trying priority {target['priority']}..."
            ok, _ = self._mac_connect_profile(target)
            if ok:
                current = target["ssid"]
                self.current_ssid = current
                self.wifi_auto_state = f"connected · priority {target['priority']}"
                return {
                    "ssid": current,
                    "wifi_auto_state": self.wifi_auto_state,
                    "switched": True,
                }

        self.wifi_auto_state = "no built-in network connected"
        return {
            "ssid": self._mac_current_ssid(self.wifi_device) or current,
            "wifi_auto_state": self.wifi_auto_state,
            "switched": False,
        }

    def refresh_network(self):
        self.network_state = "checking"
        self.network_detail = "Selecting known Wi-Fi, then checking Lichess..."
        self.wifi_auto_state = "checking profiles"
        def work():
            wifi = self.auto_connect_known_wifi()
            result = self.api.check_network()
            result["ssid"] = wifi.get("ssid")
            result["wifi_auto_state"] = wifi.get("wifi_auto_state")
            result["wifi_switched"] = wifi.get("switched", False)
            EVQ.put(("network", result))
        threading.Thread(target=work, daemon=True).start()

    def txt(self, s, font=None, c=TEXT):
        return (font or self.f15).render(str(s), True, c)

    def button(self, rect, label, on=False, enabled=True):
        col = BTN_ON if on else BTN
        if not enabled: col = (48,50,54)
        pygame.draw.rect(self.sc,col,rect,border_radius=8)
        surf = self.txt(label,self.f15, TEXT if enabled else (130,130,130))
        self.sc.blit(surf,surf.get_rect(center=rect.center))

    def setstatus(self,s): self.status=str(s)[:120]

    def home(self): self.screen="home"; self.selected=None; self.puzzle_hint_square=None

    def start_ai(self):
        if not TOKEN:
            self.setstatus("Need LICHESS_TOKEN for AI.")
            return
        self.setstatus(f"Starting AI level {self.ai_level}, {self.ai_time[0]}+{self.ai_time[1]}...")
        def work():
            try:
                data=self.api.create_ai(self.ai_level,*self.ai_time)
                EVQ.put(("start_gid",data["id"]))
            except Exception as e: EVQ.put(("status",f"AI failed: {e}"))
        threading.Thread(target=work,daemon=True).start()

    def start_seek(self):
        if not TOKEN:
            self.setstatus("Need LICHESS_TOKEN for online play.")
            return
        if self.network_state != "online":
            self.setstatus("Network is not connected to Lichess.")
            self.screen = "network"
            return
        self.screen = "online_wait"
        self.api.seek(*self.online_time,self.online_rated)

    def cancel_seek(self):
        self.api.cancel_seek()
        self.setstatus("Search cancelled.")
        self.screen = "online"

    def start_gid(self,gid):
        self.game_id=gid
        self.screen="game"
        self.board=chess.Board()
        self.selected=None
        self.last=None
        self.pending_move_uci=None
        self.pending_move_sent_mono=None
        self.pending_prev_board=None
        self.pending_prev_last=None
        self.pending_prev_clock=None
        self.clock_sync_mono=None
        self.clock_last_sync_mono=None
        self.move_stream_confirm_ms=None
        self.api.start_game_stream(gid)

    def process_event(self,d):
        t=d.get("type")
        if t=="gameStart":
            g=d.get("game",{})
            gid=g.get("gameId") or g.get("id")
            if gid:
                self.api.seek_stop.set()
                color = g.get("color")
                if color == "white":
                    self.my_color = chess.WHITE
                elif color == "black":
                    self.my_color = chess.BLACK
                self.start_gid(gid)
        elif t=="challenge":
            ch=d.get("challenge",{})
            who=((ch.get("challenger") or {}).get("name") or "Someone")
            self.setstatus(f"Challenge from {who}")

    def apply_moves(self,moves):
        b=chess.Board()
        last=None
        for u in moves.split():
            try:
                m=chess.Move.from_uci(u)
                if m not in b.legal_moves: break
                b.push(m); last=m
            except: break
        self.board=b; self.last=last

    def estimated_one_way_ms(self):
        """Small display-only estimate of stream transit time.

        We never alter the actual server clock. This only makes the locally
        interpolated display closer to the server while waiting for the next
        authoritative gameState.
        """
        candidates = []
        if self.network_latency is not None:
            candidates.append(float(self.network_latency))
        if self.move_post_rtt_ms is not None:
            candidates.append(float(self.move_post_rtt_ms))
        if not candidates:
            return 0.0
        # Use the faster recent path estimate so server processing time does not
        # get mistaken entirely for network delay. Cap extreme compensation.
        return max(0.0, min(min(candidates) / 2.0, 500.0))

    def set_clock_snapshot(self, wtime, btime, source_delay_ms=0.0):
        now = time.monotonic()
        old_w, old_b = self.current_clocks(include_source_delay=False)
        self.wtime = None if wtime is None else float(wtime)
        self.btime = None if btime is None else float(btime)
        self.clock_sync_mono = now
        self.clock_last_sync_mono = now
        self.clock_source_delay_ms = max(0.0, float(source_delay_ms))

        # Diagnostic only: how much an authoritative state corrected our
        # current local display. Large values help identify real stream delay.
        if old_w is not None and old_b is not None and self.wtime is not None and self.btime is not None:
            self.clock_last_correction_ms = int(max(
                abs(old_w - self.wtime),
                abs(old_b - self.btime),
            ))

    def current_clocks(self, include_source_delay=True):
        """Return continuously interpolated (white_ms, black_ms).

        Lichess does not send a clock packet every 10/100/1000 ms. It sends
        authoritative clock values with game state events, so the correct UI
        architecture is: server snapshot -> local monotonic countdown ->
        resync on every new snapshot.
        """
        if self.wtime is None or self.btime is None:
            return self.wtime, self.btime
        w = float(self.wtime)
        b = float(self.btime)
        if self.clock_sync_mono is None or self.game_status != "started":
            return max(0.0, w), max(0.0, b)

        elapsed = max(0.0, (time.monotonic() - self.clock_sync_mono) * 1000.0)
        if include_source_delay:
            elapsed += self.clock_source_delay_ms

        # board.turn is the side whose clock is currently running.
        if self.board.turn == chess.WHITE:
            w -= elapsed
        else:
            b -= elapsed
        return max(0.0, w), max(0.0, b)

    def sync_server_clocks(self, state):
        self.winc = int(state.get("winc") or self.winc or 0)
        self.binc = int(state.get("binc") or self.binc or 0)
        self.set_clock_snapshot(
            state.get("wtime"),
            state.get("btime"),
            source_delay_ms=self.estimated_one_way_ms(),
        )

    def clear_pending_move(self):
        self.pending_move_uci = None
        self.pending_move_sent_mono = None
        self.pending_prev_board = None
        self.pending_prev_last = None
        self.pending_prev_clock = None

    def handle_move_result(self, data):
        self.move_post_rtt_ms = data.get("rtt_ms")
        if data.get("ok"):
            # The board was already updated optimistically. The game stream is
            # still authoritative and will shortly correct board + clock.
            self.setstatus(f"Move accepted · API {self.move_post_rtt_ms} ms · waiting for clock sync")
            return

        # Server rejected the move: roll back the optimistic board and resume
        # the mover's clock, including time spent waiting for the failed POST.
        if self.pending_prev_board is not None and self.pending_prev_clock is not None:
            self.board = self.pending_prev_board.copy(stack=False)
            self.last = self.pending_prev_last
            w, b, running_color, sent_at = self.pending_prev_clock
            extra = max(0.0, (time.monotonic() - sent_at) * 1000.0)
            if running_color == chess.WHITE:
                w = max(0.0, w - extra)
            else:
                b = max(0.0, b - extra)
            self.set_clock_snapshot(w, b, source_delay_ms=0)
        self.setstatus(
            f"Move rejected · API {self.move_post_rtt_ms} ms · "
            f"{data.get('status_code') or ''} {data.get('text','')}"
        )
        self.clear_pending_move()

    def process_game(self,d):
        recv_mono = time.monotonic()
        t=d.get("type")
        state = None
        if t=="gameFull":
            white=d.get("white",{}); black=d.get("black",{})
            wid=(white.get("id") or "").lower()
            bid=(black.get("id") or "").lower()
            if self.account_id and self.account_id==bid:
                self.my_color=chess.BLACK
                self.opponent=white.get("name") or white.get("id") or "White"
            elif self.account_id and self.account_id==wid:
                self.my_color=chess.WHITE
                self.opponent=black.get("name") or black.get("id") or ("Lichess AI" if black.get("aiLevel") else "Black")
            elif white.get("aiLevel") is not None:
                self.my_color=chess.BLACK
                self.opponent="Lichess AI"
            elif black.get("aiLevel") is not None:
                self.my_color=chess.WHITE
                self.opponent="Lichess AI"
            if self.my_color==chess.BLACK:
                self.opponent=white.get("name") or white.get("id") or self.opponent
            else:
                self.opponent=black.get("name") or black.get("id") or self.opponent
            state=d.get("state",{})
            self.apply_moves(state.get("moves",""))
            self.game_status=state.get("status","started")
        elif t=="gameState":
            state=d
            self.apply_moves(state.get("moves",""))
            self.game_status=state.get("status",self.game_status)

        if state is not None:
            self.sync_server_clocks(state)

            # If this server state contains our optimistic move, measure how
            # long it took from touch -> authoritative stream confirmation.
            if self.pending_move_uci and self.pending_move_sent_mono is not None:
                moves = state.get("moves","").split()
                if self.pending_move_uci in moves[-2:]:
                    self.move_stream_confirm_ms = int(
                        (recv_mono - self.pending_move_sent_mono) * 1000
                    )
                    self.setstatus(
                        f"Clock synced · API {self.move_post_rtt_ms or '?'} ms · "
                        f"stream {self.move_stream_confirm_ms} ms"
                    )
                    self.clear_pending_move()

    def _build_puzzle_position(self, p, g):
        """Build exactly the position Lichess presents to the solver.

        Lichess `initialPly` is the zero-based ply index of the final
        game move that must already be on the board when the puzzle starts.
        Therefore a PGN must be replayed through `initialPly + 1` plies.

        We also validate that the first solution move is legal. This catches
        future API/schema changes instead of silently showing the wrong side.
        """
        solution = list(p.get("solution") or [])
        if not solution:
            raise ValueError("Puzzle has no solution moves")

        if p.get("fen"):
            # Future-proofing: if Lichess provides a ready-to-solve FEN,
            # trust it, then validate it below.
            b = chess.Board(p["fen"])
        else:
            pg = chess.pgn.read_game(io.StringIO(g["pgn"]))
            if pg is None:
                raise ValueError("Could not parse puzzle PGN")

            moves = list(pg.mainline_moves())
            initial_ply = int(p["initialPly"])
            replay_count = initial_ply + 1

            if replay_count < 0 or replay_count > len(moves):
                raise ValueError(
                    f"initialPly={initial_ply} needs {replay_count} plies, "
                    f"but PGN has {len(moves)}"
                )

            b = pg.board()
            for m in moves[:replay_count]:
                b.push(m)

        first = chess.Move.from_uci(solution[0])
        if first not in b.legal_moves:
            raise ValueError(
                f"Puzzle position mismatch: first solution {solution[0]} "
                f"is not legal (side to move: "
                f"{'white' if b.turn == chess.WHITE else 'black'})"
            )
        return b, solution

    def _build_answer_steps(self, start_board, solution):
        b = start_board.copy(stack=False)
        out = []
        for uci in solution:
            m = chess.Move.from_uci(uci)
            if m not in b.legal_moves:
                out.append(f"? {uci}")
                break
            prefix = f"{b.fullmove_number}." if b.turn == chess.WHITE else f"{b.fullmove_number}..."
            out.append(f"{prefix} {b.san(m)}")
            b.push(m)
        return out

    def _puzzle_board_at_ply(self, ply):
        """Return the puzzle start position advanced by exactly `ply` solution plies."""
        if self.puzzle_start_board is None:
            raise ValueError("Puzzle start board is unavailable")
        ply = max(0, min(int(ply), len(self.puzzle_solution)))
        b = self.puzzle_start_board.copy(stack=False)
        last = None
        for uci in self.puzzle_solution[:ply]:
            m = chess.Move.from_uci(uci)
            if m not in b.legal_moves:
                raise ValueError(f"Review line mismatch at {uci}")
            b.push(m)
            last = m
        return b, last

    def _save_live_puzzle_state_for_review(self):
        if self.puzzle_saved_board is None:
            self.puzzle_saved_board = self.board.copy(stack=False)
            self.puzzle_saved_i = self.puzzle_i
            self.puzzle_saved_last = self.last
            self.puzzle_saved_status = self.status

    def enter_puzzle_review(self, target_ply=None):
        """Enter non-interactive solution review and optionally jump to a ply."""
        if not self.puzzle_solution or self.puzzle_start_board is None:
            self.setstatus("No puzzle line to review.")
            return
        self._save_live_puzzle_state_for_review()
        self.puzzle_answer_revealed = True
        self.puzzle_hint_square = None
        self.selected = None
        self.puzzle_review_mode = True
        if target_ply is None:
            target_ply = self.puzzle_i
        self.set_puzzle_review_ply(target_ply)
        self.screen = "puzzle_game"

    def set_puzzle_review_ply(self, ply):
        if not self.puzzle_solution or self.puzzle_start_board is None:
            return
        ply = max(0, min(int(ply), len(self.puzzle_solution)))
        try:
            b, last = self._puzzle_board_at_ply(ply)
        except Exception as e:
            self.setstatus(f"Review failed: {e}")
            return
        self.board = b
        self.last = last
        self.selected = None
        self.puzzle_hint_square = None
        self.puzzle_review_ply = ply
        self.my_color = self.puzzle_color
        if ply == 0:
            self.setstatus(f"Solution review: start position · 0/{len(self.puzzle_solution)}")
        else:
            step = self.puzzle_answer_steps[ply-1] if ply-1 < len(self.puzzle_answer_steps) else self.puzzle_solution[ply-1]
            self.setstatus(f"Solution review {ply}/{len(self.puzzle_solution)}: {step}")

    def puzzle_prev_step(self):
        if not self.puzzle_review_mode:
            self.enter_puzzle_review(self.puzzle_i)
        self.set_puzzle_review_ply(self.puzzle_review_ply - 1)

    def puzzle_next_step(self):
        if not self.puzzle_review_mode:
            self.enter_puzzle_review(self.puzzle_i)
        self.set_puzzle_review_ply(self.puzzle_review_ply + 1)

    def resume_puzzle_from_review(self):
        """Return to the exact live position that existed before step review."""
        if self.puzzle_saved_board is not None:
            self.board = self.puzzle_saved_board.copy(stack=False)
            self.puzzle_i = self.puzzle_saved_i
            self.last = self.puzzle_saved_last
            self.setstatus(self.puzzle_saved_status or "Returned to puzzle.")
        self.puzzle_review_mode = False
        self.puzzle_hint_square = None
        self.selected = None
        self.screen = "puzzle_game"

    def request_next_puzzle(self):
        if self.puzzle_loading:
            return
        self.puzzle_loading = True
        self.puzzle_hint_square = None
        self.setstatus("Loading next puzzle...")
        pid = self.puzzle_meta.get("id") if self.puzzle_meta else None
        if pid:
            solved = self.puzzle_i >= len(self.puzzle_solution) and not self.puzzle_had_mistake and not self.puzzle_answer_revealed
            self.api.finish_puzzle_and_next(pid, solved, self.puzzle_diff)
        else:
            self.api.next_puzzle(self.puzzle_diff)

    def show_puzzle_hint(self):
        if self.puzzle_i >= len(self.puzzle_solution):
            self.setstatus("Puzzle is already solved.")
            return
        try:
            m = chess.Move.from_uci(self.puzzle_solution[self.puzzle_i])
            if m not in self.board.legal_moves:
                self.setstatus("Hint unavailable: puzzle state mismatch.")
                return
            self.puzzle_hint_square = m.from_square
            piece = self.board.piece_at(m.from_square)
            name = chess.piece_name(piece.piece_type).title() if piece else "piece"
            self.setstatus(f"Hint: move the {name} on {chess.square_name(m.from_square)}.")
        except Exception as e:
            self.setstatus(f"Hint unavailable: {e}")

    def show_puzzle_answer(self):
        self.puzzle_answer_revealed = True
        self.puzzle_hint_square = None
        self.screen = "puzzle_answer"
        self.setstatus("Answer revealed. This attempt will not count as solved.")

    def process_puzzle(self,d):
        try:
            p=d["puzzle"]; g=d["game"]
            b, solution = self._build_puzzle_position(p, g)

            self.board=b
            # The board orientation must follow the actual solver side.
            # This can be either White or Black.
            self.my_color=b.turn
            self.puzzle_color=b.turn
            self.puzzle_solution=solution
            self.puzzle_i=0
            self.puzzle_meta=p
            self.puzzle_start_board=b.copy(stack=False)
            self.puzzle_answer_steps=self._build_answer_steps(self.puzzle_start_board, solution)
            self.puzzle_hint_square=None
            self.puzzle_had_mistake=False
            self.puzzle_answer_revealed=False
            self.puzzle_loading=False
            self.puzzle_review_mode=False
            self.puzzle_review_ply=0
            self.puzzle_saved_board=None
            self.puzzle_saved_i=0
            self.puzzle_saved_last=None
            self.puzzle_saved_status=""
            self.last=None; self.selected=None
            self.screen="puzzle_game"

            side = "White" if b.turn == chess.WHITE else "Black"
            self.setstatus(
                f"Puzzle {p['id']} · rating {p['rating']} · {side} to move"
            )
        except Exception as e:
            self.setstatus(f"Puzzle parse failed: {e}")

    def disp_to_sq(self,dx,dy):
        if self.my_color==chess.WHITE: return chess.square(dx,7-dy)
        return chess.square(7-dx,dy)

    def sq_to_disp(self,sq):
        f=chess.square_file(sq); r=chess.square_rank(sq)
        return (f,7-r) if self.my_color==chess.WHITE else (7-f,r)

    def click_board(self,x,y):
        if self.screen=="puzzle_game" and self.puzzle_review_mode:
            self.setstatus("Solution review is read-only. Use Prev/Next or Resume.")
            return
        sq=self.disp_to_sq(x//SQ,y//SQ)
        if self.selected is None:
            p=self.board.piece_at(sq)
            if p and p.color==self.board.turn:
                if self.screen=="game" and p.color!=self.my_color: return
                if self.screen=="puzzle_game" and p.color!=self.puzzle_color: return
                self.selected=sq
            return
        if sq==self.selected:
            self.selected=None; return
        p=self.board.piece_at(self.selected)
        promo=chess.QUEEN if p and p.piece_type==chess.PAWN and chess.square_rank(sq) in (0,7) else None
        m=chess.Move(self.selected,sq,promotion=promo)
        if m not in self.board.legal_moves:
            p2=self.board.piece_at(sq)
            self.selected=sq if p2 and p2.color==self.board.turn else None
            return
        self.selected=None
        if self.screen=="game":
            if self.board.turn!=self.my_color: return
            if self.pending_move_uci is not None:
                self.setstatus("Previous move is still syncing.")
                return

            # Save the authoritative/local-interpolated state so a rejected
            # move can be rolled back cleanly.
            now = time.monotonic()
            cur_w, cur_b = self.current_clocks()
            mover = self.board.turn
            self.pending_prev_board = self.board.copy(stack=False)
            self.pending_prev_last = self.last
            self.pending_prev_clock = (cur_w, cur_b, mover, now)
            self.pending_move_uci = m.uci()
            self.pending_move_sent_mono = now

            # Optimistic UI: show the move immediately instead of waiting up to
            # a network round trip for Lichess to echo it back.
            self.board.push(m)
            self.last = m

            # Freeze mover and start opponent immediately. Apply Fischer
            # increment locally; the next gameState will correct any difference.
            if cur_w is not None and cur_b is not None:
                if mover == chess.WHITE:
                    cur_w += self.winc
                else:
                    cur_b += self.binc
                self.set_clock_snapshot(cur_w, cur_b, source_delay_ms=0)

            self.setstatus(f"Sending {m.uci()}…")
            self.api.move(self.game_id,m.uci())
        else:
            exp=self.puzzle_solution[self.puzzle_i] if self.puzzle_i<len(self.puzzle_solution) else None

            # Lichess allows alternate mating moves in mate-in-1 puzzles.
            alternate_mate = False
            if m.uci()!=exp and "mateIn1" in self.puzzle_meta.get("themes", []):
                probe = self.board.copy(stack=False)
                probe.push(m)
                alternate_mate = probe.is_checkmate()

            if m.uci()!=exp and not alternate_mate:
                self.puzzle_had_mistake = True
                self.puzzle_hint_square = None
                self.setstatus("Not the puzzle move. Try again, use Hint, or view Answer.")
                return

            self.puzzle_hint_square = None
            self.board.push(m); self.last=m

            if alternate_mate:
                self.puzzle_i=len(self.puzzle_solution)
                self.setstatus("Solved! Checkmate.")
                return

            self.puzzle_i+=1

            # The solution alternates player move / opponent reply.
            # Play exactly one opponent reply, then return control to solver.
            if self.puzzle_i<len(self.puzzle_solution):
                r=chess.Move.from_uci(self.puzzle_solution[self.puzzle_i])
                if r not in self.board.legal_moves:
                    self.setstatus(
                        f"Puzzle data mismatch: reply {r.uci()} is illegal"
                    )
                    return
                self.board.push(r); self.last=r; self.puzzle_i+=1

            if self.puzzle_i>=len(self.puzzle_solution):
                self.setstatus("Solved! Tap Next for another Lichess puzzle.")
            else:
                # Defensive check: after the automatic reply it should be
                # the solver's color again.
                if self.board.turn != self.puzzle_color:
                    self.setstatus("Puzzle turn mismatch detected.")
                else:
                    self.setstatus("Correct. Continue.")

    def draw_board(self):
        for dy in range(8):
            for dx in range(8):
                sq=self.disp_to_sq(dx,dy)
                col=LIGHT if (dx+dy)%2==0 else DARK
                if self.last and sq in (self.last.from_square,self.last.to_square): col=LAST
                if sq==self.selected: col=SEL
                r=pygame.Rect(dx*SQ,dy*SQ,SQ,SQ)
                pygame.draw.rect(self.sc,col,r)
                if self.screen == "puzzle_game" and sq == self.puzzle_hint_square:
                    pygame.draw.rect(self.sc,HINT,r,4)
                p=self.board.piece_at(sq)
                if p:
                    key=("w" if p.color else "b")+{1:"p",2:"n",3:"b",4:"r",5:"q",6:"k"}[p.piece_type]
                    self.sc.blit(self.pieces[key],self.pieces[key].get_rect(center=r.center))
        if self.selected is not None:
            for m in self.board.legal_moves:
                if m.from_square==self.selected:
                    dx,dy=self.sq_to_disp(m.to_square)
                    pygame.draw.circle(self.sc,(55,55,55),(dx*SQ+20,dy*SQ+20),5)

    def fmt(self,ms):
        if ms is None: return "--:--"
        ms=max(0,int(ms))
        if ms < 20000:
            s=ms/1000.0
            return f"{int(s)//60:02d}:{int(s)%60:02d}.{(ms%1000)//100}"
        s=ms//1000
        return f"{s//60:02d}:{s%60:02d}"

    def draw_home(self):
        self.sc.fill(BG)
        self.sc.blit(self.txt("Chess Handheld",self.f24),(18,16))
        self.sc.blit(self.txt("Mac simulator · future ESP32-C5 UI",self.f15,MUTED),(18,48))

        # Clickable network status pill; this mirrors the future Wi-Fi status area.
        net_rect = pygame.Rect(348,14,114,32)
        net_col = GOOD if self.network_state=="online" else (HINT if self.network_state=="checking" else BAD)
        pygame.draw.rect(self.sc,PANEL,net_rect,border_radius=8)
        pygame.draw.circle(self.sc,net_col,(360,30),5)
        label = "Online" if self.network_state=="online" else ("Checking" if self.network_state=="checking" else "Offline")
        self.sc.blit(self.txt(label,self.f12),(370,22))

        cards=[
            (pygame.Rect(18,88,140,110),"Online","Random player"),
            (pygame.Rect(170,88,140,110),"AI","Level 1–8"),
            (pygame.Rect(322,88,140,110),"Puzzle","Lichess API"),
        ]
        for r,a,b in cards:
            pygame.draw.rect(self.sc,PANEL,r,border_radius=12)
            self.sc.blit(self.txt(a,self.f18),(r.x+12,r.y+18))
            self.sc.blit(self.txt(b,self.f12,MUTED),(r.x+12,r.y+49))
        self.button(pygame.Rect(322,216,140,34),"Network")
        self.sc.blit(self.txt(self.status,self.f12,MUTED),(18,280))
        acct=(self.api.account or {}).get("username","offline")
        self.sc.blit(self.txt(f"Account: {acct}",self.f12,MUTED),(18,300))

    def draw_selector_screen(self,kind):
        self.sc.fill(BG)
        title={"ai":"AI game","online":"Online match","puzzle":"Puzzle"}[kind]
        self.sc.blit(self.txt(title,self.f24),(18,15))
        self.button(pygame.Rect(390,12,72,30),"Home")
        if kind=="ai":
            self.sc.blit(self.txt("AI strength",self.f15,MUTED),(18,58))
            for i in range(1,9):
                r=pygame.Rect(18+(i-1)*55,82,48,34)
                self.button(r,str(i),self.ai_level==i)
            self.sc.blit(self.txt("Time control",self.f15,MUTED),(18,138))
            opts=[(3,0),(5,3),(10,0),(15,10)]
            for i,o in enumerate(opts):
                self.button(pygame.Rect(18+i*108,164,96,36),f"{o[0]}+{o[1]}",self.ai_time==o)
            self.button(pygame.Rect(18,224,444,44),"Start AI game",enabled=bool(TOKEN),on=True)
        elif kind=="online":
            self.sc.blit(self.txt("Board API random seek (Rapid/Classical)",self.f15,MUTED),(18,58))
            opts=[(10,0),(10,5),(15,10),(30,0)]
            for i,o in enumerate(opts):
                self.button(pygame.Rect(18+i*108,92,96,36),f"{o[0]}+{o[1]}",self.online_time==o)
            self.button(pygame.Rect(18,151,210,38),"Casual",not self.online_rated)
            self.button(pygame.Rect(252,151,210,38),"Rated",self.online_rated)
            self.button(pygame.Rect(18,218,444,44),"Review & Continue",enabled=bool(TOKEN),on=True)
        else:
            self.sc.blit(self.txt("Difficulty relative to your puzzle rating",self.f15,MUTED),(18,58))
            opts=["easiest","easier","normal","harder","hardest"]
            for i,o in enumerate(opts):
                self.button(pygame.Rect(18+i*89,90,82,36),o,self.puzzle_diff==o)
            self.button(pygame.Rect(18,160,444,44),"Get next Lichess puzzle",on=True)
            self.sc.blit(self.txt("Works anonymously; puzzle:read gives account-aware selection.",self.f12,MUTED),(18,225))
        self.sc.blit(self.txt(self.status,self.f12,MUTED),(18,292))

    def draw_network(self):
        self.sc.fill(BG)
        self.sc.blit(self.txt("Network connection",self.f24),(18,15))
        self.button(pygame.Rect(390,12,72,30),"Home")

        col = GOOD if self.network_state=="online" else (HINT if self.network_state=="checking" else BAD)
        pygame.draw.circle(self.sc,col,(30,61),8)
        state = "Connected" if self.network_state=="online" else ("Checking..." if self.network_state=="checking" else "Disconnected")
        self.sc.blit(self.txt(state,self.f18),(48,50))
        self.sc.blit(self.txt(f"Wi-Fi: {self.current_ssid}",self.f15,MUTED),(18,76))

        route = f"proxy {self.network_proxy_label}" if self.network_uses_proxy else "direct"
        self.sc.blit(self.txt(f"Requests route: {route}",self.f15),(18,104))

        warm = f"{self.network_latency} ms" if self.network_latency is not None else "—"
        cold = f"{self.network_cold_latency} ms" if self.network_cold_latency is not None else "—"
        self.sc.blit(self.txt(f"Current route warm/cold: {warm} / {cold}",self.f15),(18,131))

        dwarm = f"{self.network_direct_latency} ms" if self.network_direct_latency is not None else "—"
        dcold = f"{self.network_direct_cold_latency} ms" if self.network_direct_cold_latency is not None else "—"
        self.sc.blit(self.txt(f"Direct warm/cold: {dwarm} / {dcold}",self.f15),(18,156))

        move_rtt = f"{self.move_post_rtt_ms} ms" if self.move_post_rtt_ms is not None else "—"
        stream_rtt = f"{self.move_stream_confirm_ms} ms" if self.move_stream_confirm_ms is not None else "—"
        self.sc.blit(self.txt(f"Last move POST / stream sync: {move_rtt} / {stream_rtt}",self.f15),(18,181))

        self.sc.blit(self.txt("Known Wi-Fi priority:",self.f12,MUTED),(18,211))
        y=229
        for profile in KNOWN_WIFI_PROFILES:
            current = (self.current_ssid == profile["ssid"])
            mark = "●" if current else "○"
            col2 = GOOD if current else MUTED
            self.sc.blit(self.txt(f"{profile['priority']}. {mark} {profile['ssid']}",self.f12,col2),(28,y))
            y += 17

        self.button(pygame.Rect(18,282,210,30),"Recheck + auto Wi-Fi",on=True)
        self.button(pygame.Rect(252,282,210,30),"Back")

    def draw_online_confirm(self):
        self.sc.fill(BG)
        self.sc.blit(self.txt("Confirm online match",self.f24),(18,15))
        self.sc.blit(self.txt("Please check these settings before matchmaking.",self.f15,MUTED),(18,50))
        pygame.draw.rect(self.sc,PANEL,pygame.Rect(18,80,444,144),border_radius=12)
        self.sc.blit(self.txt("Time control",self.f15,MUTED),(36,98))
        self.sc.blit(self.txt(f"{self.online_time[0]}+{self.online_time[1]}",self.f24),(36,119))
        self.sc.blit(self.txt("Game type",self.f15,MUTED),(220,98))
        self.sc.blit(self.txt("Rated" if self.online_rated else "Casual",self.f24),(220,119))
        self.sc.blit(self.txt("Network",self.f15,MUTED),(36,166))
        net = "Connected" if self.network_state=="online" else "Not connected"
        self.sc.blit(self.txt(net,self.f18, GOOD if self.network_state=="online" else BAD),(36,187))
        acct=(self.api.account or {}).get("username","—")
        self.sc.blit(self.txt(f"Account: {acct}",self.f15),(220,177))
        self.button(pygame.Rect(18,250,210,44),"Back")
        self.button(pygame.Rect(252,250,210,44),"Confirm & Search",
                    enabled=bool(TOKEN) and self.network_state=="online",on=True)

    def draw_online_wait(self):
        self.sc.fill(BG)
        self.sc.blit(self.txt("Finding opponent...",self.f24),(18,28))
        self.sc.blit(self.txt(f"{self.online_time[0]}+{self.online_time[1]}  ·  {'Rated' if self.online_rated else 'Casual'}",self.f18),(18,74))
        self.sc.blit(self.txt("Waiting for Lichess Board API matchmaking",self.f15,MUTED),(18,112))
        self.sc.blit(self.txt(self.status,self.f12,MUTED),(18,150))
        self.button(pygame.Rect(18,232,444,46),"Cancel search")

    def draw_puzzle_answer(self):
        self.sc.fill(BG)
        p=self.puzzle_meta
        self.sc.blit(self.txt("Puzzle answer",self.f24),(18,14))
        self.sc.blit(self.txt(f"#{p.get('id','')}  ·  rating {p.get('rating','?')}",self.f15,MUTED),(18,47))
        self.sc.blit(self.txt("Full solution steps",self.f18),(18,78))

        # Two columns if the line is long, keeping the 480×320 screen readable.
        steps=self.puzzle_answer_steps
        for i,step in enumerate(steps[:12]):
            col=0 if i<6 else 1
            row=i if i<6 else i-6
            x=18 + col*225
            y=112 + row*25
            self.sc.blit(self.txt(f"{i+1}. {step}",self.f15),(x,y))
        if len(steps)>12:
            self.sc.blit(self.txt(f"+ {len(steps)-12} more plies",self.f12,MUTED),(18,262))
        self.button(pygame.Rect(18,276,136,32),"Back to board")
        self.button(pygame.Rect(172,276,136,32),"View line",on=True)
        self.button(pygame.Rect(326,276,136,32),"Next puzzle",on=True)

    def draw_game(self):
        self.draw_board()
        pygame.draw.rect(self.sc,PANEL,(SIDE_X,0,160,320))
        x=330
        if self.screen=="game":
            self.sc.blit(self.txt("Online game",self.f18),(x,10))
            me=(self.api.account or {}).get("username","You")
            top=self.opponent if self.my_color==chess.WHITE else me
            bot=me if self.my_color==chess.WHITE else self.opponent
            live_w, live_b = self.current_clocks()
            tt=live_b if self.my_color==chess.WHITE else live_w
            bt=live_w if self.my_color==chess.WHITE else live_b
            self.sc.blit(self.txt(top,self.f15,MUTED),(x,48))
            self.sc.blit(self.txt(self.fmt(tt),self.f24),(x,68))
            self.sc.blit(self.txt(bot,self.f15,MUTED),(x,126))
            self.sc.blit(self.txt(self.fmt(bt),self.f24),(x,146))

            api_txt = "—" if self.move_post_rtt_ms is None else f"{self.move_post_rtt_ms}ms"
            stream_txt = "—" if self.move_stream_confirm_ms is None else f"{self.move_stream_confirm_ms}ms"
            self.sc.blit(self.txt(f"API {api_txt}",self.f12,MUTED),(x,184))
            self.sc.blit(self.txt(f"Sync {stream_txt}",self.f12,MUTED),(x,199))
            self.button(pygame.Rect(330,226,140,32),"Home")
            self.button(pygame.Rect(330,264,140,32),"Resign")
        else:
            p=self.puzzle_meta
            self.sc.blit(self.txt("Puzzle",self.f18),(x,10))
            self.sc.blit(self.txt(f"#{p.get('id','')}",self.f15,MUTED),(x,45))
            self.sc.blit(self.txt(f"Rating {p.get('rating','?')}",self.f18),(x,66))
            side = "White to move" if self.puzzle_color == chess.WHITE else "Black to move"
            self.sc.blit(self.txt(side,self.f15),(x,92))
            themes=", ".join(p.get("themes",[])[:3])
            y=116
            for chunk in [themes[i:i+19] for i in range(0,len(themes),19)][:3]:
                self.sc.blit(self.txt(chunk,self.f12,MUTED),(x,y)); y+=16
            shown_ply = self.puzzle_review_ply if self.puzzle_review_mode else self.puzzle_i
            label = "Review" if self.puzzle_review_mode else "Progress"
            self.sc.blit(self.txt(f"{label} {shown_ply}/{len(self.puzzle_solution)}",self.f15),(x,181))

            if self.puzzle_review_mode:
                self.button(pygame.Rect(330,210,66,28),"< Prev",enabled=self.puzzle_review_ply>0)
                self.button(pygame.Rect(404,210,66,28),"Next >",enabled=self.puzzle_review_ply<len(self.puzzle_solution))
                self.button(pygame.Rect(330,244,66,28),"Resume")
                self.button(pygame.Rect(404,244,66,28),"Answer")
            else:
                self.button(pygame.Rect(330,210,66,28),"Hint")
                self.button(pygame.Rect(404,210,66,28),"Answer")
                self.button(pygame.Rect(330,244,66,28),"< Prev",enabled=self.puzzle_i>0)
                self.button(pygame.Rect(404,244,66,28),"Next >",enabled=bool(self.puzzle_solution))

            self.button(pygame.Rect(330,278,66,28),"Home")
            self.button(pygame.Rect(404,278,66,28),"Next Puz",enabled=not self.puzzle_loading)
        self.sc.blit(self.txt(self.status[:24],self.f12,MUTED),(330,308))

    def click(self,pos):
        x,y=pos
        if self.screen=="home":
            if pygame.Rect(18,88,140,110).collidepoint(pos): self.screen="online"
            elif pygame.Rect(170,88,140,110).collidepoint(pos): self.screen="ai"
            elif pygame.Rect(322,88,140,110).collidepoint(pos): self.screen="puzzle"
            elif pygame.Rect(322,216,140,34).collidepoint(pos) or pygame.Rect(348,14,114,32).collidepoint(pos): self.screen="network"

        elif self.screen=="network":
            if pygame.Rect(390,12,72,30).collidepoint(pos) or pygame.Rect(252,272,210,36).collidepoint(pos):
                self.home()
            elif pygame.Rect(18,272,210,36).collidepoint(pos):
                self.refresh_network()

        elif self.screen in ("ai","online","puzzle"):
            if pygame.Rect(390,12,72,30).collidepoint(pos): self.home(); return
            if self.screen=="ai":
                for i in range(1,9):
                    if pygame.Rect(18+(i-1)*55,82,48,34).collidepoint(pos): self.ai_level=i
                for i,o in enumerate([(3,0),(5,3),(10,0),(15,10)]):
                    if pygame.Rect(18+i*108,164,96,36).collidepoint(pos): self.ai_time=o
                if pygame.Rect(18,224,444,44).collidepoint(pos): self.start_ai()
            elif self.screen=="online":
                for i,o in enumerate([(10,0),(10,5),(15,10),(30,0)]):
                    if pygame.Rect(18+i*108,92,96,36).collidepoint(pos): self.online_time=o
                if pygame.Rect(18,151,210,38).collidepoint(pos): self.online_rated=False
                if pygame.Rect(252,151,210,38).collidepoint(pos): self.online_rated=True
                if pygame.Rect(18,218,444,44).collidepoint(pos): self.screen="online_confirm"
            else:
                for i,o in enumerate(["easiest","easier","normal","harder","hardest"]):
                    if pygame.Rect(18+i*89,90,82,36).collidepoint(pos): self.puzzle_diff=o
                if pygame.Rect(18,160,444,44).collidepoint(pos):
                    if not self.puzzle_loading:
                        self.puzzle_loading=True
                        self.setstatus("Loading puzzle...")
                        self.api.next_puzzle(self.puzzle_diff)

        elif self.screen=="online_confirm":
            if pygame.Rect(18,250,210,44).collidepoint(pos): self.screen="online"
            elif pygame.Rect(252,250,210,44).collidepoint(pos): self.start_seek()

        elif self.screen=="online_wait":
            if pygame.Rect(18,232,444,46).collidepoint(pos): self.cancel_seek()

        elif self.screen=="puzzle_answer":
            if pygame.Rect(18,276,136,32).collidepoint(pos): self.screen="puzzle_game"
            elif pygame.Rect(172,276,136,32).collidepoint(pos):
                self.enter_puzzle_review(0)
            elif pygame.Rect(326,276,136,32).collidepoint(pos): self.request_next_puzzle()

        elif self.screen in ("game","puzzle_game"):
            if x<BOARD:
                self.click_board(x,y)
            elif self.screen=="game":
                if pygame.Rect(330,226,140,32).collidepoint(pos): self.home()
                elif pygame.Rect(330,264,140,32).collidepoint(pos): self.api.resign(self.game_id)
            else:
                if self.puzzle_review_mode:
                    if pygame.Rect(330,210,66,28).collidepoint(pos): self.puzzle_prev_step()
                    elif pygame.Rect(404,210,66,28).collidepoint(pos): self.puzzle_next_step()
                    elif pygame.Rect(330,244,66,28).collidepoint(pos): self.resume_puzzle_from_review()
                    elif pygame.Rect(404,244,66,28).collidepoint(pos): self.show_puzzle_answer()
                else:
                    if pygame.Rect(330,210,66,28).collidepoint(pos): self.show_puzzle_hint()
                    elif pygame.Rect(404,210,66,28).collidepoint(pos): self.show_puzzle_answer()
                    elif pygame.Rect(330,244,66,28).collidepoint(pos): self.puzzle_prev_step()
                    elif pygame.Rect(404,244,66,28).collidepoint(pos): self.puzzle_next_step()
                if pygame.Rect(330,278,66,28).collidepoint(pos): self.home()
                elif pygame.Rect(404,278,66,28).collidepoint(pos): self.request_next_puzzle()

    def run(self):
        run=True
        while run:
            while True:
                try: kind,data=EVQ.get_nowait()
                except queue.Empty: break
                if kind=="status": self.setstatus(data)
                elif kind=="network":
                    if data.get("ssid"):
                        self.current_ssid=data.get("ssid")
                    if data.get("wifi_auto_state"):
                        self.wifi_auto_state=data.get("wifi_auto_state")
                    self.network_proxy_label=data.get("proxy_label","unknown")
                    self.network_uses_proxy=bool(data.get("uses_proxy"))
                    self.network_direct_latency=data.get("direct_latency")
                    self.network_direct_cold_latency=data.get("direct_cold_latency")
                    if data.get("online"):
                        self.network_state="online"
                        self.network_latency=data.get("latency")
                        self.network_cold_latency=data.get("cold_latency")
                        self.network_detail=f"Lichess API warm TTFB · {self.network_latency} ms"
                    else:
                        self.network_state="offline"
                        self.network_latency=None
                        self.network_cold_latency=None
                        self.network_detail=data.get("error","Could not reach Lichess")
                elif kind=="event": self.process_event(data)
                elif kind=="start_gid": self.start_gid(data)
                elif kind=="game": self.process_game(data)
                elif kind=="move_result": self.handle_move_result(data)
                elif kind=="puzzle": self.process_puzzle(data)
                elif kind=="puzzle_error":
                    self.puzzle_loading=False
                    self.setstatus(data)
            for e in pygame.event.get():
                if e.type==pygame.QUIT: run=False
                elif e.type==pygame.KEYDOWN and e.key==pygame.K_ESCAPE: run=False
                elif e.type==pygame.MOUSEBUTTONDOWN and e.button==1: self.click(e.pos)
            if self.screen=="home": self.draw_home()
            elif self.screen in ("ai","online","puzzle"): self.draw_selector_screen(self.screen)
            elif self.screen=="network": self.draw_network()
            elif self.screen=="online_confirm": self.draw_online_confirm()
            elif self.screen=="online_wait": self.draw_online_wait()
            elif self.screen=="puzzle_answer": self.draw_puzzle_answer()
            else:
                self.sc.fill((0,0,0)); self.draw_game()
            pygame.display.flip()
            self.clock.tick(30)
        self.api.event_stop.set(); self.api.game_stop.set(); self.api.cancel_seek()
        pygame.quit()

if __name__=="__main__":
    App().run()
