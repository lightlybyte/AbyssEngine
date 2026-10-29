# DATS Python Console Module
# Provides commands exposed to the embedded Python runtime and
# a simple in-game console REPL backed by the engine.

import sys


def echo(text):
    """Print text to the engine console."""
    print(f"[PY] {text}")
    return text


def deploy():
    """Handle the DEPLOY action from Python."""
    print("[PY] Deploying to server...")
    return "DEPLOYING"


def quit_game():
    """Handle the QUIT action from Python."""
    print("[PY] Quitting game...")
    return "QUITTING"


def get_status():
    """Return current game status string."""
    return "DATS v1.0 - Tactical Operations Command"


def run_console():
    """
    Simple Python REPL for the in-game console.
    Reads lines from stdin until 'exit()' or EOF.
    """
    print("[PY] DATS Python Console (type exit() to return)")
    print("[PY] Commands: echo(text), deploy(), quit_game(), get_status()")
    while True:
        try:
            line = input("py> ")
        except EOFError:
            print()
            break
        if not line.strip():
            continue
        if line.strip() in ("exit()", "quit()"):
            break
        try:
            result = eval(line, {"__builtins__": {}},
                          {"echo": echo, "deploy": deploy,
                           "quit_game": quit_game, "get_status": get_status,
                           "print": print})
            if result is not None:
                print(result)
        except Exception as exc:
            print(f"[PY] ERROR: {exc}")
