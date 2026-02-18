""" initial settings."""
#!/usr/bin/env python3

import os
# r-tree load&query, data generator executables
sailbindir = os.path.expanduser("~/PROGRAMMING/libspatialindex/bin/")
# bdbbindir = os.path.expanduser("~/git/research/project1/Debug/")  # BDB B-tree executables ..

# SCRIPT_PATH=os.path.expanduser("~/git/mysail/test/rtree/")

datadir = os.path.expanduser ("~/PROGRAMMING/tmp/data/")

dbdir = os.path.expanduser ("~/PROGRAMMING/tmp/database/")

querydir = os.path.expanduser ("~/PROGRAMMING/tmp/query/" )

pltdir = os.path.expanduser ("~/PROGRAMMING/tmp/plt/")

resultsdir = os.path.expanduser ("~/PROGRAMMING/tmp/results/")

if not os.path.exists(datadir):
    os.makedirs(datadir)

if not os.path.exists(dbdir):
    os.makedirs(dbdir)

if not os.path.exists(querydir):
    os.makedirs(querydir)

if not os.path.exists(pltdir):
    os.makedirs(pltdir)
else:
    contents = os.listdir(pltdir)
    for item in contents:
        item_path = os.path.join(pltdir, item)
        if os.path.isfile(item_path):
            os.remove(item_path)

if not os.path.exists(resultsdir):
    os.makedirs(resultsdir)
else:
    contents = os.listdir(resultsdir)
    for item in contents:
        item_path = os.path.join(resultsdir, item)
        if os.path.isfile(item_path):
            os.remove(item_path)

aqar=1  # STR-tree use this default if not set to 1.4 3.3 .. for AdpSTR.
# external sorting parameters:  STR-treee uses)
pS=10000  # Page size (RPB). Used in ext-sort
bP=100    # buffer size (num of buffers). Used in ext-sort.   10000 * 100 * 40B-records = 40 MB
fillfactor=0.999   # used in only bulk loading, STR

# system params:
# onceden page_size idi
nodesize_pages=2  # page cinsinden node size. Node-size manasında. = number of 4K.    page_size = 1 ==> we have 4K-size pages..page_size = 2 ==> we have 8K-size pages ( note that pgsql uses 8K-sized pages)

#cache_size idi.
# cachesize_nodes=131072 # max. number of 4K-pages in mem.  131072 = 128K ==> 512 MB cache
cachesize_nodes=65536  # pages. max. number of 8K-pages in mem.  ==> 512 MB cache

# Common usage ise page size 4K.  If you change page size, you should increase capacity.
#export capacity=$(($page_size*5))
nodecapacity=180   # En az 4 olmalı!!! for 4K-size node use 90.  8K-size node use 180.
