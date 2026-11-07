"""
Abyss Engine / DATS Website Auth Server
=======================================
A monochrome Flask authentication server backed up to JSON.
- Runs on port 80
- User accounts with profile pictures
- Owner account: lightlybyte / DivineAsiegbu123
- Accounts are backed up to users.json (and users.json.bak)
"""

import os
import json
import shutil
import base64
import hashlib
import threading
from datetime import datetime, timezone
from pathlib import Path

from flask import (
    Flask, render_template, request, redirect, url_for,
    session, jsonify, send_from_directory, abort
)
from werkzeug.utils import secure_filename

# ---------------------------------------------------------------------------
# Paths & constants
# ---------------------------------------------------------------------------
BASE_DIR    = Path(__file__).resolve().parent
USERS_FILE  = BASE_DIR / "users.json"
BACKUP_FILE = BASE_DIR / "users.json.bak"
AVATAR_DIR  = BASE_DIR / "static" / "avatars"

USERS_FILE.parent.mkdir(parents=True, exist_ok=True)
AVATAR_DIR.mkdir(parents=True, exist_ok=True)

ALLOWED_EXTENSIONS = {"png", "jpg", "jpeg", "gif", "webp"}
MAX_AVATAR_BYTES   = 2 * 1024 * 1024  # 2 MB

OWNER_USERNAME = "lightlybyte"
OWNER_PASSWORD = "DivineAsiegbu123"

# ---------------------------------------------------------------------------
# Account storage helpers (JSON + backup)
# ---------------------------------------------------------------------------

def _hash_password(password: str, salt: str | None = None) -> str:
    salt = salt or base64.b64encode(os.urandom(16)).decode("ascii")
    pw_hash = hashlib.pbkdf2_hmac("sha256", password.encode("utf-8"),
                                  salt.encode("utf-8"), 200_000)
    return base64.b64encode(pw_hash).decode("ascii") + "$" + salt


def _verify_password(stored: str, password: str) -> bool:
    try:
        h, s = stored.rsplit("$", 1)
        return _hash_password(password, s) == stored
    except Exception:
        return False


def _default_users():
    """Seed the database with the owner account."""
    return {
        OWNER_USERNAME: {
            "username": OWNER_USERNAME,
            "password_hash": _hash_password(OWNER_PASSWORD),
            "role": "owner",
            "avatar": None,
            "created_at": datetime.now(timezone.utc).isoformat() + "Z",
            "last_login": None,
        }
    }


def load_users():
    """Load accounts from JSON, restoring from backup if needed."""
    if USERS_FILE.exists():
        try:
            with USERS_FILE.open("r", encoding="utf-8") as f:
                data = json.load(f)
            if isinstance(data, dict) and data:
                return data
        except (json.JSONDecodeError, OSError):
            pass
    if BACKUP_FILE.exists():
        try:
            shutil.copyfile(BACKUP_FILE, USERS_FILE)
            with USERS_FILE.open("r", encoding="utf-8") as f:
                return json.load(f)
        except Exception:
            pass
    return _default_users()


def save_users(users):
    """Atomically write accounts to JSON with a backup."""
    tmp = USERS_FILE.with_suffix(".json.tmp")
    with tmp.open("w", encoding="utf-8") as f:
        json.dump(users, f, indent=2, ensure_ascii=False)
    if USERS_FILE.exists():
        shutil.copyfile(USERS_FILE, BACKUP_FILE)
    shutil.move(str(tmp), str(USERS_FILE))


def allowed_file(filename: str) -> bool:
    return "." in filename and filename.rsplit(".", 1)[1].lower() in ALLOWED_EXTENSIONS

# ---------------------------------------------------------------------------
# Flask app
# ---------------------------------------------------------------------------
app = Flask(__name__)
app.secret_key = os.environ.get("ABYSS_AUTH_SECRET", "abyss-dev-secret-change-me")
app.config["MAX_CONTENT_LENGTH"] = MAX_AVATAR_BYTES + 1024

_users_lock = threading.RLock()
USERS = load_users()


def current_user():
    return session.get("username")


def is_owner(username: str) -> bool:
    return username == OWNER_USERNAME

# ---------------------------------------------------------------------------
# Routes
# ---------------------------------------------------------------------------

@app.route("/")
def index():
    if "username" in session:
        return redirect(url_for("profile"))
    return redirect(url_for("login"))


@app.route("/login", methods=["GET", "POST"])
def login():
    error = None
    if request.method == "POST":
        username = (request.form.get("username") or "").strip()
        password = request.form.get("password") or ""
        with _users_lock:
            user = USERS.get(username)
        if user and _verify_password(user["password_hash"], password):
            session["username"] = username
            session["role"] = user.get("role", "user")
            with _users_lock:
                user["last_login"] = datetime.now(timezone.utc).isoformat() + "Z"
                save_users(USERS)
            return redirect(url_for("profile"))
        error = "Invalid username or password."
    return render_template("login.html", error=error)


@app.route("/logout")
def logout():
    session.clear()
    return redirect(url_for("login"))


