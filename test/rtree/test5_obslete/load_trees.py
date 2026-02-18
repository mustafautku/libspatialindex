#!/usr/bin/env python3
'''
Bu script veri seti uretiyor ve R-tree yükluyor ( R*, STR veya incremental..) ve
yaprak dugumlerinin MBR'larini icerisindeki data ile beraber plot ediyor..o kadar..
'''

import os
import subprocess
import sailconfig as sc
import sailutility as su

import myplotting as myplt

# os.environ['LD_LIBRARY_PATH'] = '/home/mustafautku/eclipse-workspace/mysail-build/bin'

datafile=os.path.join(sc.datadir, 'datafile')

# Bilgi amacli o yüzden dagistirme burayı::
#settigns below leads 2-heigth tree. 3 nodes * (2 * 4K-page) + 1 head (2* 4K-page) =  32 * 4K-page  = 32 KB 
# sc.nodesize_pages=2  # yani node-size. 2* 4K => index 8KB lik node'lardan olusuyor.  
# sc.nodecapacity=180
# initialdbs=182  # initial data batch size
# querybs=0 # querybatchsize.

# used in ıeee access fig 1
# sc.nodesize_pages=1
# sc.nodecapacity=8
# initialdbs=100  # initial data batch size
# querybs=0 # querybatchsize.

sc.nodesize_pages=2
sc.nodecapacity=180
initialdbs=182  # initial data batch size
querybs=0 # querybatchsize.

su.generate_objs(False, -1, \
                 initialdbs, 'g6', 0.1, 0.1, 'u', [('1','0','3')], \
                 querybs,   '',    0,  0,    '',  [],\
                 False,-1,-1)



print("r*-tree loading...")
leafplt=os.path.join(sc.pltdir, 'plt-rleaf')
treestats=os.path.join(sc.pltdir, 'stats-rtree-load')
with open(leafplt, 'w') as plt_file, open(treestats,'w') as stats_file:
                            ## datafile  treename    pageSize  cachesize_nodes nodecapacity   isPlotting. isIncremental
    subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-RTreeLoad'), datafile, 'rtree', f'{sc.nodesize_pages}', \
                    f'{sc.cachesize_nodes}', f'{sc.nodecapacity}','1','0'], cwd=sc.dbdir, stdout=plt_file, stderr=stats_file, text=True)
# myplt.plot1(leafplt,'MBR-rtree','region')
myplt.plot2(leafplt,'Rstar-tree-MBR','region',datafile,'data-region','region')

print("STR-tree loading...")
leafplt=os.path.join(sc.pltdir, 'plt-strleaf')
treestats=os.path.join(sc.pltdir, 'stats-strtree-load')
with open(leafplt, 'w') as plt_file, open(treestats,'w') as stats_file:
                            ## datafile treename nodesize_pages cachesize_nodes nodecapacity      fillfactor    isPlotting    $aqar $pS $bP
    subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-RTreeBulkLoad'), datafile, 'strtree', f'{sc.nodesize_pages}', \
                    f'{sc.cachesize_nodes}', f'{sc.nodecapacity}',     f'{sc.fillfactor}','1', f'{sc.aqar}',f'{sc.pS}',f'{sc.bP}' ], cwd=sc.dbdir, stdout=plt_file, stderr=stats_file, text=True)
# myplt.plot1(leafplt,'MBR-strtree','region')
myplt.plot2(leafplt,'STR-tree-MBR','region',datafile,'data-region','region')


rstar_sizeInByte=os.path.getsize(os.path.join(sc.dbdir,'rtree.dat'))
str_sizeInByte=os.path.getsize(os.path.join(sc.dbdir,'strtree.dat'))
print("R*-tree db_size(# of pages in tree):",rstar_sizeInByte/(sc.nodesize_pages*4*1024) -1 )  # -1 is to exclude head page.
print("STR-tree db_size(# of pages in tree):",str_sizeInByte/(sc.nodesize_pages*4*1024)  -1 ) # -1 is to exclude head page.
