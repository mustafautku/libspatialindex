#!/usr/bin/env python3
# if you want to need your own evironment activate it in the "current shell" than run this utility..

import sys
import time

import subprocess
import os
import re
import sailconfig as sc
import myplotting as myplt

# os.environ['LD_LIBRARY_PATH'] = '/home/mustafautku/eclipse-workspace/mysail-build/bin'
#
# from pygnuplot import gnuplot
# g = gnuplot.Gnuplot()

# generate generates 2 types of objetcs::
#     --- data and query generator
#        Data::
#           AQAR is not valid for data sets. Always provide dxdydist for data.
#       Query::
#             You may generate f|u|aqar extent-ed queries.
##          AQAR values are from [0.1, 0.3, 0.7, 1, 1.428, 3.33, 10 ]
#             For point query, set AQAR as 1. If not, no problem :<
# sample usage:  butun argumanlar  '...'  icinde olmali.
# generate(data, 100, u, 0.04 0.04 f    0          1          0        2)    -- [0,3]
# generate(data, 100, u, 0.04 0.04 f    0            2           0        2      7     )            --- [0,15]
#          g[1,>1] : skewedness 1 veya daha buyuk.. Artıkça daha sıkışık veri. Default 5,,, 10 oldukça sıkışık !!
# generate (data 1000 g10 0.04    0.04   f    0          1          0        2      3 )  --> 10 oldukça sıkışık !!
# generate( query 100  u   0.04    0.04  1.4 )  -- generate queries with specific AQAR.
# generate (query 100  u   0.04    0.04   u  )   -- generate queries with uniform-sized extent.

##  Mesela; $qs= 5 partition=1 0 3 ==> 4 tane query uretir.
def generate(file,obj_type,dss,dlocdist,dx,dy,dxdydist,*optional_args):
    if obj_type == "data":
        #  stdout=output_file, stderr=error_file  ... if you want another file
        with open(file, 'w') as output_file :
            subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-Generator'), obj_type, dss, dlocdist, \
                            dx, dy, dxdydist]+ list(optional_args), stdout=output_file, text=True)
    else:
        with open(file, 'a') as output_file :   # append cunku ilk satıra metadata ekledim..numOfBatches querybatchsize
            subprocess.run([os.path.join(sc.sailbindir, 'test-rtree-Generator'), obj_type, dss, dlocdist, \
                            dx, dy, dxdydist]+ list(optional_args), stdout=output_file, text=True)