@app.route("/register", methods=["GET", "POST"])
def register():
    error = None
    if request.method == "POST":
        username = (request.form.get("username") or "").strip()
        password = request.form.get("password") or ""
        confirm  = request.form.get("confirm") or ""
        if not username or not password:
            error = "Username and password are required."
        elif password != confirm:
            error = "Passwords do not match."
        elif " " in username:
            error = "Username cannot contain spaces."
        else:
            with _users_lock:
                if username in USERS:
                    error = "Username already exists."
                else:
                    USERS[username] = {
                        "username": username,
                        "password_hash": _hash_password(password),
                        "role": "user",
                        "avatar": None,
                        "created_at": datetime.now(timezone.utc).isoformat() + "Z",
                        "last_login": None,
                    }
                    save_users(USERS)
            if not error:
                session["username"] = username
                session["role"] = "user"
                return redirect(url_for("profile"))
    return render_template("register.html", error=error)


@app.route("/profile")
def profile():
    if "username" not in session:
        return redirect(url_for("login"))
    username = session["username"]
    with _users_lock:
        user = USERS.get(username)
    if not user:
        session.clear()
        return redirect(url_for("login"))
    avatar_url = url_for("static", filename="avatars/" + user["avatar"]) if user.get("avatar") else None
    return render_template("profile.html", user=user, avatar_url=avatar_url)


@app.route("/avatar/upload", methods=["POST"])
def upload_avatar():
    if "username" not in session:
        return jsonify({"error": "Not authenticated"}), 401
    username = session["username"]
    file = request.files.get("avatar")
    if not file or file.filename == "":
        return jsonify({"error": "No file provided"}), 400
    if not allowed_file(file.filename):
        return jsonify({"error": "Unsupported file type"}), 400
    file.seek(0, os.SEEK_END)
    size = file.tell()
    file.seek(0)
    if size > MAX_AVATAR_BYTES:
        return jsonify({"error": "File too large (max 2 MB)"}), 400

    filename = secure_filename(file.filename)
    ext = filename.rsplit(".", 1)[1].lower()
    safe_name = f"{username}.{ext}"
    path = AVATAR_DIR / safe_name
    file.save(path)

    with _users_lock:
        user = USERS.get(username)
        if user:
            old = user.get("avatar")
            if old and old != safe_name:
                try:
                    (AVATAR_DIR / old).unlink(missing_ok=True)
                except OSError:
                    pass
            user["avatar"] = safe_name
            save_users(USERS)
    return jsonify({"avatar_url": url_for("static", filename="avatars/" + safe_name)})


@app.route("/avatar/<filename>")
def avatar_file(filename):
    return send_from_directory(AVATAR_DIR, filename)

# JSON API used by the game client
@app.route("/api/login", methods=["POST"])
def api_login():
    data = request.get_json(silent=True) or {}
    username = (data.get("username") or "").strip()
    password = data.get("password") or ""
    with _users_lock:
        user = USERS.get(username)
    if user and _verify_password(user["password_hash"], password):
        session["username"] = username
        session["role"] = user.get("role", "user")
        with _users_lock:
            user["last_login"] = datetime.now(timezone.utc).isoformat() + "Z"
            save_users(USERS)
        return jsonify({"ok": True, "username": username, "role": user.get("role", "user")})
    return jsonify({"ok": False, "error": "Invalid credentials"}), 401


@app.route("/api/logout", methods=["POST"])
def api_logout():
    session.clear()
    return jsonify({"ok": True})


@app.route("/api/me")
def api_me():
    if "username" not in session:
        return jsonify({"authenticated": False}), 401
    username = session["username"]
    with _users_lock:
        user = USERS.get(username)
    if not user:
        return jsonify({"authenticated": False}), 401
    avatar_url = url_for("static", filename="avatars/" + user["avatar"]) if user.get("avatar") else None
    return jsonify({
        "authenticated": True,
        "username": user["username"],
        "role": user.get("role", "user"),
        "avatar_url": avatar_url,
        "created_at": user.get("created_at"),
        "last_login": user.get("last_login"),
    })


@app.route("/api/users")
def api_users():
    """Owner-only: list all accounts."""
    if session.get("username") != OWNER_USERNAME:
        return jsonify({"error": "Owner only"}), 403
    with _users_lock:
        out = [{
            "username": u["username"],
            "role": u.get("role", "user"),
            "created_at": u.get("created_at"),
            "last_login": u.get("last_login"),
            "has_avatar": bool(u.get("avatar")),
        } for u in USERS.values()]
    return jsonify({"users": out})


@app.route("/health")
def health():
    return jsonify({"status": "ok"})


# ---------------------------------------------------------------------------
# Run
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    # Ensure owner account exists even if users.json was deleted
    with _users_lock:
        if OWNER_USERNAME not in USERS:
            USERS[OWNER_USERNAME] = _default_users()[OWNER_USERNAME]
            save_users(USERS)
    app.run(host="0.0.0.0", port=80, debug=False, threaded=True)

