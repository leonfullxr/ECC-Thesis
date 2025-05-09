import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import Circle, Ellipse, FancyArrowPatch, Arc

# --- Parameters for your state vector:
theta = np.pi/4   # polar angle
phi   = np.pi/3   # azimuthal angle

# Compute Bloch vector projection
bx = np.sin(theta)*np.cos(phi)
by = np.sin(theta)*np.sin(phi)

fig, ax = plt.subplots(figsize=(6,6))
ax.set_aspect('equal')
ax.axis('off')

# 1) Outer sphere
outer = Circle((0,0), 1.0, fill=False, linewidth=1.5)
ax.add_patch(outer)

# 2) Dashed equator (as a flattened ellipse for perspective)
equator = Ellipse((0,0), width=2.0, height=0.25, 
                  angle=0, fill=False, linestyle='--', linewidth=1.2)
ax.add_patch(equator)

# 3) Axes arrows
arrow_kw = dict(arrowstyle='-|>,head_length=6,head_width=4', linewidth=1.5, color='k')
# +Z
ax.add_patch(FancyArrowPatch((0,0),(0,1), **arrow_kw))
# +Y
ax.add_patch(FancyArrowPatch((0,0),(1,0), **arrow_kw))
# +X (tilted down-left by 150°)
x_dir = np.deg2rad(210)
ax.add_patch(FancyArrowPatch((0,0),
                             (np.cos(x_dir), np.sin(x_dir)),
                             **arrow_kw))

# 4) Bloch vector |ψ⟩
ax.plot([0, bx], [0, by], color='k', linewidth=1.2)
ax.scatter([bx], [by], s=30, color='k')

# 5) Dotted projection lines
ax.plot([bx, bx], [by, 0], linestyle=':', linewidth=1.0, color='k')
ax.plot([0, bx], [0, 0], linestyle=':', linewidth=1.0, color='k')

# 6) Angle arcs
# θ between +Z and vector
theta_arc = Arc((0,0), 0.4, 0.4, angle=0,
                theta1=90 - np.degrees(theta), theta2=90,
                linewidth=1.0)
ax.add_patch(theta_arc)
# φ between +X-proj and vector’s projection
# Here +X-proj is the dotted horizontal line pointing to the right,
# but we draw φ from the tilted x̂ axis to the projection vector.
phi_start = np.degrees(x_dir)
phi_arc = Arc((0,0), 0.7, 0.7,
              angle=0,
              theta1=phi_start,
              theta2=phi_start + np.degrees(phi),
              linewidth=1.0)
ax.add_patch(phi_arc)

# 7) Labels
ax.text(0, 1.05,   r'$\hat z = |0\rangle$', ha='center', va='bottom', fontsize=14)
ax.text(0, -1.05,  r'$-\hat z = |1\rangle$', ha='center', va='top',    fontsize=14)
ax.text(1.05, 0,   r'$\hat y$',          ha='left',   va='center', fontsize=14)
ax.text(np.cos(x_dir)*1.1, np.sin(x_dir)*1.1,
        r'$\hat x$', ha='center', va='center', fontsize=14)

ax.text(bx+0.03, by+0.03, r'$|\psi\rangle$', ha='left', va='bottom', fontsize=14)
ax.text(0.15, 0.15,        r'$\theta$',       ha='left', va='bottom', fontsize=12)
ax.text(0.5*np.cos(x_dir/2), 0.5*np.sin(x_dir/2)-0.05,
        r'$\phi$', ha='center', va='top', fontsize=12)

# 8) Limits
ax.set_xlim(-1.3, 1.3)
ax.set_ylim(-1.3, 1.3)

plt.show()
