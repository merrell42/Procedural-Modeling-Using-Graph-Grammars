# PMUGG: Procedural Modeling Using Graph Grammars

The code is very poorly documented. I’ve been working on this alone for many years. It’s still just in the proof-of-concept stage. It contains code I wrote over a decade ago and much of the old code hasn’t been cleaned up. This file is only documentation at a high level.

There are two parts to the paper I wrote. The first part is to extract a graph grammar from an input shape. The second part is to use a graph grammar to generated shape. This code only includes the second part.

This is currently built as a web page entirely on the frontend with no backend code. I have a demo of this project at http://demo.paulmerrell.org/.

I currently only have this implemented as a website, but I'm planning to create a C++ version that can be used in various game engines.

## Using the Rules

We apply a rule and find the positions of the new vertices and edges. This requires some linear algebra to find the space of possible solutions. We then pick solutions from the space of possible solutions and reject them if they cause self-intersections.

There are two different versions of this. One for familyTree and one for networkHierarchy. For familyTree, this is in transistor.js. For the networkHierarchy, it is in netTransistor.js. In netTransistor, I use a BSP-tree to efficiently check if any of the polygons intersect each other.

There are some things I’ve implemented in 2D, but not in 3D. For 2D shapes, I can bend the edges to create curved organic shapes. I still need to implement this in 3D.

This is the part of the algorithm where speed is pretty important. I imagine the other parts would usually be done as a preprocessing step. I have some ideas for parallelizing the algorithm by modifying different parts of the shape in parallel without interfering with one another.

One thing that’s missing that is super important is to give the user more control over the results. Right now it just randomly applies rules. It’s easy to combine this with some sort of cost function and have it accept or reject each change based on the cost function. I’ve done that, but I haven’t really done much with the cost function yet. What I’d like to do is combine this with a set of brushes. The brush would tell the program what shapes you want. You pick the brush and wherever you draw the shapes appear that you want. One brush might draw a highway, another a bunch of skyscrapers, another a bunch of farms and the shapes would all connect together intelligently. I imagine artists would design lots of different brushes for different purposes. Most users wouldn’t even think about the earlier steps in the processes they would just be using brushes. I think the brushing system won’t be hard to implement, but I worry about speed. If the algorithm is fast enough it will work great, but it’s slow it won’t feel responsive.

## Decoration

The output is a set of polygons, edges, and vertices. As a post processing step, we can easily apply different textures and decorations to the faces, edges, and vertices. This is easy to do, but it makes a big difference. The way I currently do this is I generate an XML file in exporter.js which I copy as a string and save as I file. Then I have a script in 3DS Max that I use to generate the shapes from the XML. I use 3DS Max for all my 3D modeling and rendering. It is an expensive program.

I think I probably should implement this in a game engine as well as the part where we use the rules.