#False,1,100,'g',0.03,0.03,'f',['2','3','5'],10,'g8',0.1,0.1,'u',['2','2','7'],True,2
def generate_objs(scalarmode,sel,\
                  initdatabatchsize,ddist,dx,dy,dxdydist,data_optional_args,\
                  querybatchsize,qdist,qx,qy,qxqydist,query_optional_args,\
                  mixquery,numOfBatches,appdatabatchsize):

    if scalarmode==True and (ddist != 'u' or qdist != 'u') :
        print('Scalar data & query !!only!! can be generated uniformly..')
        sys.exit()
    # DATA:::
    if initdatabatchsize >0:
        datafile=os.path.join(sc.datadir, 'datafile')
        if scalarmode==True:
            generate(datafile,'data',f'{initdatabatchsize}',f'{ddist}','-1','-1','-1','0')  # dx=-1  no selectivity, no interval
        else:
            if len(data_optional_args)==0:
                print("Generating (initial) data in unit area")
                generate(datafile,'data',f'{initdatabatchsize}',f'{ddist}',f'{dx}',f'{dy}',f'{dxdydist}','0')
            else:
                print("Generating (initial) data in grid(s):"+ f'{data_optional_args[0]}')
                generate(datafile,'data',f'{initdatabatchsize}',f'{ddist}',f'{dx}',f'{dy}',f'{dxdydist}','0',*(data_optional_args[0]))  # f'{initdatabatchsize}'  is same str(initdatabatchsize)
            if dx==0:
                myplt.plot1(datafile,'data','point')  # NOTE: important to write correct spatialtype: point vs region
            else:
                myplt.plot1(datafile,'data','region')

    time.sleep(1)  # important to generate with different seeds..

    # QUERY:::
    if querybatchsize >0:
        queryfile=os.path.join(sc.querydir, 'queryfile')
        os.remove(queryfile)  # mevcut ise silelim. Cunku su.generate icinde append ediliyor. mixed workload icin bu gerekiyordu..
        if mixquery== False:
            print("Generating regular query file..")
            if scalarmode==True:
                generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{sel}','-1','-1', '0')
            else:
                if len(query_optional_args) == 0:
                    print("Generating (initial) query in unit area")
                    generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{qx}',f'{qy}',f'{qxqydist}','0')
                else:
                    print("Generating (initial) query in grid(s):"+ f'{query_optional_args[0]}')
                    generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{qx}',f'{qy}',f'{qxqydist}','0',*(query_optional_args[0])) # '0' manasız..
                myplt.plot1(queryfile,'query','region')
                if initdatabatchsize >0:
                    if dx==0:
                        myplt.plot2(datafile,'data','point',queryfile,'query','region')
                    else:
                        myplt.plot2(datafile,'data','region',queryfile,'query','region')
        else:
            print("Generating mixfile(query, insert, query, insert..)")
            with open(queryfile, "w") as dosya:
                dosya.write("# "+f'{numOfBatches}' + " "+ f'{querybatchsize}'+"\n")
    
            if scalarmode==True:
                generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{sel}','-1','-1', '0')
            else:
                if len(query_optional_args) == 0:
                    print("Generating (initial) query in unit area")
                    generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{qx}',f'{qy}',f'{qxqydist}','0') # '0' manasız..
                else:
                    print("Generating (initial) query in grid(s):"+ f'{query_optional_args[0]}')
                    generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{qx}',f'{qy}',f'{qxqydist}','0',*(query_optional_args[0])) # '0' manasız..
    
            mixfile=os.path.join(sc.querydir, 'query_x')
            with open(mixfile, 'w') as outfile:
                with open(queryfile) as infile:
                    for line in infile:
                        outfile.write(line)
                time.sleep(1)
                for i in range(1,numOfBatches):  # ilk batch (data ve query) hallettik zaten
                    datafile=os.path.join(sc.datadir, 'appenddatafile')
                    if scalarmode==True:
                        generate(datafile,'data',f'{appdatabatchsize}',f'{ddist}','-1','-1','-1',f'{initdatabatchsize+(i-1)*appdatabatchsize}')  # dx=-1  no selectivity, no interval
                    else:
                        if len(data_optional_args) == 0:
                            print("Generating data in unit area")
                            generate(datafile,'data',f'{appdatabatchsize}',f'{ddist}',f'{dx}',f'{dy}',f'{dxdydist}',f'{initdatabatchsize+(i-1)*appdatabatchsize}');
                        else:
                            print("Generating data in grid(s):"+ f'{data_optional_args[i]}')
                            generate(datafile,'data',f'{appdatabatchsize}',f'{ddist}',f'{dx}',f'{dy}',f'{dxdydist}',f'{initdatabatchsize+(i-1)*appdatabatchsize}',*(data_optional_args[i]));
    
                    with open(datafile) as infile:
                        for line in infile:
                            outfile.write(line)
                    time.sleep(1)  # suspend 1 sec for a new seed
                    os.remove(queryfile)
                    if scalarmode==True:
                        generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{sel}','-1','-1', '0')
                    else:
                        if len(query_optional_args) ==0:
                            print("Generating query in unit area")
                            generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{qx}',f'{qy}',f'{qxqydist}','0')
                        else:
                            print("Generating query in grid(s):"+ f'{query_optional_args[i]}')
                            generate(queryfile,'query',f'{querybatchsize}',f'{qdist}',f'{qx}',f'{qy}',f'{qxqydist}','0',*(query_optional_args[i]))
                    with open(queryfile) as infile:
                        for line in infile:
                            outfile.write(line)
                    time.sleep(1)  # suspend 1 sec for a new seed
                    os.remove(datafile)  # bu silinen ilk yuklenen datafile degil..append edilenler!!!
    
            os.remove(queryfile)
            os.rename(mixfile,queryfile)
            # myplt.plot1(queryfile,'mixquery','region' ) # gerek kalmadı.


