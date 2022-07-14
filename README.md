PDE Indexer

Author: Nicholas Facciola

The PDE indexer is a C++ command line tool that serves as an alternative to conventional Unix command line search tools such as “find” or “grep”. The PDE indexer uses a database search rather than a file tree walk to find files and is specific to the sc_aepde disks managed on the Santa Clara server. The advantage to this is that search times are reduced however the files are only updated to reflect what is stored in the SQLite database. To manage this, the SQLite database is update nightly from a cronjob running the shell script `updateDB.sh`. This allows for an accurate search for any files that were not modified within the last 24 hours of using the tool.
