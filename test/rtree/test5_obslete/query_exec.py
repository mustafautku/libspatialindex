#!/usr/bin/env python3

'''
Bu test'de R*-tree de execution dogrulugunu sagladim ve exec-stats bilgilerini topladım.
Dogruluk icin rtree-ehaustive kullanılıyor. Bu program veri ve sorgulari aynı dosyada istedigi icin iki dosyayı birarya getirdik.. 
'''
import sys
import os
import subprocess

import sailconfig as sc
import sailutility as su
import myplotting as myplt

# os.environ['LD_LIBRARY_PATH'] = '/home/mustafautku/eclipse-workspace/mysail-build/bin'


initialdbs=10000  # initial data batch size
querybs=5 # querybatchsize. always same

su.generate_objs(False, -1,\
                 initialdbs, 'u','0.3', '0.3', 'f', [], \
                 querybs, 'g', '0.05', '0.05', 'f', [], \
                 False, -1,-1)  #  no mixquery


# ATTENTION: Use small test files..Large datafile may be problem..because try to copy all content to MM.
# I need this mixfile for ExhaustiveSearch. THIS IS NOT interleaved-mix file. All inserts then queries..
datafile=os.path.join(sc.datadir, 'datafile')
queryfile=os.path.join(sc.querydir, 'queryfile')
dataqueryfile=os.path.join(sc.datadir, 'dataqueryfile')
with open(datafile, 'r') as file1, open(queryfile, 'r') as file2, open(dataqueryfile, 'w') as output_file:
        content1 = file1.read()
        content2 = file2.read()
        combined = content1 + content2 
        output_file.write(combined)

print("r*-tree loading...")
leafplt=os.path.join(sc.pltdir, 'plt-rleaf')
treestats=os.path.join(sc.pltdir, 'stats-rtree-load')
with open(leafplt, 'w') as plt_file, open(treestats,'w') as stats_file:  
                            ## datafile  treename    pageSize  cachesize_nodes nodecapacity   isPlotting. isIncremental
    subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-RTreeLoad'), datafile, 'rtree', f'{sc.nodesize_pages}', \
                    f'{sc.cachesize_nodes}', f'{sc.nodecapacity}','1','0'], cwd=sc.dbdir, stdout=plt_file, stderr=stats_file, text=True)
myplt.plot1(leafplt,'MBR-rtree','region')

# selfjoin , intersection ,   pointquery   ,   10NN
# ATTENTION !!!  selfjoin her satırda 2 sonuc deger veriyor. Yani sorgu region ile aynı zamanda birbiriyle de kesisen dataregion ikililerini veriyor.
# Bu yuzden mevcuıt su.validate_results(..) bu validate i yapamaz..Daha cok zaman ayırmadım ama dogru calisiyor..Genel olarak scan ederek kontrol edilebilir..
querytype="containedIn"

print("   ***r-tree query processing: ", querytype)
results1=os.path.join(sc.resultsdir,'spat-rtreeresults')
treestats=os.path.join(sc.pltdir, 'stats-rtree-exec')
with open(results1, 'w') as res_file,open(treestats,'w') as stats_file:
                                #query_file  tree_file  cachesize_nodes    query_type [intersection | 10NN | selfjoin | containedIn].
    subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-RTreeQuery'), queryfile, 'rtree', f'{sc.cachesize_nodes}',\
                    querytype], cwd=sc.dbdir, stdout=res_file,stderr=stats_file, text=True)


print("   *** exhaustive query processing: ", querytype)
results2=os.path.join(sc.resultsdir,'spat-exhaustiveresults')
treestats=os.path.join(sc.pltdir, 'stats-exhaustive-exec')
with open(results2, 'w') as res_file,open(treestats,'w') as stats_file:
                                #query_file  tree_file  cachesize_nodes    query_type [intersection | 10NN | selfjoin | containedIn].
    subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-Exhaustive'), dataqueryfile, querytype], cwd=sc.dbdir, stdout=res_file, stderr=stats_file,text=True)

if querytype == "selfjoin":
    print("scan result yourself..")
    sys.exit()
    
(falsehits,misses,totalcorrect)=su.validate_results(results1, results2)
rate=(falsehits+misses)/totalcorrect
print(f"The errors of {querytype} query on r-tree: falsehits:{falsehits}, misses:{misses} in total:{totalcorrect}. Error rate: {rate}\n")


