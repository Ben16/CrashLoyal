Crash Loyal is a clone of a famous moble app game. This project is not to be
used as a comercial product, simply as a teaching tool for 4150 Game AI.

My Approach
I tried to get things working a little bit at a time. I first dealt with enemy
collisions, making sure that they could not overlap with each other. To implement
this, I calculated which unit was heavier and the distance they would need to move
to not overlap, and made the vector in the direction of the movement, but of magnitude
equal to the time delta.
For building collisions, I took the same approach but assumed the mobs would
always be pushed by the tower (and not vice versa)
For the river, since it was not a structure but rather a series of constants, I had to do
math to see whether each unit was in a river or on a bridge and then move the unit accordingly.

What works well
Units seem to collide with everything well! They move off each other quickly but without teleporting.

What I'd like to improve
Units overlap on the corners of their sprites - I believe this is due to a slight discrepency between
GetSize() and the actual size of the unit. While I could multiply the distance required to move to
account for this, I believe changing GetSize() is the more correct solution, and so I believe it's not
truly part of the movement code.
Additionally, units have difficulty moving around each other and will get "stuck" on the back corners of
other units when trying to pass. I believe this is due in part to the pathfinding - units begin to pass,
but then path sideways toward the path, pushing them back and to the side again, making them stuck.
I'm not sure what the fix would be for this - perhaps an algorithm that somehow detects this behavior
and makes the offset only horizontal, not vertical.