# BDB result dosyası ile R-tree result farklı. O yüzden validate_results oncesinde re-format gerekiyor.
# BDB result ciktilarının 2. kolonunu alıp baska bir dosyaya kaydediyor.   
def format_results_file(file):
    fileformatted=os.path.join(sc.resultsdir,f'{file}-'+'formatted')
    with open(file, 'r') as infile, open(fileformatted, 'w') as outfile:
        for line in infile:
            parts = line.split()
            if len(parts) >= 2:  # Satırda en az 2 sütun varsa
                col2 = parts[1]  # 2. sütunu al
                outfile.write(col2 + '\n')  # 2. sütunu yeni dosyaya yaz
            else:
                outfile.write('\n')  # Boş satırı yeni dosyaya yaz
    return fileformatted   

# Sorgu is yukundeki her sorgu sonucunun dogrulugunu kontrol ediyor.
# **** file2 her zaman brute force, ve tam dogru oldugunu varsayabilriiz****..
def validate_results(file1, file2):
    with open(file1, 'r') as file_a, open(file2, 'r') as file_b:
        data_a = re.split(r"\n\s*\n", file_a.read().strip())
        data_b = re.split(r"\n\s*\n", file_b.read().strip())

    # if len(data_a) != len(data_b):
    #     return "Dosyalar farklı sayıda blok içeriyor."
    falsehits=0
    misses=0
    totalcorrect=0
    for i, (block_a, block_b) in enumerate(zip(data_a, data_b), start=1):
        lines_a = block_a.strip().split("\n")
        lines_b = block_b.strip().split("\n")

        if len(lines_a) != len(lines_b):
            if len(lines_a) > len(lines_b):
                falsehits = len(lines_a) - len(lines_b)
            else:
                misses = len(lines_b) - len(lines_a)
            print (f"Blok {i} içindeki verilerin sayısı farklı." )
        
        lines_a_int = []
        for string in lines_a:
            integer = int(string)
            lines_a_int.append(integer)
        lines_b_int = []
        for string in lines_b:
            integer = int(string)
            lines_b_int.append(integer) 
            
        lines_a_int.sort()
        lines_b_int.sort()
        # print(lines_a_int)
        # print(lines_b_int)
        for item_a in lines_a_int:
            for item_b in lines_b_int:
                if item_a == item_b:
                    totalcorrect += 1
                    break
            else:
                falsehits += 1
                print( f"Farklı olan öğe blok numarası: {i}" )
                # sys.exit()
    # Her iki dosyada da tüm bloklar ve toplam sonuç sayıları aynı.
    return (falsehits,misses,totalcorrect)


