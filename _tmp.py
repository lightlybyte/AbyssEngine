import sys  
f=open(r\"c:/AbyssEngine/source/main.cpp\",\"rb\")  
data=f.read()  
f.close()  
print(repr(data[:200]))  
