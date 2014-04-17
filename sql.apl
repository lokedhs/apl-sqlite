#!apl -f

⍝  This file contains helper definititions to make the SQL APL more easy
⍝  to use. Example use:
⍝
⍝  db ← 'sqlite' SQL∆Connect '/path/to/db/file'
⍝
⍝    Connects to the database of type X with connect arguments Y.
⍝    This call returns the ID of the connection.
⍝
⍝  results ← 'select * from foo where a > ?' SQL∆Select[db] 100
⍝
⍝    Executes the query given in X with the bind parameters in Y.
⍝    The symbols to use for bind parameters depends on the inderlying
⍝    database. SQLite uses ?. PostgreSQL uses $1, $2, $3, etc...
⍝
⍝  SQL∆Disconnect db
⍝
⍝    Disconnects from the database connection given as X.


∇Z←X SQL∆Connect Y
Z←X SQL[1] Y
∇

∇Z←SQL∆Disconnect Y
Z←SQL[2] Y
∇

∇Z←X SQL∆Select[db] Y
Z←X SQL[3,db] Y
∇

∇Z←X SQL∆Exec[db] Y
Z←X SQL[4,db] Y
∇

∇Z←SQL∆Begin Y
Z←SQL[5] Y
∇

∇Z←SQL∆Commit Y
Z←SQL[6] Y
∇

∇Z←SQL∆Rollback Y
Z←SQL[7] Y
∇
