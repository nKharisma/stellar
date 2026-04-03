### Stellar: Starfield Simulation

Written by me: SeNiah McField


This is a real-time starfield simulation built with C++ and OpenGL 4.6, inspired by my Computer Graphics course which utilizes Python. I chose to build this in C++ to challenge myself with lower-level performance and memory management. I spent time researching how to optimize N-body physics for real-time rendering, eventually implementing an inverse-cube shortcut and a softening factor to keep the simulation stable even when stars overlap. The project renders 500 stars with mass-based colors, layered over a procedural nebula background I wrote for the shaders. This project makes use of libraries GLAD, GLFW, and GLM, but the core simulation logic, the shader implementation, and the vertex buffer management are all my original work.