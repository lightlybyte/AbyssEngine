import server

rules = sorted(r.rule for r in server.app.url_map.iter_rules())
print("Flask routes:")
for r in rules:
    print(" ", r)

print("Owner account seeded:", "lightlybyte" in server.USERS)
