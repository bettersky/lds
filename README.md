# Prototype LSM-tree Direct Storage based on LevelDB.

How to compile:
1. Put the lds* files under the db directory.
2. Put the env_lds.cc under the util directory.
3. Compile LevelDB.

How to use:

When open a database, the user should pass a device name such as /dev/sdh or /dev/sdh1 to the LevelDB, instead of passing a directory. A pre-allocated file is also legal to be passed but not fully supported. 
