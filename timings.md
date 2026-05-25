# Timings

As we make changes to the code, we should measure the timings to understand how any new code improves or degrades the speed of the model generation.

I have measured the following timings on my machine in Unity for each of the following folder on May 25, 2026 2:00 PM MST. (I realize this is a crude imperfect way of doing things, but at least it's a start.)

| Folder            | Time (s) |
| ----------------- | -------- |
| 2D Basic Shapes   |     21.9 |
| 2D Branches       |      6.1 |
| 2D Networks       |     17.3 |
| 2D Regions        |     16.2 |
| 3D Complex Shapes |    116.2 |
| 3D Shapes         |     39.0 |

Here are the timings for the pmugg executable on each of the test cases that it runs:

| Grammar           | Time (ms) | Num Faces |
| ----------------- | --------- | --------- |
| Castle            |    1878.3 |       265 |
| Square (Filled)   |      16.1 |         4 |
| House             |     636.6 |        64 |
| Docks             |     286.6 |       163 |

