import os, sys, subprocess

REPL = """
=================================
AbyssEngine/D.A.T.S BUILD SYSTEM
=================================

[ 1 ] Build (Full Wipe and Rebuild)
[ 2 ] Clear Build Directory
[ 3 ] Build (Incremental Build)
[ 4 ] Configuration
[ 5 ] Exit

"""

while True:
    print(REPL)
    Choice = input(">>>")
    print()
    BuildChoice = input("What is your Build Choice (Linux, Windows): ")
    
    if Choice == "1":
        BuildDir = os.path.join("Build", BuildChoice)
        if os.path.exists("Build"):
            import shutil
            shutil.rmtree("Build")
        os.makedirs(BuildDir, exist_ok=True)
        print(f"Build directory set to: {BuildDir}")
    elif Choice == "2":
        if os.path.exists("Build"):
            import shutil
            shutil.rmtree("Build")
            print("Build directory cleared")
        else:
            print("Build directory does not exist")
    elif Choice == "3":
        BuildDir = os.path.join("Build", BuildChoice)
        print(f"Incremental build from: {BuildDir}")
    elif Choice == "4":
        print("Configuration mode - not implemented")
    elif Choice == "5":
        break
    else:
        print("Invalid choice, please try again")