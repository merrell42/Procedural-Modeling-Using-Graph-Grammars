The web version is the old deprecated version. Moving forward we should concentrate our efforts on the C++ version. To use this, open pmugg.html. This is purely frontend Javascript with no backend.

All the core functionality has been migrated to the C++ version, although this conversion wasn't perfect and there still are some things that are working better in the web version.

I wrote this code over about 8 years. I've gone through the C++ code and cleaned it up and added documentation, but I haven't done this for the web version. So there's a lot of confusing code. It parallels the C++ version, but most things have different names.

I actually have two different versions of the graph grammars and neither is actually called "graph grammar" in the code. One version is called "familyTree" and one is called "networkHierarchy". For familyTree uses transistor.js to apply the graph grammar. For networkHierarchy, it uses netTransistor.js. In netTransistor, I use a BSP-tree to efficiently check if any of the polygons intersect each other.

The networkHierarchy version is what I've based the C++ version on since the family tree version does not handle 3D shapes. However the family tree version is better in some ways. One thing is that you can bend the edges to create curved organic shapes. This still needs to be implemented in the C++ version