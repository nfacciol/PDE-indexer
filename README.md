# PDE Indexer

###### Author: Nicholas Facciola

The PDE indexer is a C++ command line tool that serves as an alternative to conventional Unix command line search tools such as “find” or “grep”. The PDE indexer uses a database search rather than a file tree walk to find files and is specific to the sc_aepde disks managed on the Santa Clara server. The advantage to this is that search times are reduced however the files are only updated to reflect what is stored in the SQLite database. To manage this, the SQLite database is update nightly from a cronjob running the shell script `updateDB.sh`. This allows for an accurate search for any files that were not modified within the last 24 hours of using the tool.

## Indexer basic commands
There are three basic arguments 
1. --file 
2. --dir 
3. -r [regex]

To run the tool use the command `./indexer` if no argument type is given, then the next argument will be presumed to be a file. and the code will look for all files in the database containing the command line input. For example, running `./indexer .java` will find all files with the .java extension across all the PDE disks
![image](https://user-images.githubusercontent.com/106199257/179088914-836bb890-8bc2-436e-9711-b39854a71615.png)

Running with the `--dir` command will essentially do the same thing however your query will be limited to directories instead of files.

Running with `-r` will pattern match files in the databse based on a regular expression. This method is more precise but takes longer to run. I reccomend running this when you know either the disk, or the user that the file in question is associated with (see [flags](https://github.com/nfacciol/PDE-indexer/edit/main/README.md#indexer-flags)). As an example of when to use regex, suppose that you wanted to find all files in disk 2 with the pattern `_00_VMB022TS_` within the jsl tester file directory. You would query it as shown below:
`./indexer -r '/nfs/site/disks/sc_aepdeD02/jsl/tvpv/trc/d01/s99/([a-zA-Z]+([0-9]+[a-zA-Z]+)+)_00_VMB022TS_([a-zA-Z]+([0-9]+[a-zA-Z]+)+)_/[a-zA-Z]+_([a-zA-Z]+(\.[a-zA-Z]+)+)' -d 2` (NOTE: do not forget the single quotes around the regex as doing so will produce incorrect results). If this command is run correctly it will produce the follwing result

![image](https://user-images.githubusercontent.com/106199257/179095642-9368f5fb-42de-4736-9659-8c758f209e90.png)

For more help on generating regular expressions visit [this site](https://regex-generator.olafneumann.org/). Here you can paste a sample of text and select the important patterns and the site will automatically generate a regular expression for you that you can then paste into the command line.

## Indexer flags
-f	FORCE: If force is set then all query results will display without any pause up this flag is true by default

-x	MAX: If the max flag is given, specify the amount of results to display to the screen before a pause is shown (this can also be accomplished by piping the command to `more`)

-m	MATCH: If the match flag is given, results will only display names that exactly match the name of the given token

-d	DISK: If the disk flag is given, results will be narrowed down to onedisk instead of all disks. (This can be useful to prevent hangs on large queries) Please enter the disk number and do not manually enter a leading zero i.e `-d 2` will search sc_aepdeD02.db, `-d 02` will fail

-u	USER: If the user flag is given, results will only be displayed for a username entered after the flag. (Note: if you want to search for files not owned by a particular user enter 'root_scaepdeD(DISKNUM)' where DISKNUM is the number of the disk you are searching (i.e `-u root_sc_aepdeD02`)

## Potential improves
1. Possibility of spilitting queries into threads based on the disk 
  - This will parallelize the search and make queries faster while the main thread can handle printing to the console
2. Database schema optimization
  - Consider redoing the database schema in way where search results can be optimized
3. Adding more features to the indexer
  - Possibility of zipping large files
  - Viewing disk statistics quickly and simply
  - Viewing stats by user
 
Please report all feedback and bugs to nicholas.facciola@intel.com I appreciate any feedback positive or negative and feel free to consider modifying the code if youh have time. Thanks!

###### Version History
- 20220714A - Basic functions released 