def stripe_stats(stats_file_path):

    stats = {
        "db_size": None,
        "tot_loadtime": None,
        "tree_height": None,
        "#_of_nodes": None,
        "L0_pages": None,
        "L1_pages": None,
        "L2_pages": None,
        "L3_pages": None,
        "tot_splits": None,
        "inc_splits": None,
        "wl_size": None,
        "tot_searchtime": None,
        "tot_recs_found": None,
        "cum_endtime": None,
    }
    
    # Read the stats file
    inc_splits_counter=0
    with open(stats_file_path, 'r') as file:
        for line in file:
            if "Splitting the Current max" in line:
                inc_splits_counter += 1
            # Extract specific values by splitting on ':' and stripping whitespace
            if "DATABASE-size :" in line:
                stats["db_size"] = int(line.split(':')[1].strip())
            elif "TOTAL Elapsed time during LOAD(msecs):" in line:
                stats["tot_loadtime"] = float(line.split(':')[1].strip())
            elif "Tree height:" in line:
                stats["tree_height"] = int(line.split(':')[1].strip())
            elif "Number of nodes:" in line:
                stats["#_of_nodes"] = int(line.split(':')[1].strip())
            elif "Level 0 pages:" in line:
                stats["L0_pages"] = int(line.split(':')[1].strip())
            elif "Level 1 pages:" in line:
                stats["L1_pages"] = int(line.split(':')[1].strip())
            elif "Level 2 pages:" in line:
                stats["L2_pages"] = int(line.split(':')[1].strip())
            elif "Level 3 pages:" in line:
                stats["L3_pages"] = int(line.split(':')[1].strip())
            elif "Splits:" in line:
                stats["tot_splits"] = int(line.split(':')[1].strip())
            elif "WL-size :" in line:
                stats["wl_size"] = int(line.split(':')[1].strip())
            elif "TOTAL Elapsed time during Search(msecs):" in line:
                stats["tot_searchtime"] = float(line.split(':')[1].strip())
            elif "TOTAL Records found:" in line:
                stats["tot_recs_found"] = int(line.split(':')[1].strip())
    
    stats["inc_splits"] =inc_splits_counter
    # Calculate Cumulative Ending Time
    if stats["tot_loadtime"] is not None and stats["tot_searchtime"] is not None:
        stats["cum_endtime"] = stats["tot_loadtime"] + stats["tot_searchtime"]
    
    # Create a DataFrame from the stats dictionary
    # stats_sum = pd.DataFrame([stats])
    return stats
    # Display the DataFrame (you can print or manipulate it as needed)
    # print(stats_sum)  # Uncomment this line if you want to see the DataFrame output
    # pd.set_option('display.max_columns', None)  # Show all columns
    # pd.set_option('display.expand_frame_repr', False)  # Prevent line breaks in the output

    # print(stats_sum)



