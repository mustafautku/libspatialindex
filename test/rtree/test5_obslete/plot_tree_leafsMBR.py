#!/usr/bin/env python3

'''
Bu script mevcut olan bir R-tree nin ( R*, STR veya incremental..) yaprak dugumlerinin MBR'larini plot ediyor..o kadar..
Genelde buradaki yapıyı diger testlerde mevcut eski bir agaci çizdirmek istediğinde kullanabilrisin...
'''
import os
import subprocess

import sailconfig as sc
import myplotting as myplt

# os.environ['LD_LIBRARY_PATH'] = '/home/mustafautku/eclipse-workspace/mysail-build/bin'

treename="rtree"  # this tree(idx) name should exist in sc.dbdir directory

print(" traverse r-tree leaf nodes to generate MBRs ...")
leafplt=os.path.join(sc.pltdir, 'plt-rleaf-'+f'{treename}')
with open(leafplt, 'w') as plt_file:  
                            ## datafile  treename    pageSize  cachesize_nodes nodecapacity   isPlotting. isIncremental  idxIdentifier
    subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-RTreeLoad'), "", treename, f'{sc.nodesize_pages}', \
                    f'{sc.cachesize_nodes}', f'{sc.nodecapacity}','1','0', '1'], cwd=sc.dbdir, stdout=plt_file, text=True)
myplt.plot1(leafplt,'MBR-rtree-'+f'{treename}','region')

