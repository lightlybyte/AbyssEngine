# Dynami 20.09 Auth Server - connects to game via HTTP API
from flask import Flask, request, jsonify
import json, os, hashlib, secrets

app = Flask(__name__)
app.secret_key = os.environ.get('FLASK_SECRET_KEY', secrets.token_hex(32))

DATA_DIR = os.path.join(os.path.dirname(__file__), 'data')
USERS_FILE = os.path.join(DATA_DIR, 'users.json')
SESSIONS_FILE = os.path.join(DATA_DIR, 'sessions.json')

os.makedirs(DATA_DIR, exist_ok=True)

def load_json(path, default):
    if not os.path.exists(path): return default
    try:
        with open(path, 'r') as f: return json.load(f)
    except: return default

def save_json(path, data):
    with open(path, 'w') as f: json.dump(data, f, indent=2)

users = load_json(USERS_FILE, {})
sessions = load_json(SESSIONS_FILE, {})

@app.route('/api/login', methods=['POST'])
def api_login():
    data = request.get_json() or {}
    username = data.get('username', '').strip()
    password = data.get('password', '')
    
    if username not in users:
        return jsonify({'ok': False, 'error': 'Invalid credentials'})
    
    stored_hash = users[username].get('password', '')
    if not stored_hash or stored_hash != hashlib.sha256(password.encode()).hexdigest():
        return jsonify({'ok': False, 'error': 'Invalid credentials'})
    
    token = secrets.token_hex(16)
    sessions[token] = {'username': username, 'created': str(os.time())}
    users[username]['last_login'] = str(os.time())
    save_json(USERS_FILE, users)
    save_json(SESSIONS_FILE, sessions)
    
    return jsonify({'ok': True, 'username': username, 'token': token})

@app.route('/api/logout', methods=['POST'])
def api_logout():
    token = request.headers.get('Authorization', '').replace('Bearer ', '')
    if token in sessions:
        del sessions[token]
        save_json(SESSIONS_FILE, sessions)
    return jsonify({'ok': True})

@app.route('/api/me')
def api_me():
    token = request.headers.get('Authorization', '').replace('Bearer ', '')
    if token not in sessions:
        return jsonify({'error': 'Not authenticated'}), 401
    return jsonify({'username': sessions[token]['username'], 'authenticated': True})

@app.route('/api/register', methods=['POST'])
def api_register():
    data = request.get_json() or {}
    username = data.get('username', '').strip()
    password = data.get('password', '')
    
    if len(username) < 3 or len(username) > 20:
        return jsonify({'ok': False, 'error': 'Username 3-20 chars'})
    if len(password) < 6:
        return jsonify({'ok': False, 'error': 'Password min 6 chars'})
    if username in users:
        return jsonify({'ok': False, 'error': 'Username taken'})
    
    users[username] = {
        'password': hashlib.sha256(password.encode()).hexdigest(),
        'role': 'player',
        'created': str(os.time()),
        'last_login': None
    }
    save_json(USERS_FILE, users)
    return jsonify({'ok': True, 'username': username})

@app.route('/api/status')
def api_status():
    return jsonify({'status': 'online', 'users': len(users), 'version': '1.0'})

if __name__ == '__main__':
    port = int(os.environ.get('PORT', 5000))
    print(f'[Dynami 20.09 Auth] Server starting on port {port}')
    app.run(host='0.0.0.0', port=port, debug=False)