if __name__ == "__main__": # it execute here if it were run as a standalone program..
    
    # generate  fonksiyonunu haricten kullanmana hic gerek yok. Her zaman generate_objs kullan ki standart olsun..
    # datafile=os.path.join(sc.datadir, 'datafile')
    # generate(datafile,'data','100','u','0','0','f')
    # generate(datafile,'data','100','u','0','0','f','0','2','3','5')

    # regular spatial query file::
    # False,1,           100,'g',0.03,0.03,'f',['2','3','5'],   10,'g8',0.1,0.1,'u',['2','2','7'],   True,  2,                10   
    # scalarmode,sel,     initialdatabatch                       initialquerybatch                   mixed? numberofBatches   appendBatchsize
    #                                                                                                "False" ise son ikisinin manası yok!!    
    # generate_objs(False,-1,1000,'g',0.05,0.05,'f',[('2','3','5')],10,'u',0.1,0.1,'u',[('1','0')],False,-1,-1)
    # generate_objs(False,-1,1000,'g',0.05,0.05,'f',[],10,'u',0.1,0.1,'u',[],False,-1,-1)

    # to debug spatial mixed queryfile, run the plotting function: plot1(queryfile,'mixquery','region')... See myplotting module.
    # to debug mixed queryfile, use region data. Aksi takdirde aynı dosyada hem region hem data point varsa sadece region'lari cizecek..
    # generate_objs(False,-1,100,'g',0.05,0.05,'f',[('2','3','5'),('2','7')],10,'u',0.1,0.1,'u',[('1','0'),('1','3')],True,2,10)  # ilk databatch ciziyor. mixfile'da 2 querybatch var ('1','0'),('1','3') ve son bir databaseh var ('2','7')
    # generate_objs(False,-1,80,'g',0.01,0.01,'u',[('2','0','15'),('2','7')],8,'u',0.07,0.07,'1.4',[('2','0','12'),('2','7')],True,2,3)  # 2 grid deseni tanımlaman gerek..
    # generate_objs(False,-1,80,'g',0.001,0.001,'f',[('2','0','15'),('2','7')],8,'u',0.07,0.07,'1.4',[('2','0','12'),('2','7')],True,2,3)

    # unit area test::
    # generate_objs(False,-1,80,'g',0.001,0.001,'f',[(),()],8,'u',0.07,0.07,'1.4',[(),()],True,2,2)
    # generate_objs(False,-1,10,'g',0.001,0.001,'f',[],5,'u',0.07,0.07,'1.4',[],True,2) # Dogru.. [] ya bos olmalı ya da numOfBatch ile uyumlu olmalı. Bu unit area da 2 tane batch uretiyor.
    # generate_objs(False,-1,80,'g',0.001,0.001,'f',[('2','0','15')],8,'u',0.07,0.07,'1.4',[('2','0','15')],True,2)  # bu hatalı olacak..

    # only datafile or only queryfile. [] icine sadece en fazla 1 tane desen yazabilrisin..digerlerini ignore edecek zaten..
    # generate_objs(False,-1,   8,'g',0.01,0.01,'f',[],  0,'',0,0,'',[],  False,-1,-1)
    # generate_objs(False,-1,   0,'',0,0,'',[],          10,'u',0.1,0.1,'u',[],  False,-1,-1)
    
    # scalar data::
    # generate_objs(True,1,1000,'u',-1,-1,'-1',[],10,'u',-1,-1,'-1',[],False,-1)
    # generate_objs(True,1,100,'u',-1,-1,'-1',[],10,'u',-1,-1,'-1',[],True,1)
    # generate_objs(True,1,100,'u',-1,-1,'-1',[],10,'u',-1,-1,'-1',[],True,2)
    # plot(os.path.join(sc.datadir,'datafile'),'data','point')

    #hatalı olabilecek bazı uc durumlar::
    # generate_objs(False,-1,1000,'u',0.3,0.3,'f',[('2','3','5')],10,'u',0.1,0.1,'u',[('1','0')],False,-1)  # Bu uretemeyecek. cunku dept=2 ==> hucre size ufak. dx=0.3, dy=0.3 cok buyuk!!!

    # results1=os.path.join(sc.resultsdir,'spat-p-bdbresults')
    # results1f=format_results_file(results1)
    # results2=os.path.join(sc.resultsdir,'spat-bfresults')
    # results2f=format_results_file(results2)
    # (falsehits,misses,totalcorrect)=validate_results(results1f,results2f)
    # os.remove(results1f)
    # os.remove(results2f)
    # print(f"The error in Part-b+tree is falsehits:{falsehits}, misses:{misses} in total:{totalcorrect}")
    
    stripe_stats('stats-IncR')

# Bu asagidakileri kullanan kalmadi. SAdece bilgi amacli duruyorlar..
'''
def calculate_the_error(file1, file2, file1col, file2col): # file2 should be the correct results from brute force.
    with open(file1, 'r') as f1, open(file2, 'r') as f2:
        lines1 = f1.readlines()
        lines2 = f2.readlines()

    column1 = set(line.split()[int(file1col)] for line in lines1)
    column2 = set(line.split()[int(file2col)] for line in lines2)

    falsehits = sum(1 for item in column1 if item not in column2)
    falsemisses = sum(1 for item in column2 if item not in column1)
    # rate = (count / total) * 100

    return (falsehits, falsemisses, len(column2))

def count_duplicates(file_path):
    items = []

    with open(file_path, 'r') as file:
        for line in file:
            columns = line.strip().split()
            if len(columns) >= 2:
                item = int(columns[1])  # İkinci sütunu bir tamsayıya dönüştür
                items.append(item)

    num_duplicates = len(items) - len(set(items))
    return num_duplicates


from itertools import groupby
def sort_file(file_path, output_path):
    data = []

    with open(file_path, 'r') as file:
        for line in file:
            columns = line.strip().split()
            if len(columns) >= 2:
                columns[1] = int(columns[1])  # Convert second column to integer
                data.append(columns)

    sorted_data = sorted(data, key=lambda x: x[1])
    grouped_data = [list(group) for _, group in groupby(sorted_data, key=lambda x: x[1])]

    with open(output_path, 'w') as output_file:
        for group in grouped_data:
            for row in group:
                output_file.write('\t'.join(map(str, row)) + '\n')  # Convert integers back to strings

'''
