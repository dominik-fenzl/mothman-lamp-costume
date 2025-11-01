import math

# everything in cm
H = 10.0
T = 18.0
B = 40.0
R = 15.0

R = math.sqrt((0.5*B - 0.5*T)**2 + H**2)
L = math.pi * T
M = math.pi * B

P = (L*R)/(M-L)
Q = P + R

alpha = M / Q


print(f"Draw a circle of radius {Q:.2f} cm")
print(f"Draw a smaller concentric circle with radius {P:.2f} cm")
print(f"Draw the sector angle θ={math.degrees(alpha):.2f}∘(or measure {alpha:.2f} rad)")
print(f"Cut along the two arcs and along the radii edges")
print(f"Roll into the truncated cone: small radius = top, large radius = bottom")

