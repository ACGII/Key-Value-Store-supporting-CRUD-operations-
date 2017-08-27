# Key-Value-Store-supporting-CRUD-operations

Coursera Cloud Computing Concepts

University of Illinois at Urbana-Champaign

A key-value store supporting CRUD operations (Create, Read, Update, Delete)

Load-balancing (via a consistent hashing ring to hash both servers and keys)

Fault-tolerance up to two failures (by replicating each key three times to three successive nodes in the ring, starting from the first node at or to the clockwise of the hashed key)

Quorum consistency level for both reads and writes (at least two replicas).  Stabilization after failure (recreate three replicas after failure)

This project uses C++. (gcc version 4.7 and onwards).
