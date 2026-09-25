# Decorations

Decorations are listed in the grammar JSON under `decorations`, in the `vertex`, `edge`, and `face` arrays. A vertex, edge, or face type uses one by setting `"decoration"` to that decoration's `id`.

Every decoration has these parameters:

- `id` — Name other decorations use to refer to this one.
- `type` — Which decoration this is. The types are listed below.

A parameter marked optional can be left out. Everything else is required.

Child parameters (`child`, `childIds`, `edge`, `startVertex`, `endVertex`) are ids of other decorations. A child has to be the same kind the parent places: a vertex decoration, an edge decoration, or a face decoration.

## Vertex decorations

Vertex decorations run at a vertex, or at a point chosen by an edge or face decoration.

### Place object

Places a mesh instance.

- `object` — Asset id of the mesh to place.

### Rotate

Rotates its child by a random angle.

- `minAngle` — Minimum angle, in degrees.
- `maxAngle` — Maximum angle, in degrees.
- `child` — Vertex decoration to rotate.
- `axis` — Optional. Axis to rotate around. Defaults to `[0, 0, 1]`.

### Scale

Scales its child by a random amount.

- `min` — Minimum scale. A number scales all three axes together. An `[x, y, z]` array scales each axis separately.
- `max` — Maximum scale, in the same form as `min`.
- `child` — Vertex decoration to scale.

### Union

Runs every child.

- `childIds` — Optional. Ids of the vertex decorations to run. If omitted, the union does nothing.

### Pick random

Runs one child, chosen at random.

- `childIds` — Optional. Ids of the vertex decorations to choose from. If omitted, nothing is placed.
- `weight` — Optional. One weight per child, in the same order as `childIds`. If omitted, each child is equally likely.

## Edge decorations

Edge decorations run along an edge, or along a segment cut by a slice decoration.

### Space evenly

Places its child at even intervals along the edge. The gap between points is `spacing`, and the points are centered so the margins at both ends match.

- `spacing` — Distance between points.
- `child` — Vertex decoration to place.

### Space randomly

Places its child at random positions along the edge. The count is a Poisson sample whose mean is the edge length divided by `spacing`. Each position is uniform along the edge.

- `spacing` — Average distance between points.
- `child` — Vertex decoration to place.

### Extrude

Extrudes a 2D profile along the edge and adds the resulting mesh. Provide either `svg` or `profile`. If both are present, `svg` is used.

- `svg` — Name of an SVG file in the same folder as the grammar. `"square"` loads `square.svg`. The file needs a `<polygon>` or `<polyline>` `points` attribute. These points are the profile, in the same order as `profile`.
- `profile` — Profile as an array of `[x, y]` points. `x` is sideways from the edge and `y` is the other axis of the cross-section.
- `color` — Optional. `[red, green, blue]` color of the mesh. Defaults to white, `[1, 1, 1]`.
- `scale` — Optional. Multiplies the profile. `2` doubles it and `0.5` halves it. Defaults to `1`.

### Union

Runs every child.

- `childIds` — Optional. Ids of the edge decorations to run. If omitted, the union does nothing.

### Pick random

Runs one child, chosen at random.

- `childIds` — Optional. Ids of the edge decorations to choose from. If omitted, nothing is placed.
- `weight` — Optional. One weight per child, in the same order as `childIds`. If omitted, each child is equally likely.

## Face decorations

Face decorations run on a face. Scatter and grid skip hole faces.

### Scatter

Places its child at random points on the face. The count is a Poisson sample whose mean is `density` times the face area. Each point is chosen with probability proportional to area.

- `density` — Expected number of points per unit of area. `0.1` on a face of area 1 places 0.1 points on average.
- `child` — Vertex decoration to place.

### Grid

Places its child on a regular grid over the face. The grid follows the face type's `u` and `v` axes. Points are spaced by `spacing` and centered on the face, then kept only when they lie inside the face.

- `spacing` — Distance between grid points along each axis.
- `child` — Vertex decoration to place.

### Slice

Cuts the face with planes spaced evenly along `direction`. Each cut is a line segment. The planes use the same centered spacing as space evenly.

- `direction` — Direction to slice along, such as `[1, 0, 0]`.
- `spacing` — Distance between planes.
- `edge` — Optional. Edge decoration applied to each cut segment.
- `startVertex` — Optional. Vertex decoration applied to one endpoint of each segment.
- `endVertex` — Optional. Vertex decoration applied to the other endpoint.

`edge`, `startVertex`, and `endVertex` can each be left out. If all three are left out, the slice is not created.

### Union

Runs every child.

- `childIds` — Ids of the face decorations to run.

### Pick random

Runs one child, chosen at random.

- `childIds` — Optional. Ids of the face decorations to choose from. If omitted, nothing is placed.
- `weight` — Optional. One weight per child, in the same order as `childIds`. If omitted, each child is equally likely.
