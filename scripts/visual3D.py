#!/usr/bin/env python3
import sys
import numpy as np
from matplotlib import pyplot as plt
import s3dlib.surface as s3d
import s3dlib.cmap_utilities as cmu

#— PARAMETERS —#
maxZ = 3
rez, dmn = 6, (-3, 3)
tks = [-2, -1, 0, 1, 2]
fiveCmap = cmu.stitch_color('r', 'orange', 'g', 'b', 'm')

#— SURFACE DEFINITIONS —#
def eSurface_A(xyz, a):
    x, y, z = xyz
    Z = y**2 - x**3 - a*x
    return x, y, np.clip(Z, -maxZ, maxZ)

def eSurface_B(xyz, b):
    x, y, z = xyz
    Z = (y**2 - x**3 - b) / x
    return x, y, np.clip(Z, -maxZ, maxZ)

#— ARGUMENT PARSING —#
param = sys.argv[1].lower() if len(sys.argv) > 1 else None

if param == 'a':
    mode = 'A'
    a = -2
elif param == 'b':
    mode = 'B'
    b = 1
else:
    # default to A‐mode with a = -1
    mode = 'A'
    a = -2

#— BUILD & MAP SURFACE —#
if mode == 'A':
    title = rf"$Z_{{a}} = y^2 - x^3 - a\,x,\quad a = {a}$"
    surface = s3d.PlanarSurface(rez, color='k').domain(dmn, dmn)
    surface.map_geom_from_op(lambda xyz: eSurface_A(xyz, a))
    contour_vals = tks
    cname = 'b'

else:
    title = rf"$Z_{{b}} = \dfrac{{y^2 - x^3 - b}}{{x}},\quad b = {b}$"
    # left half‐plane
    s1 = s3d.PlanarSurface(rez, color='k').domain((-3, -1e-4), dmn)
    s1.map_geom_from_op(lambda xyz: eSurface_B(xyz, b))
    # right half‐plane
    s2 = s3d.PlanarSurface(rez, color='k').domain((+1e-4, 3), dmn)
    s2.map_geom_from_op(lambda xyz: eSurface_B(xyz, b))
    # now combine
    surface = s1 + s2
    contour_vals = tks
    cname = 'a'

#— COMMON POSTPROCESSING —#
surface.clip(lambda c: np.abs(c[2]) < maxZ)
surface.shade().set_surface_alpha(.15)

contours = surface.contourLines(*contour_vals)
contours.map_cmap_from_op(lambda xyz: xyz[2], fiveCmap, cname=cname)

#— PLOTTING —#
fig = plt.figure(figsize=plt.figaspect(0.75))
ax = plt.axes(projection='3d', aspect='equal')
ax.set_title(title, fontsize='large')
ax.set(xlabel='X', ylabel='Y',
       xlim=dmn, ylim=dmn, zlim=dmn)

cbar = plt.colorbar(
    contours.stcBar_ScalarMappable(len(contour_vals), bgcolor='w'),
    ax=ax, shrink=0.6
)
cbar.set_label(contours.cname, rotation=0, labelpad=10, fontsize=20)
cbar.set_ticks(contour_vals)

ax.view_init(35, -40)
ax.add_collection3d(surface)
ax.add_collection3d(contours)
fig.tight_layout(pad=2)
plt.show()
