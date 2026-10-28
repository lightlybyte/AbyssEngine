import json
from glb_reader import read_glb

gltf, _ = read_glb("example/test.glb")

for i, m in enumerate(gltf["materials"]):
    print(f"\n=== material[{i}] {m.get('name')} ===")
    print("pbrMetallicRoughness:")
    for k, v in m.get("pbrMetallicRoughness", {}).items():
        print(f"  {k}: {v}")
    print("extensions:")
    for k, v in m.get("extensions", {}).items():
        print(f"  {k}:")
        for kk, vv in v.items():
            print(f"    {kk}: {vv}")

input()