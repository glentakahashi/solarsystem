Assignment 2: Glen Takahashi, UID: 704004642

Requirements 1,2,3,4,5,6,7 and Extra Credit 1,2 are satisfied.

Built with g++ 4.7.2 on Debian 7

Requirements:
1. I added a keybind so when you press 'q', it calls exit(0);
2. I used some logic found in the book to subdivide dodecahedrons. I altered the logic so it would generate spheres based on the complexity that you pass into it.
3. I had the code generate both types of normals for each vertex so i could quickly switch between the two.
4. I made my own Satellite class, which can have satellites orbiting them. I made one called Sun which had no velocity and added all my planets and moons to it.
5. Using my Satellite class, I easily added each satellite to my solar system.
6. I could easily reuse my code from last time that already had this implemented.
7. I implemented phong and gouraud shading using my vshader and fshader. It computes the colors in the fshader and vshader respectively.

Extra credit:
1. Using my satellite model, I can add satellites to one another, which simply add another translation and rotation on top of it.
2. Using my arrow keys, you can attach yourself to the top of any satellite, including moons, and use the 'D' key to toggle locking at the sun.

Also, I was able to get it to rotate not on the same plane. By using some vetex math I was able to rotate planets around any axis that I wanted. The mud planet is rotating around the sun at a 30 degree angle.
