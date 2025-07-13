# PMUGG: Procedural Modeling Using Graph Grammars

This project is a way to procedurally generate large complex shapes using graph grammars. The code is written in C++ and is used as a plugin to generate shapes in Unity, Unreal Engine and Godot. There is also a web version written in Javascript.

## How to Use

This works as a plugin in Unity, Unreal Engine, and Godot. In each of these game engines, there is a simple editor you can use to try to this out. In the editor, you can load a graph grammar from a file. These files are in the [grammar data directory](https://github.com/merrell42/PMUGG/tree/main/grammar%20data).

![editor screenshot](editor%20screenshot.png)

If you hit the play button, it will generate shapes according to the graph grammar. You can also click the step button to go through each step individually. You can also click the load folder button. This will load all the graph grammars in a folder and will cycle through them. This is a nice way to quickly test many different grammars. I also have a web version.

## References

This is based on my SIGGRAPH paper. Please cite this if you use the code. The paper explains the theory behind this, puts this in context of other work, and explains how to generate graph grammars:

&nbsp;&nbsp;&nbsp;&nbsp; Paul Merrell. 2023. [Example-Based Procedural Modeling Using Graph Grammars](https://paulmerrell.org/wp-content/uploads/2023/08/ProcModelUsingGraphGram.pdf). ACM Transactions on Graphics. ([project](https://paulmerrell.org/grammar/), [video](https://www.youtube.com/watch?v=FG3LbcOGHqw))

And if you’ve heard of the Wave Function Collapse (WFC) algorithm you should know that it is based on my PhD work:

&nbsp;&nbsp;&nbsp;&nbsp; Paul Merrell. 2009. [Model Synthesis](https://paulmerrell.org/wp-content/uploads/2021/06/thesis.pdf).  Ph.D. Dissertation.
([project](https://paulmerrell.org/model-synthesis/), [video](https://www.youtube.com/watch?v=A2ODauA1a0M))

## Code Overview

[Graph grammars](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_grammar.h) operate on graphs. A [graph](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph/graph.h) is just a set of [vertices](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph/graph_vertex.h) and [edges](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph/graph_edge.h). If we assign a position to every vertex and every edge, this is called a [graph drawing](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_drawing/graph_drawing.h). We start with an empty graph drawing and then modify it in small incremental steps using a graph grammar.

At each step, we apply a rule from the grammar. The grammar is just a list of [production rules](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/production_rule.h). We pick one of the rules and apply it to the graph drawing. Each rule consists of a left graph and a right graph. To apply the rule, you [match](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_morphism/morphism_finder.h) the left graph to part of the graph drawing. Then you remove the left graph and replace it with the right graph. By applying different rules we can generate a rich variety of complex shapes.

## Matching Graphs

Now let's look at this process in more detail. There are a few different kinds of rules. In [most rules](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_grammar.h#L49), there is a left graph and a right graph that are both not empty. But in a [starter rule](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_grammar.h#L47), the left graph is empty. We are creating something from nothing. And a [ground rule](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_grammar.h#L52) is just used at the beginning to create the ground plane.

At each step, we [randomly pick](https://github.com/merrell42/PMUGG/blob/main/cpp_version/mutator.cpp#L47) one of the production rules in the grammar and try to [apply it](https://github.com/merrell42/PMUGG/blob/main/cpp_version/mutator.cpp#L95). We apply a rule by [matching one side](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_morphism/morphism_finder.h) of the rule to part of the graph drawing.

We start by matching [one vertex](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_morphism/morphism_finder.cpp#L30) in the left graph to the graph drawing. Each vertex and each edge has a label or a type assigned to it. So we search for vertices with the [same type](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_morphism/morphism_finder.cpp#L65). Once we [match one vertex](https://github.com/merrell42/PMUGG/blob/main/cpp_version/graph_morphism/morphism_finder.cpp#L88) we examine its neighboring edges and vertices and try to find matches for them. We continue to match edges and vertices until we've matched the entire graph.

This isn’t always successful. For a given starting location, we may not find a match. So we keep trying. We try different starting locations and different production rules until we find a match.

Once a match is found we replace the left graph with the right graph. We first [cut out](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/rule_applier.cpp#L133) the left graph. We find each edge along the outer boundary of the graph and we cut that edge into two half edges. Then [we glue](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/rule_applier.cpp#L103) the right graph in its place. We find each half-edge that was cut and glue it onto the right graph. The right graph always fits right into where the left graph was removed.

The left graph may contain [splices](https://github.com/merrell42/PMUGG/blob/main/cpp_version/primitives/edge_type.h#L46). Splices allow us to apply changes to disconnected parts of the graph drawing. The left graph may contain spliced edges. These represent [rays that we shoot](https://github.com/merrell42/PMUGG/blob/934fbf3d827c1beb1ab749950cc72a1cf8efa82d/cpp_version/graph_morphism/morphism_finder.h#L63) through empty space to try to find the parts we need to fully match the left graph.

## Setting the Positions

Next, we need to [figure out](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/rule_applier.cpp#L362) the position of the vertices we've added to the graph. Our initial strategy will be to leave the existing vertices where they are. We don’t want to move them if we don’t have to. But we still need to set the position of the new vertices.

We can write down a list of equations describing the position of the vertices. Every edge connects two vertices and we know the direction of each edge. For each edge we have a linear equation that constrains the position of its two vertices. This gives us one equation. We can write down all the equations for all the edges and put these equations into a matrix.

This is a linear system of equations. These equations could have many solutions or they could have none. If they have no solutions this means our initial strategy of not moving the other vertices will not work. Those vertices were fixed in place. If we allow some of these vertices to [freely move](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/rule_applier.cpp#L748), this will give us a new set of equations that are not so tightly constrained and are more likely to have a solution.

On the other hand, this set of equations could have many solutions and we need to choose between them. One strategy is to find the null space of the matrix and pick a point in the null space, but that method is slow and there's a faster method that's equivalent.

[A better strategy](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/rule_applier.cpp#L630) is to go through each face that can freely move and determine the position of the place it sits on. Once we have set the positions of [three planes](https://github.com/merrell42/PMUGG/blob/main/cpp_version/placements/vertex_placement.cpp#L51) that touch a vertex you have fixed the position of the vertex. We continue to set the positions of the  planes until every vertex has a fixed position.

Each time we fix the position of a plane, we have [a range](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/rule_applier.cpp#L572) of acceptable values to choose from. This range is based on ensuring that the vertices stay within [a desired volume](https://github.com/merrell42/PMUGG/blob/main/cpp_version/placements/vertex_placement.cpp#L160) and ensuring the edges are within a [desired length](https://github.com/merrell42/PMUGG/blob/main/cpp_version/placements/edge_placement.cpp#L94). We pick a random value within this range. We also need to check that the edges and faces do not [self-intersect](https://github.com/merrell42/PMUGG/blob/main/cpp_version/grammar_rules/rule_applier.cpp#L827) themselves. We do this using rejection sampling. We randomly pick a possible solution. We reject a solution if it self-intersects and keep picking solutions until we find a good one.

## Future of the Project

This project is just getting started. With some additional work, this could be much more powerful. The [road map](https://github.com/merrell42/PMUGG/blob/934fbf3d827c1beb1ab749950cc72a1cf8efa82d/Road%20Map.md) has a list of short-term and long-term goals for the project. We are looking for people to help and collaborate with.
