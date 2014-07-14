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

∇Z←L SQL∆Connect R
⍝ Connect to database of type L using connection arguments R.
⍝
⍝ L must be a string indicating the database type. Current supported
⍝ values are 'postgresql' and 'sqlite'.
⍝
⍝ Y is the connection parameters which depends on the type of
⍝ database.
⍝
⍝ This function returns a database handle that should be used when
⍝ using other SQL functions. This value should be seen as an opaque
⍝ handle. It is, however, guaranteed that the handle is a scalar
⍝ value.
  Z←L SQL[1] R
∇

∇Z←SQL∆Disconnect R
⍝ Disconnect from database R.
⍝
⍝ R is the database handle that should be disconnected. After this
⍝ function has been called, no further operations are to be performed
⍝ on this handle. Future calls to SQL∆Connect may reuse previously
⍝ disconnected handles.
  Z←SQL[2] R
∇

∇Z←L SQL∆Select[db] R
⍝ Execute a select statement and return the result table.
⍝
⍝ The axis parameter indicates the database handle.
⍝
⍝ L is a select statement to be executed. Positional parameters can be
⍝ supplied by specifying a question mark "?" in the statemement.
⍝
⍝ R is an array containing the values for the positional parameters.
⍝ If the array is of rank 2, the statement will be executed multiple
⍝ times with each row being the values for each call.
⍝
⍝ The return value is a rank-2 array representing the result of the
⍝ select statement. Null values are returned as ⍬ and empty strings
⍝ are returned as ''.
  Z←L SQL[3,db] R
∇

∇Z←L SQL∆Exec[db] R
⍝ Execute an SQL statement that does not return a result.
⍝
⍝ This function is identical to SQL∆Select with the exception that it
⍝ is used on statements which do not return a result table.
  Z←L SQL[4,db] R
∇

∇Z←SQL∆Begin R
⍝ Begin a transaction.
  Z←SQL[5] R
∇

∇Z←SQL∆Commit R
⍝ Commit a transaction.
  Z←SQL[6] R
∇

∇Z←SQL∆Rollback R
⍝ Rolls back the current transaction.
  Z←SQL[7] R
∇

∇Z←SQL∆Tables R
⍝ Return an array containing the name of all tables.
  Z←SQL[8] R
∇

∇Z←X (F SQL∆WithTransaction FINDDB) Y;result
  SQL∆Begin FINDDB

  →(0≠⎕NC 'X')/dyadic
  result ← '→rollback' ⎕EA 'F Y'
  →commit

dyadic:
  result ← '→rollback' ⎕EA 'X F Y'

commit:
  SQL∆Commit FINDDB
  Z ← result
  →end

rollback:
  SQL∆Rollback FINDDB
  ⎕ES "Transaction rolled back"
end:
∇

∇sql⍙load_library;result
  →(0≠⎕NC 'SQL')/skip
  result ← 'lib_sql.so' ⎕FX 'SQL'
  →('SQL'≡result)/skip
  ⎕ES 'Error loading native library'
skip:
∇

sql⍙load_library
)erase sql⍙load_library

⎕←'SQL lib loaded'
