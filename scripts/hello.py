print("Hello, World!")

# simple loop test
k = 0
for i in range(100):
    for j in range(100):
        k += 1

print(k)

# math test (not yet working)
# import math
# print(1/2, 2**3, math.sin(math.pi/4), math.log(10,2))

# introspection test - what functions are available?
def dir(x):
    if "__doc__" in x:
        print(x.__doc__)
    members = [str(e) for e in x]
    members.sort()
    print(", ".join(members))

M = [m for m in MODULES]
M.sort()
for m in M:
    if m in ["parse", "tokenize", "encode", "py2bc"]:
        continue
    print("\nModule", m, ":")
    dir(MODULES[m])
