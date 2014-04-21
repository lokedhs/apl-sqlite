#!apl -f

⍝  This file contains helper definititions to make the SQL API more easy
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

∇Z←SQL∆Tables Y
Z←SQL[8] Y
∇

∇sql∆∆load_library;result
→(0≠⎕NC 'SQL')/skip
result ← 'lib_sql.so' ⎕FX 'SQL'
→('SQL'≡result)/skip
⎕ ← 'Error loading native library'
skip:
∇

sql∆∆load_library
)erase sql∆∆load_library

⎕←'SQL lib loaded'
