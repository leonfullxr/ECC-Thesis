#!/usr/bin/env python3
import numpy as np
import matplotlib

# 1) Force a GUI‐capable backend; change if you need (e.g. 'tkagg', 'qt5agg', etc.)
matplotlib.use('qt5agg')
import matplotlib.pyplot as plt
import time

from qutip import Bloch, basis

# --- Parameters for your state vector:
theta = np.pi/4   # polar angle
phi   = np.pi/3   # azimuthal angle

# Build the state |ψ> = cos(θ/2)|0> + e^{iφ} sin(θ/2)|1>
psi = np.cos(theta/2)*basis(2, 0) + np.exp(1j*phi)*np.sin(theta/2)*basis(2, 1)

# Create the Bloch sphere
b = Bloch(figsize=(6,6))

# Style the vector & marker
b.vector_color = ['r']
b.vector_width = 4
b.point_marker = ['o']
b.point_color  = ['r']
b.point_size   = [30]

# Add state + |0>,|1> annotations
b.add_states(psi)
b.add_annotation([0,0,1.1],  r'$\hat z = |0\rangle$')
b.add_annotation([0,0,-1.1], r'$-\hat z = |1\rangle$')

# Make sphere semi‐transparent, choose camera angle
b.frame_alpha = 0.1
b.view        = [45, 30]
b.font_size   = 16

# Render into a Matplotlib window
b.show()

# 2) Save a PNG in case you want a file snapshot
out_file = 'bloch_sphere.png'
b.save(out_file)
print(f"Bloch sphere saved as {out_file}")

# 3) Now keep that window alive and processing events until you close it:
fig = plt.gcf()                # “current” figure
print("Close the Bloch sphere window to exit.")
while plt.fignum_exists(fig.number):
    plt.pause(0.1)             # process GUI events

# After you close the window, the script will finally exit.
