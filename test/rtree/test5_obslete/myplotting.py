#!/usr/bin/env python3
import os
import datetime
import pandas as pd
import numpy as np
from pygnuplot import gnuplot

import sailconfig as sc

'''
STATISTICAL DATA ::::used for QRT, disk IO evaulations...

There are simple line plots, bar graphs and stacked graphs...

'''

# Processing plt/stats-spat  file. Incremental_test_4-stats kullanıyor.. qt, q_put u icermedigi icin onu eklemek gerekti. (Onceki testlerde qt: q_in+q_last+q_del+q_put  idi..)
# Regular veya Mixquery. # Mixfile queries, data, queries,...queries formatında olmalı. En son queries olmalı!!!
# Incremental_test_4-stats bunu kullanıyor.
# qt == 0 olanları çıkarıyorum. --> loaddf
# geride kalanlar QRTs ==> df
# Her batch'daki total query RT i buluyorum --> qTTdf
# loaddf ve qTTdf format ları aynı. İkisini de yöntemler-yan-yana bar graf olarak ciziyor..
# df sadece sorgu gecikmeleri, eger split varsa (q_put) o da qt'ye ekleniyor..Sadece split yapıyorsa q_put degeri olmalı--> buna dikkat..
# multiplot ile çiziyorum..farklı sayıda batch'ler, farklı sayıda yöntemlerin hepsini çizebiliyor..
# Batch-based "Load time" and "total-repsonse-time (TRT)" and Indıvidiual Query RTs(QRT)
def multiplot_Load_TRT_QRTs(file,debug):
    print('''\nPLOTTING...generating Batch-based, "Load time" & "total-repsonse-time (TRT)" and Individiual Query RTs(QRT) :::''')
    df = pd.read_csv(file, sep=' ', header=None, names = ['name', 'qt','q_is','q_ls','q_del', 'q_put','res1', 'res2','lt'])  # names = ['name', 'a','b','c','d']
    df['qt'] = df['qt'] + df['q_put'];

    algset=set(df['name'])
    dseq={} # dictionary
    for alg in algset:
        algrows=df[df['name']==alg]  # subdf for each algorithm
        dseq.update({alg:algrows})
    if debug:
        print(dseq)
    # find numOfQueries in each Batch
    samplesubdf=list(dseq.values())[0]  # hangisi oldugu önemli değil..Cunku qt 0 ==> lt>0 mutlaka
    if debug:
        print(samplesubdf)
    samplesubdf['qt']
    qt0idxes = samplesubdf[samplesubdf['qt'] == 0].index
    # Check if there are at least two occurrences of 'qt' == 0
    # if len(qt0idxes) > 1:
    #     occurance1= qt0idxes[0]
    #     occurance2 = qt0idxes[1]
    # else:
    QSInaBatch = qt0idxes[1]-qt0idxes[0]-1 if len(qt0idxes) > 1 else len(samplesubdf)-1  # -1 is for loading line
        # indeks2_value = len(samplesubdf)  # Total number of rows if second index does not
    # indeks1 = samplesubdf[samplesubdf['qt'] == 0].index[0]
    # indeks2 = samplesubdf[samplesubdf['qt'] == 0].index[1]
    # QSInaBatch = indeks2 - indeks1 - 1
    if debug:
        print(QSInaBatch)
    if debug:
        print ("Generating QRTs, Load times and Total RTs in Each Batch..")
    loaddf = pd.DataFrame()
    qRTdf = pd.DataFrame()
    qTTdf = pd.DataFrame()
    for key, subdf in dseq.items():
        loadsubdf=subdf[subdf['qt']==0]
        subdf=subdf.drop(loadsubdf.index)

        loadsubdf=loadsubdf.reset_index(drop=True)
        subdf=subdf.reset_index(drop=True)  #reset index starting from 0. All algs. must start from same index.
        # print(loadsubdf)
        # print(subdf)
        qRTdf[key]=subdf['qt'] # mevcuda yeni bir sutun ekleme.
        loaddf[key]=loadsubdf['lt']
        # print(key, '\n',subdf['qt'].groupby(subdf.index // 10).sum())
        qTTdf[key]=subdf['qt'].groupby(subdf.index // QSInaBatch).sum()

    minYqt = np.min(qRTdf.values[np.nonzero(qRTdf.values)]) * .8
    maxYqt = qRTdf.values.max() * 1.05
    minYlt = np.min(loaddf.values[np.nonzero(loaddf.values)]) * .8
    maxYlt = loaddf.values.max()* 1.05
    minYqtt = np.min(qTTdf.values[np.nonzero(qTTdf.values)]) * .8
    maxYqtt = qTTdf.values.max()* 1.05


    #Bu asagidaki isimler onemli, sistemde(c++) kullanılan isimler.
    if debug:
        print("reorder columns!!")
    my_order = ['TS',
                'GiST-R-tree',
                'GridFile',
                'R*-tree',
                'STR-tree',
                'STRof-tree',
                'Inc-R-tree']
    qRTdf = qRTdf.reindex(columns=my_order)
    loaddf = loaddf.reindex(columns=my_order)
    qTTdf = qTTdf.reindex(columns=my_order)


    # now remove empty columns..
    qRTdf=qRTdf.dropna(axis=1, how='all')
    loaddf=loaddf.dropna(axis=1, how='all')
    qTTdf=qTTdf.dropna(axis=1, how='all')


    dlabels = ['db' + str(i) for i in range(len(loaddf))]
    qlabels = ['qb' + str(i) for i in range(len(loaddf))]
    loaddf.index=dlabels
    qTTdf.index=qlabels

    if debug:
        print("\nQRTs:::\n", qRTdf)    # for validation
        print("Min-Max:",minYqt, maxYqt)
        print("\nLoad time @ each Batch:::\n", loaddf)
        print("Min-Max:",minYlt, maxYlt)
        print("\nTotal RTs @ each Batch:::\n", qTTdf)
        print("Min-Max:",minYqtt, maxYqtt)

    barwidth = 0.08
    wlsize=len(qRTdf)*1.2  # workload size.Total num of queries.

    # TS requires logscale y
    setlogy=True
    if 'TS' in loaddf.columns:
        setlogy=True

    #gene de ihtiyac olursa: Bazen STRof-tree cok artabiliyor. O zaman ihtiyac olabilir..
    # setlogy=False

    imgfile=os.path.join(sc.pltdir,"TotalLoadTimesAndTotalQRT")
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 1200, 800',\
    #                      output = '"'+ imgfile +'.png"')
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+ imgfile +'.pdf"')
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 enhanced size 1800, 1200',\
    #                      output = '"'+ imgfile +'.png"')

    # k satır * 1 kolon
    g.cmd(f'set multiplot layout 2, 1 ')  # İlk değer yatay boşluk (sütunlar arası), ikinci değer dikey boşluk (satırlar arası) içindir.
    if setlogy:
        g.cmd('set logscale y')  # bu asagidakilerin hepsi icin gecerli olacak..

    # LOAD TIMES @ each BATCH:::
    g.cmd(
        f'set xrange[-1:{len(loaddf)+1}]',
      f'set yrange[{minYlt}:{maxYlt}]',
      # 'set mytics 4',
      # 'set mxtics 10',
       # 'set logscale y',
      # f'set xlabel "Batches"',
      f'set ylabel "Total Load time[msec]"',
      f'set title "Total Latency at each data batch (db_i)" ',
      f'set title  font ",6" norotate',
      # f'set title ""',
      # 'set label "(a) Total Latency at each data batch" at graph 0.5, graph -0.2 center font ",7"',
      f'set xtics norotate', #veya rotate by -45
      'set key left bottom',
      'set ytics'
      )
    plot_commands = []
    color=""
    for j, column in enumerate(loaddf.columns,start=0):
        if column == "GiST-R-tree":
            color = "blue"
        elif column == "STR-tree":
            color = "purple"
        elif column == "STRof-tree":
            color = "green"
        elif column == "Inc-R-tree":
            color = "red"
        elif column == "R*-tree":
            color = "cyan blue"
        else:
            color = "black"
        if j == 0:
            plot_command = f'using {j+2}:xtic(1) w boxes  lc rgb "{color}" title "{column}"' #plot_command = f'using {j+2}:xtic(1) w boxes  title "{column}-{int(loaddf[column].sum())} msec"'
        else:
            plot_command = f'using ($0+{j*barwidth}):{j+2} w boxes  lc rgb "{color}" title "{column}"' #plot_command = f'using ($0+{j*barwidth}):{j+2} w boxes  title "{column}-{int(loaddf[column].sum())} msec"'
        plot_commands.append(plot_command)
    # if debug:
    #     print(plot_commands)
    g.plot_data(loaddf, *plot_commands,
                offset = '0,0',
                boxwidth = '%s' %(barwidth),
                style = ['data boxplot', 'fill solid 0.5 border -1'])

    # TOTAL QRT @ each BATCH:::
    g.cmd(
        f'set xrange[-1:{len(qTTdf)+1}]',
      f'set yrange[{minYqtt}:{maxYqtt}]',
      # 'set mytics 4',
      # 'set mxtics 10',
      # 'set logscale y',
      # f'set xlabel "Batches"',
      f'set ylabel "Total QRT[msec]"',
      f'set title "Total Latency at each query batch (qb_i)" ',
      'set title  font ",6" norotate',
      # 'unset label',
      # 'set label "(b) Total Query Response Time at each batch" at graph 0.5, graph -0.2 center font ",7"',
      # f'set xtics norotate' #veya rotate by -45
      'set key left bottom',
      'set ytics'
      )
    plot_commands = []
    for j, column in enumerate(qTTdf.columns,start=0):
        if column == "GiST-R-tree":
            color = "blue"
        elif column == "STR-tree":
            color = "purple"
        elif column == "STRof-tree":
            color = "green"
        elif column == "Inc-R-tree":
            color = "red"
        elif column == "R*-tree":
            color = "cyan blue"
        else:
            color = "black"
        if j == 0:
            plot_command = f'using {j+2}:xtic(1) w boxes  lc rgb "{color}" title "{column}"' #plot_command = f'using {j+2}:xtic(1) w boxes  title "{column}-{int(qTTdf[column].sum())} msec"'
        else:
            plot_command = f'using ($0+{j*barwidth}):{j+2} w boxes  lc rgb "{color}" title "{column}"' #plot_command = f'using ($0+{j*barwidth}):{j+2} w boxes  title "{column}-{int(qTTdf[column].sum())} msec"'
        plot_commands.append(plot_command)
    if debug:
        print(plot_commands)
    g.plot_data(qTTdf, *plot_commands,
                offset = '0,0',
                boxwidth = '%s' %(barwidth),
                style = ['data boxplot', 'fill solid 0.5 border -1'])
    g.cmd(f'unset multiplot')

    imgfile=os.path.join(sc.pltdir,"Individual QRT")
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+ imgfile +'.pdf"')
    # INDIVIDUAL QUERY RES. TIMES:::
    g.cmd(f'set xrange[150:15100]', # start from 1 due to logscale. To see the first query RT, i re-set index to 1 above  {QSInaBatch}:{wlsize+100}
      f'set yrange[{minYqt}:{maxYqt}]',
      # 'set mytics 4',
      'set xtics 1000',
      f'set xlabel "Query sequence"',
      f'set ylabel "Q-Res.time[msec]"',
      'set style data points',
      'set pointsize 0.2',
      # 'set logscale x',
      # f'set xtics ({QSInaBatch})',  #'set xtics (500,1000)',
      'set logscale y',
      f'set title "Individual Query Response Time" ',
      'set title  font ",6" norotate',
      # f'set title ""',
      # 'unset label',
      # 'set label "(c) Individual Query Response Time" at graph 0.5, graph -0.15 center font ",7"',
    'set key left top'
      )
    # qRTdf = qRTdf[["STR-tree","STRof-tree", "Inc-R-tree"]]
    # qRTdf = qRTdf[["R*-tree", "Inc-R-tree"]]
    plot_commands = []
    for i, column in enumerate(qRTdf.columns, start=0):
        if column == "GiST-R-tree":
            color = "blue"
        elif column == "STR-tree":
            color = "purple"
        elif column == "STRof-tree":
            color = "green"
        elif column == "Inc-R-tree":
            color = "red"
        elif column == "R*-tree":
            color = "cyan blue"
        else:
            color = "black"
        plot_command = f'using 1:{i+2} w lp ps 0.1 lc rgb "{color}" lw 0.1 title "{column}"'
        plot_commands.append(plot_command)
    g.plot_data(qRTdf, *plot_commands)



# Onceki ile hemen heman aynı fakat sadece cumQRTs tek plot ediliyor.
# Processing plt/stats-spat  file. Incremental_test_4-stats kullanıyor..
# qt, q_put u icermedigi icin onu eklemek gerekti. (Onceki testlerde qt: q_in+q_last+q_del+q_put  idi..)
# Bir de qt'ye lt'yi ekledim kisadece qt uzerinden cumulative yapabileyim..Zaten lt varsa qt=0
# Regular veya Mixquery. # Mixfile queries, data, queries,...queries formatında olmalı. En son queries olmalı!!!
# Incremental_test_4-stats bunu kullanıyor.

def plot_cumQRTs(file, debug):
    print('''\nPLOTTING...generating cumulative query response times. :::''')
    df = pd.read_csv(file, sep=' ', header=None, names = ['name', 'qt','q_is','q_ls','q_del', 'q_put','res1', 'res2','lt'])  # names = ['name', 'a','b','c','d']
    df['qt'] = df['qt'] + df['q_put'] + df['lt'];

    algset=set(df['name'])
    dseq={} # dictionary
    for alg in algset:
        algrows=df[df['name']==alg]  # subdf for each algorithm
        dseq.update({alg:algrows})
    # del dseq["R*-tree"]
    cumdf = pd.DataFrame()
    for key, subdf in dseq.items():
        RTs=subdf['qt']   #  it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        RTs=pd.Series(RTs.to_numpy().cumsum())
        cumdf[key]=RTs # yeni sutun ekle..
    minYcum = np.min(cumdf.values[np.nonzero(cumdf.values)]) * .8
    maxYcum = cumdf.values.max() * 1.05

    #Bu asagidaki isimler sistemde(c++) kullanılan isimler.
    # print("reorder columns!!")
    my_order = ['TS',
                'GiST-R-tree',
                'GridFile',
                'R*-tree',
                'STR-tree',
                'STRof-tree',
                'Inc-R-tree']
    cumdf = cumdf.reindex(columns=my_order)
    cumdf=cumdf.dropna(axis=1, how='all')
    if debug:
        print("\nCumQRTs:::\n", cumdf)    # for validation
        print("Min-Max:",minYcum, maxYcum)
    cumwlsize=len(cumdf)  # workload size.Total loadings + num of queries.

    # TS requires logscale y
    setlogy=False
    if 'TS' in cumdf.columns:
        setlogy=True

    #gene de ihtiyac olursa:
    setlogy=True

    imgfile=os.path.join(sc.pltdir,"CumQRTs")
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 1200, 800',\
    #                      output = '"'+ imgfile +'.png"')
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+ imgfile +'.pdf"')
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 enhanced size 1800, 1200',\
    #                      output = '"'+ imgfile +'.png"')
    if setlogy:
        g.cmd('set logscale y')  # bu asagidakilerin hepsi icin gecerli olacak..

    # CUMULATIVE QRTs::::
    g.cmd(
      f'set xrange[0:{cumwlsize}+1]', # start from 1 due to logscale. To see the first query RT, i re-set index to 1 above
      f'set yrange[{minYcum}:{maxYcum}]',
      # 'set mytics 4',
      # 'set mxtics 10',
      f'set xlabel "Query sequence"',
      f'set ylabel "Q-Res.time[msec]"',
      # 'set logscale y',
      'set style data points',
      'set pointsize 0.2',
      f'set title "Cumulative Query Response Time" ',
      'set title  font ",10" norotate',
      'set key right bottom')
    plot_commands = []
    color=""
    for i, column in enumerate(cumdf.columns, start=0):
        if column == "GiST-R-tree":
            color = "blue"
        elif column == "STR-tree":
            color = "purple"
        elif column == "STRof-tree":
            color = "green"
        elif column == "Inc-R-tree":
            color = "red"
        elif column == "R*-tree":
            color = "cyan blue"
        else:
            color = "black"
        plot_command = f'using 1:{i+2} w lp  ps 0.1 lc rgb "{color}" title "{column}"'
        plot_commands.append(plot_command)
    g.plot_data(cumdf, *plot_commands)

# Regular veya Mixquery file icin valid. qt == 0 ise loadtime(lt) bir sonraki sorgu qt sine ekliyoruz.
# Mixfile queries, data, queries,...queries formatında olmalı. En son queries!!!

def plotQRT1(file):
    print ("Generating QRTs...")
    df = pd.read_csv(file, sep=' ', header=None, names = ['name', 'qt','q_is','q_ls','q_del', 'q_put','res1', 'res2','lt'])  # names = ['name', 'a','b','c','d']
    algset=set(df['name'])
    dseq={} # dictionary
    for alg in algset:
        algrows=df[df['name']==alg]  # subdf for each algorithm
        dseq.update({alg:algrows})

    df = pd.DataFrame()
    for key, subdf in dseq.items():
        subdf = subdf.reset_index(drop=True)
        # detect load-related-rows and then drop all
        rows_to_drop = []
        for i in range(len(subdf)):
            if subdf.at[i, 'qt'] == 0:
                if i+1 < len(subdf):
                    subdf.at[i+1, 'qt'] += subdf.at[i, 'lt']
                rows_to_drop.append(i)

        subdf = subdf.drop(rows_to_drop)
        # print(subdf)
        RTs=subdf['qt']
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        print(RTs)
        df[key]=RTs # mevcut df ye yeni bir sutun ekleme.
    minY = np.min(df.values[np.nonzero(df.values)]) * .8
    maxY = df.values.max() * 1.2
    print(df)    # for validation
    print(minY, maxY)
    wlsize=len(df)*1.2  # workload size.Total num of queries.
    # df.index += 1  # default start from 0. Pull it to 1.
    # plot("Cumulative Query Response Time",1,wlsize,minY,maxY,"Q seq", "Cum-Q-RT[msec]",df,"")
    g = gnuplot.Gnuplot()    # use this instance in pydev console
    # When execute .py file and store the figure in a file comment above, uncomment below.
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 600, 400',output = '"Fig2b.png"')
    g.cmd(f'set xrange[1:{wlsize}]', # start from 1 due to logscale. To see the first query RT, i re-set index to 1 above
      f'set yrange[{minY}:{maxY}]',
      # 'set mytics 4',
      # 'set mxtics 10',
      f'set xlabel "Query sequence"',
      f'set ylabel "Q-Res.time[msec]"',
      'set logscale x',
      'set logscale y',
      f'set title "Query Response Time" ',
      'set title  font ",10" norotate')
    plot_commands = []
    for i, column in enumerate(df.columns, start=0):
        plot_command = f'using 1:{i+2} w lp  title "{column}"'
        plot_commands.append(plot_command)
    g.plot_data(df, *plot_commands)

    # g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ',\
    #                 f'using 1:3 w lp  title "{df.columns[1]}" ',\
    #                 f'using 1:4 w lp  title "{df.columns[2]}" '
    #                 )
    input()

# # run32-mr kullanıyor.
# def plotQRT1(file):
#     print ("Generating QRTs...Special case: First QRT = Load time + QRT")
#     df = pd.read_csv(file, sep=' ', header=None)  # names = ['name', 'a','b','c','d']
#     algset=set(df.iloc[:,0])
#     dseq={} # dictionary
#     for alg in algset:
#         algrows=df[df[0]==alg]
#         print(type(algrows))
#         dseq.update({alg:algrows})
#
#     df = pd.DataFrame()
#     for key, value in dseq.items():   # RTs below is pandas.series.  plot need dataframe!!
#         RTs=value[1]
#         # RTs=value[2]+value[3]+value[4]+value[5]  #
#         # print(RTs.index)
#         RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
#         loadtime=RTs.iloc[0]
#         q1=RTs.iloc[1]
#         print(loadtime,q1)
#         RTs.iloc[1]= loadtime+q1
#         print(RTs.iloc[1])
#         RTs2=RTs[1:]
#         print(RTs2.iloc[0])
#         df[key]=RTs2 # mevcut df ye yeni bir sutun ekleme.
#
#     minY = np.min(df.values[np.nonzero(df.values)]) * .8
#     # minY = df.values.min() * .8
#     maxY = df.values.max() * 1.2
#     print(df)    # for validation
#     print(minY, maxY)
#     wlsize=len(df)*1.2  # workload size.Total num of queries.
#     # df.index += 1  # default start from 0. Pull it to 1.
#     # plot("Cumulative Query Response Time",1,wlsize,minY,maxY,"Q seq", "Cum-Q-RT[msec]",df,"")
#     g = gnuplot.Gnuplot()    # use this instance in pydev console
#     # When execute .py file and store the figure in a file comment above, uncomment below.
#     # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 600, 400',output = '"Fig2b.png"')
#     g.cmd(f'set xrange[1:{wlsize}]', # start from 1 due to logscale. To see the first query RT, i re-set index to 1 above
#       f'set yrange[{minY}:{maxY}]',
#       # 'set mytics 4',
#       # 'set mxtics 10',
#       f'set xlabel "Query sequence"',
#       f'set ylabel "Q-Res.time[msec]"',
#       'set logscale x',
#       'set logscale y',
#       f'set title "Query Response Time" ',
#       'set title  font ",10" norotate')
#     plot_commands = []
#     for i, column in enumerate(df.columns, start=0):
#         plot_command = f'using 1:{i+2} w lp  title "{column}"'
#         plot_commands.append(plot_command)
#     g.plot_data(df, *plot_commands)
#
#     # g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ',\
#     #                 f'using 1:3 w lp  title "{df.columns[1]}" ',\
#     #                 f'using 1:4 w lp  title "{df.columns[2]}" '
#     #                 )
#     input()

# Used in evaluating the [Part-]B-tree including Load time & Query RTs in the WL. Cumulative manner
# run32-mr kullanıyor.
def plotCumLQRT(file):
    print ("Generating Cumulative QRTs...Including Loading time")
    df = pd.read_csv(file, sep=' ', header=None)  # names = ['name', 'a','b','c','d']
    algset=set(df.iloc[:,0])
    dseq={}
    for alg in algset:
        algrows=df[df[0]==alg]
        dseq.update({alg:algrows})

    df = pd.DataFrame()
    for key, value in dseq.items():
        RTs=value[1]   #  it is pandas.series.  plot need dataframe!!
        # RTs=value[2]+value[3]+value[4]+value[5]
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        RTs=pd.Series(RTs.to_numpy().cumsum())
        df[key]=RTs # mevcut df ye yeni bir sutun ekleme.

    minY = np.min(df.values[np.nonzero(df.values)]) * .8
    maxY = df.values.max() * 1.2
    print(df)    # for validation
    print(minY, maxY)
    wlsize=len(df)  # workload size.Total num of queries.
    # df.index += 1  # default start from 0. Pull it to 1.

    g = gnuplot.Gnuplot()    # use this instance in pydev console
    g.cmd( 'set multiplot', 'set origin 0,0', 'set size 1,1',
        f'set xrange[0:{wlsize}+1]', # start from 1 due to logscale. To see the first query RT, i re-set index to 1 above
      f'set yrange[{minY}:{maxY}]',
      'set mytics 4',
      'set mxtics 10',
      f'set xlabel "Query sequence"',
      f'set ylabel "Q-Res.time[msec]"',
      'set logscale y',
      f'set title "Cumulative Query Response Time" ',
      'set title  font ",10" norotate')
    plot_commands = []
    for i, column in enumerate(df.columns, start=0):
        plot_command = f'using 1:{i+2} w lp  title "{column}"'
        plot_commands.append(plot_command)
    g.plot_data(df, *plot_commands)
    # g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ',\
    #                 f'using 1:3 w lp  title "{df.columns[1]}" ',\
    #                 f'using 1:4 w lp  title "{df.columns[2]}" '
    #                 )

    # bu asagidaki multiplot, plot icinde kucuk plot..
    # g.cmd( 'set origin 0.1,0.1', 'set size 0.3,0.3',
    #     f'set xrange[0:10]', # start from 1 due to logscale. To see the first query RT, i re-set index to 1 above
    #     f'set yrange[{minY}:{maxY}]',
    #     'unset key','unset xlabel','unset ylabel')
    # g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ',\
    #                 f'using 1:3 w lp  title "{df.columns[1]}" ',\
    #                 f'using 1:4 w lp  title "{df.columns[2]}" ')
    g.cmd( 'unset multiplot')
    input()



# Used in evaluating the breakdown of Part-B-tree QRT. qtot = qin+qlast
# run32-mr kullanıyor.

def plotQRT_breakdownWithSelectivity(file,queryfile):
    print ("QRT's Breakdown (with Selectivity)...")
    df = pd.read_csv(file, sep=' ', header=None)  # names = ['name', 'a','b','c','d']
    algset=set(df.iloc[:,0])
    dseq={}
    for alg in algset:
        algrows=df[df[0]==alg]
        algrows=algrows.drop(algrows.head(1).index)
        dseq.update({alg:algrows})
    # print(dseq)
    df = pd.DataFrame()
    for key, value in dseq.items():
        RTs=value[1]   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qtot']=RTs # mevcut df ye yeni bir sutun ekleme.

        RTs=value[2]   # + value[4] buna gerek yok. cunku q_is, q_del iceriyor!!!   (innersearch+delete)  it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qin']=RTs # mevcut df ye yeni bir sutun ekleme.

        RTs=value[3] + value[5]  # (lastpartsearch + insert) it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qlast']=RTs # mevcut df ye yeni bir sutun ekleme.

    # "B+tree-qtot",
    df.drop(["B+tree-qtot","B+tree-qlast","Part-B-tree-qtot"],axis=1,inplace=True)
    df=df[["Part-B-tree-qin","Part-B-tree-qlast","B+tree-qin"]]
    minY = np.min(df.values[np.nonzero(df.values)]) * .8
    maxY = df.values.max() * 2
    print(df)# for validation
    print(minY, maxY)
    wlsize=len(df)  # workload size.Total num of queries.
    df.index += 1  # default start from 0. Pull it to 1.
    df_query = pd.read_csv(queryfile, sep=' ', header=None, names = ['querymode','qid','low','high','sel'])
    # df_query=df_query[['sel']]
    df_query.index += 1
    df['sel']=df_query['sel']
    # print(df)
    # plotHist("Breakdown of Query Response Time",0,wlsize+1,minY,maxY,"Q seq", "Q-RT[msec]",df)
    width = 0.35       # the width of the bars:
    g = gnuplot.Gnuplot()
    g.plot_data(df,
            'using :($2 + $3):xtic(1) with boxes title "innerpartitions(search+deletes)" lc "dark-orange"',
            'using :3  with boxes title "lastpartition(search+insert)" lc "royalblue"',  # :2 2.kolon degerini
            'using ($0 + 0.35):4  with boxes title "b-tree" lc "black"',
            'using :5 axes x1y2 with lines title "Selectivity" lc "red" ',  #smooth bezier  can be added. but seems not good.
            style = ['data boxplot', 'fill solid 0.5 border -1'],
            boxwidth = '%s' %(width),
            key = 'font "Helvetica,10"',   # font size of legend
            xrange = f'[0:{wlsize}+1]',
            xlabel = '"Query Sequence"',
            xtics = 'font "Helvetica,5"',   # font size of x-labels
            yrange = f'[{minY}:{maxY}]',
            ylabel = '"Q-Res.time[msec]"',
            ytics = 'font "Helvetica,10"',
            mytics = '4',   # minor y tics
            y2range = '[0:100]',
            y2label = '"Selectivity (%)"',
            y2tics = 'font "Helvetica,10"',
            title = '"Breakdown of Query Response Time"',
            # output = '"PartB-tree-QRT_breakdown.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()

# Used in evaluating the breakdown of Part-B-tree QRT. qtot = qin+qlast
# run32-spatial kullanıyor.
def plotQRT_breakdown(file):
    print ("QRT's Breakdown...")
    df = pd.read_csv(file, sep=' ', header=None)  # names = ['name', 'a','b','c','d']
    algset=set(df.iloc[:,0])
    dseq={}
    for alg in algset:
        algrows=df[df[0]==alg]
        algrows=algrows.drop(algrows.head(1).index)
        dseq.update({alg:algrows})
    # print(dseq)
    df = pd.DataFrame()
    for key, value in dseq.items():
        RTs=value[1]   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qtot']=RTs # mevcut df ye yeni bir sutun ekleme.

        RTs=value[2]   # + value[4] buna gerek yok. cunku q_is, q_del iceriyor!!!   (innersearch+delete)  it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qin']=RTs # mevcut df ye yeni bir sutun ekleme.

        RTs=value[3] + value[5]  # (lastpartsearch + insert) it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qlast']=RTs # mevcut df ye yeni bir sutun ekleme.

    # "B+tree-qtot",
    df.drop(["B+tree-qtot","B+tree-qlast","Part-B-tree-qtot"],axis=1,inplace=True)
    df=df[["Part-B-tree-qin","Part-B-tree-qlast","B+tree-qin"]]
    minY = np.min(df.values[np.nonzero(df.values)]) * .8
    maxY = df.values.max() * 2
    print(df)# for validation
    print(minY, maxY)
    wlsize=len(df)  # workload size.Total num of queries.
    df.index += 1  # default start from 0. Pull it to 1.
    # print(df)
    # plotHist("Breakdown of Query Response Time",0,wlsize+1,minY,maxY,"Q seq", "Q-RT[msec]",df)
    width = 0.35       # the width of the bars:
    g = gnuplot.Gnuplot()
    g.plot_data(df,
            'using :($2 + $3):xtic(1) with boxes title "innerpartitions(search+deletes)" lc "dark-orange"',
            'using :3  with boxes title "lastpartition(search+insert)" lc "royalblue"',  # :2 2.kolon degerini
            'using ($0 + 0.35):4  with boxes title "b-tree" lc "black"',
            # 'using :5 axes x1y2 with lines title "Selectivity" lc "red" ',  #smooth bezier  can be added. but seems not good.
            style = ['data boxplot', 'fill solid 0.5 border -1'],
            boxwidth = '%s' %(width),
            key = 'font "Helvetica,10"',   # font size of legend
            xrange = f'[0:{wlsize}+1]',
            xlabel = '"Query Sequence"',
            xtics = 'font "Helvetica,5"',   # font size of x-labels
            yrange = f'[{minY}:{maxY}]',
            ylabel = '"Q-Res.time[msec]"',
            ytics = 'font "Helvetica,10"',
            mytics = '4',   # minor y tics
            # y2range = '[0:100]',
            # y2label = '"Selectivity (%)"',
            # y2tics = 'font "Helvetica,10"',
            title = '"Breakdown of Query Response Time"',
            # output = '"PartB-tree-QRT_breakdown.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()

# Kullanan yok. Kontrol edilmesi gerek !!!
def plotQRT_breakdown_backup(file):
    print ("QRT's Breakdown...")
    df = pd.read_csv(file, sep=' ', header=None)  # names = ['name', 'a','b','c','d']
    algset=set(df.iloc[:,0])
    dseq={}
    for alg in algset:
        algrows=df[df[0]==alg]
        algrows=algrows.drop(algrows.head(1).index)
        dseq.update({alg:algrows})
    # print(dseq)
    df = pd.DataFrame()
    maxY=0
    minY=float('inf')
    for key, value in dseq.items():
        RTs=value[1]   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qtot']=RTs # mevcut df ye yeni bir sutun ekleme.
        maxY=max(maxY,max(RTs))
        minY=min(minY,min(RTs))

        RTs=value[2]   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qin']=RTs # mevcut df ye yeni bir sutun ekleme.
        maxY=max(maxY,max(RTs))
        minY=min(minY,min(RTs))

        RTs=value[3]   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
        RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key+'-qlast']=RTs # mevcut df ye yeni bir sutun ekleme.
        maxY=max(maxY,max(RTs))
        minY=min(minY,min(RTs))
    # "B+tree-qtot",
    df.drop(["B+tree-qtot","B+tree-qlast","Part-B-tree-qtot"],axis=1,inplace=True)
    df=df[["Part-B-tree-qin","Part-B-tree-qlast","B+tree-qin"]]
    print(df)  # for validation
    # print(minY)
    # print(maxY)
    maxY=maxY*1.2
    minY=minY*.8
    wlsize=len(df)  # workload size.Total num of queries.
    df.index += 1  # default start from 0. Pull it to 1.
    print(df)
    # plotHist("Breakdown of Query Response Time",0,wlsize+1,minY,maxY,"Q seq", "Q-RT[msec]",df)
    width = 0.35       # the width of the bars:
    g = gnuplot.Gnuplot()
    g.plot_data(df,
            'using :($2 + $3):xtic(1) with boxes title "qin" lc "dark-orange"',
            'using :3  with boxes title "qlast" lc "royalblue"',  # :2 2.kolon degerini
            'using ($0 + 0.35):4  with boxes title "b-tree" lc "black"',
            style = ['data boxplot', 'fill solid 0.5 border -1'],
            boxwidth = '%s' %(width),
            key = 'font "Helvetica,10"',   # font size of legend
            xrange = f'[0:{wlsize}+1]',
            xlabel = '"Query Sequence"',
            xtics = 'font "Helvetica,5"',   # font size of x-labels
            yrange = f'[{minY}:{maxY}]',
            ylabel = '"Q-Res.time[msec]"',
            ytics = 'font "Helvetica,10"',
            mytics = '4',   # minor y tics
            title = '"Breakdown of Query Response Time"',
            # output = '"PartB-tree-QRT_breakdown.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()

# Used in evaluating the MM cracking tests..
def plotQRT(file):
    df = pd.read_csv(file, sep='\t', names = ['name', 'qt','q_il','q_da','q_pf', 'ct',\
                                                      'c_il','c_do','c_iu','c_ip','sc','numOfProj', \
                                                      'covertype', 'numOfCoverAttrs'])

    type(df['name'])  # series
    algset=set(df['name'])
    dseq={}
    dcum={}
    for alg in algset:
        algrows=df[df['name']==alg]
        algcum=algrows.tail(1)
        algrows=algrows.drop(algrows.tail(1).index)
        dseq.update({alg:algrows})
        dcum.update({alg:algcum})

    #----------------------------------
    # query seq vs Query response time, QRT= qt+ct.  Fig 2.b in Ditrich's paper.
    #---------------------------------
    # a new data frame
    df = pd.DataFrame()
    maxY=0
    minY=float('inf')
    for key, value in dseq.items():
        qts=value['qt']   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
        cts=value['ct']
        totalRTs=qts+cts  # total Response Time  (RT)
        totalRTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
        df[key]=totalRTs # mevcut df ye yeni bir sutun ekleme.
        maxY=max(maxY,max(totalRTs))
        minY=min(minY,min(totalRTs))

    maxY=maxY*1.2
    minY=minY*.8
    wlsize=len(df)  # workload size.Total num of queries.
    df.index += 1  # default start from 0. Pull it to 1.
    print(df)
    # plot("Query Response Time",1,wlsize,minY,maxY,"Q seq", "Q-RT[msec]",df,"")
    g = gnuplot.Gnuplot()    # use this instance in pydev console
    # When execute .py file and store the figure in a file comment above, uncomment below.
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 600, 400',output = '"Fig2b.png"')
    g.cmd(f'set xrange[1:{wlsize}+1]', # start from 1 due to logscale. To see the first query RT, i re-set index to 1 above
      f'set yrange[{minY}:{maxY}]',
      'set mytics 4',
      f'set xlabel "Q seq"',
      f'set ylabel "Q-RT[msec]"',
      # 'set logscale x',
      'set logscale y',
      f'set title "Query Response Time" ',
      'set title  font ",10" norotate')
    if df.columns.size ==1:
        g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ')  # sbezier can be added
    elif df.columns.size ==2:
        g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ',\
                    f'using 1:3 w lp  title "{df.columns[1]}" ')
    elif df.columns.size ==3:
        g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ',\
                    f'using 1:3 w lp  title "{df.columns[1]}" ',\
                    f'using 1:4 w lp  title "{df.columns[2]}" ')
    elif df.columns.size ==4:
        g.plot_data(df, f'using 1:2 w lp  title "{df.columns[0]}" ',\
                    f'using 1:3 w lp  title "{df.columns[1]}" ',\
                    f'using 1:4 w lp  title "{df.columns[2]}"  ',\
                    f'using 1:5 w lp  title "{df.columns[3]}" ')
    input()

def samplebar1():  # only 1 bar
    labels = ['G1', 'G2', 'G3', 'G4', 'G5']
    men_means = [20, 35, 30, 35, 27]
    women_means = [25, 32, 34, 20, 25]
    men_std = [2, 3, 4, 1, 2]
    women_std = [3, 5, 2, 3, 3]
    width = 0.35       # the width of the bars: can also be len(x) sequence
    # Plot programme:
    df = pd.DataFrame({'men_means': men_means,
        'women_means': women_means,
        'men_std': men_std,
        'women_std': women_std}, index = labels)
    print(df)
    # :1 index yani G1,G2,...,
    # :2 2.kolon men,  :3 3.kolon women,  :4 men's std,   :5 women's std
    gnuplot.plot_data(df,
            # 'using :($2 + $3):5:xticlabels(1) with boxerror title "women" lc "dark-orange"',
            'using :2 with boxes title "men" lc "royalblue"',  # :2 2.kolon degerini :4 boxerror degeri 4.kolonda
            # 'using :2:4 with boxerror title "men" lc "royalblue"',  # :2 2.kolon degerini :4 boxerror degeri 4.kolonda
            style = ['data boxplot', 'fill solid 0.5 border -1'],
            boxwidth = '%s' %(width),
            xrange = '[-1:5]',  # G1 0'a dayanmasın diye..
            yrange = '[0:100]',
            ylabel = '"Scores"',
            title = '"Scores by group and gender"',
            xtics = '("G1" 0, "G2" 1, "G3" 2, "G4" 3, "G5" 4)'
            # output = '"sphx_glr_bar_stacked_001.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()

def samplebar2a():  # clustered bars (yanyana) but with ONLY boxplot.
    labels = ['G1', 'G2', 'G3', 'G4', 'G5']
    men_means = [20, 35, 30, 35, 27]
    women_means = [25, 32, 34, 20, 25]
    men_std = [2, 3, 4, 1, 2]
    women_std = [3, 5, 2, 3, 3]
    width = 0.35       # the width of the bars: can also be len(x) sequence
    # Plot programme:
    df = pd.DataFrame({'men_means': men_means,
        'women_means': women_means,
        'men_std': men_std,
        'women_std': women_std}, index = labels)
    print(df)
    # :1 index yani G1,G2,...,
    # :2 2.kolon men,  :3 3.kolon women,  :4 men's std,   :5 women's std
    gnuplot.plot_data(df,
            # 'using :($2 + $3):5:xticlabels(1) with boxerror title "women" lc "dark-orange"',
            'using :2:4 with boxerror title "men" lc "royalblue"',  # :2 2.kolon degerini :4 boxerror degeri 4.kolonda. men boxerror ile çiz.
            'using ($0 + 0.35):3 with boxes title "women" lc "black"',  # women boxerror kulllanma, sadece box.
            style = ['data boxplot', 'fill solid 0.5 border -1'],
            boxwidth = '%s' %(width),
            xrange = '[-1:5]',  # G1 0'a dayanmasın diye..
            yrange = '[0:100]',
            ylabel = '"Scores"',
            title = '"Scores by group and gender"',
            # xtics = '("G1" 0, "G2" 1, "G3" 2, "G4" 3, "G5" 4)'
            # output = '"sphx_glr_bar_stacked_001.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()

def samplebar2b():    # rowstacked bars but with ONLY boxplot.  ( without "set style histogram rowstacked"  )
    labels = ['G1', 'G2', 'G3', 'G4', 'G5']
    men_means = [20, 35, 30, 35, 27]
    women_means = [25, 32, 34, 20, 25]
    men_std = [2, 3, 4, 1, 2]
    women_std = [3, 5, 2, 3, 3]
    width = 0.35       # the width of the bars: can also be len(x) sequence
    # Plot programme:
    df = pd.DataFrame({'men_means': men_means,
        'women_means': women_means,
        'men_std': men_std,
        'women_std': women_std}, index = labels)
    print(df)
    # :1 index yani G1,G2,...,
    # :2 2.kolon men,  :3 3.kolon women,  :4 men's std,   :5 women's std
    gnuplot.plot_data(df,
            'using :($2 + $3):5:xticlabels(1) with boxerror title "women" lc "dark-orange"',    # burada rowstack yapmış olduk!!
            'using :2:4 with boxerror title "men" lc "royalblue"',  # :2 2.kolon degerini :4 boxerror degeri 4.kolonda
            style = ['data boxplot', 'fill solid 0.5 border -1'],
            boxwidth = '%s' %(width),
            xrange = '[-1:5]',  # G1 0'a dayanmasın diye..
            yrange = '[0:100]',
            ylabel = '"Scores"',
            title = '"Scores by group and gender"',
            xtics = '("G1" 0, "G2" 1, "G3" 2, "G4" 3, "G5" 4)'
            # output = '"sphx_glr_bar_stacked_001.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()

def samplebar3():  # rowstacked with "set style data histograms" AND "set style histogram rowstacked".  Here ı did not use boxerror
    labels = ['G1', 'G2', 'G3', 'G4', 'G5']
    men_means = [20, 35, 30, 35, 27]
    women_means = [25, 32, 34, 20, 25]
    men_std = [2, 3, 4, 1, 2]
    women_std = [3, 5, 2, 3, 3]
    width = 0.35       # the width of the bars: can also be len(x) sequence
    # Plot programme:
    df = pd.DataFrame({'men_means': men_means,
        'women_means': women_means,
        'men_std': men_std,
        'women_std': women_std
        }, index = labels)
    print(df)
    gnuplot.plot_data(df,
            'using 2:xtic(1) title "men" lc "royalblue"',
            'using 3:xtic(1) title "women" lc "dark-orange"',
            style = ['data histograms', 'fill solid 0.5 border -1', 'histogram rowstacked'],
            boxwidth = '%s' %(width),
            xrange = '[0.5:5.5]',
            ylabel = '"Scores"',
            title = '"Scores by group and gender"',
            # output = '"sphx_glr_bar_stacked_001.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()

def samplebar4():  # rowstacked with clustering. Easier with boxplot. Because it seems we cannot replot with this library.
    labels = ['G1', 'G2', 'G3', 'G4', 'G5']
    men_means = [20, 35, 30, 35, 27]
    women_means = [25, 32, 34, 20, 25]
    men_std = [2, 3, 4, 1, 2]
    women_std = [3, 5, 2, 3, 3]
    width = 0.35       # the width of the bars: can also be len(x) sequence
    # Plot programme:
    df = pd.DataFrame({'men_means': men_means,
        'women_means': women_means,
        'men_std': men_std,
        'women_std': women_std
        }, index = labels)
    print(df)
    gnuplot.plot_data(df,
            'using :($2 + $3):xtic(1) with boxes title "women" lc "dark-orange"',
            'using :2  with boxes title "men" lc "royalblue"',  # :2 2.kolon degerini
            'using ($0 + 0.35):4  with boxes title "men-std" lc "black"',  # ($0 + 0.35)  this shifts bar
            style = ['data boxplot', 'fill solid 0.5 border -1'],
            boxwidth = '%s' %(width),
            xrange = '[0.5:5.5]',
            yrange = '[0:100]',
            ylabel = '"Scores"',
            title = '"Scores by group and gender"',
            # output = '"sphx_glr_bar_stacked_001.png"',
            # terminal = 'pngcairo size 640, 480'
            )
    input()


'''
SPATIAL DATA ::::used for region, point, MBR plottings..

Bu asagidakiler plot1, plot2,... spatial data test lerde kullanmak icin..
Mesela run32-spatial-validation bunlari cagiriyor..
dosyaları sc.pltdir altında  kaydediyor..
input file should be in format:   0 1 x0 y0 x1 y1   : plot takes only 2,3,4,5 columns..
attent to enter correct dspatialtype: region or point
'''
def plot1(file,obj_type,dspatialtype):

    imgfile=os.path.join(sc.pltdir,f'{obj_type}')
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 2400, 1600',\
    #                      output = '"'+ imgfile +'.png"')
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+ imgfile +'.pdf"')

    timestamp = datetime.datetime.now().strftime("%M%S%f")  #%f
    pltfile = f"{imgfile}_{timestamp}.plt"
    dss=0
    with open(file, 'r') as infile, open(pltfile, 'w') as outfile:
        for line in infile:
            if line.strip().startswith('#'):
                continue  # Yorum satırını atla
            else:
                dss+=1
                parts = line.split()
                x1, y1 = parts[2], parts[3]
                if dspatialtype=="region" :
                    x2, y2 = parts[4], parts[5]
                    outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
                else :
                    outfile.write(f"{x1} {y1}\n")
    g.cmd(f'set xrange[0:1]',
      f'set yrange[0:1]',
      # 'set mytics 4',
      'set xtics 0.25',
      'set ytics 0.25',
      f'set xlabel "x"',
      f'set ylabel "y"',
      f'set title "{dss} {obj_type}-{dspatialtype}" ',
      'set grid',
      'set grid lt 1 lw 1 lc rgb "gray" ',
      'set size square',
      'set title  font ",10" norotate')
    if(dspatialtype == "point"):
        g.plot(f' "{pltfile}" using 1:2 title "{obj_type}-point" w p  ps 0.2 pt 7 lc rgb "black"')  #ps 0.8 pt 15 idi
    else:
        g.plot(f' "{pltfile}" using 1:2 w l title "{obj_type}-region" lw 0.5 ')
    # os.remove(pltfile)  # pltfile i silersen
    # input()  # wait to see & save the plot. Then press enter to continue.

def plot2(file1,obj_type1,dspatialtype1,file2,obj_type2,dspatialtype2):

    imgfile=os.path.join(sc.pltdir,f'{obj_type1}'+f'{obj_type2}')
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 2400, 1600',\
    #                      output = '"'+ imgfile +'.png"')
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+ imgfile +'.pdf"')
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 enhanced size 1800, 1200',\
    #                      output = '"'+ imgfile +'.png"')
    timestamp = datetime.datetime.now().strftime("%M%S%f")
    pltfile1 = f"{imgfile}_{timestamp}_1.plt"
    dss1=0
    with open(file1, 'r') as infile, open(pltfile1, 'w') as outfile:
        for line in infile:
            if line.strip().startswith('#'):
                continue  # Yorum satırını atla
            else:
                dss1 +=1
                parts = line.split()
                x1, y1 = parts[2], parts[3]
                if dspatialtype1=="region" :
                    x2, y2 = parts[4], parts[5]
                    outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
                else :
                    outfile.write(f"{x1} {y1}\n")
    timestamp = datetime.datetime.now().strftime("%M%S%f")
    pltfile2 = f"{imgfile}_{timestamp}_2.plt"
    dss2=0
    with open(file2, 'r') as infile, open(pltfile2, 'w') as outfile:
        for line in infile:
            if line.strip().startswith('#'):
                continue  # Yorum satırını atla
            else:
                dss2 +=1
                parts = line.split()
                x1, y1 = parts[2], parts[3]
                if dspatialtype2=="region" :
                    x2, y2 = parts[4], parts[5]
                    outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
                else :
                    outfile.write(f"{x1} {y1}\n")

    g.cmd(f'set xrange[0:1.01]',
      f'set yrange[0:1.01]',
      'set mytics 4',
      f'set xlabel "x"',
      f'set ylabel "y"',
      # f'set title "{dss1} {obj_type1}-{dspatialtype1} AND {dss2} {obj_type2}-{dspatialtype2}"',
      'set grid',
      'set size square',
      'set title  font ",10" norotate',
      'set pointsize 0.2',
      'set key right bottom'
      )

    if dspatialtype1 == "region":
        dspatialtype1="lines"
    if dspatialtype2 == "region":
        dspatialtype2="lines"
    color="red"
    if "R*-tree" in obj_type1:
        color="cyan blue"
    elif "STR-tree" in obj_type1:
        color="purple"
    g.plot(f'"{pltfile1}" using 1:2 w {dspatialtype1} title "{dss1} {obj_type1}-{dspatialtype1}" lw 1 lc rgb "{color}"',
               f' "{pltfile2}" using 1:2 w {dspatialtype2} title "{dss2} {obj_type2}-{dspatialtype2}" lw 0.5 lc rgb "black"'
               )
    # if dspatialtype1 == "point" and dspatialtype2 == "region" :
    #     g.plot(f' "{pltfile1}" using 1:2 w p title "{obj_type1}-point" ps 0.2 pt 15 ',
    #            f' "{pltfile2}" using 1:2 w l title "{obj_type2}-region" lw 1 lc rgb "red" '
    #            )
    # else if:
    #     g.plot(f' "{pltfile}" using 1:2 w l title "{obj_type}-region" lw 1 ')

# Bu plot3'de "data", "query","result"  basmak icin.
def plot3(file1,obj_type1,dspatialtype1,file2,obj_type2,dspatialtype2,file3,obj_type3,dspatialtype3):
    imgfile=os.path.join(sc.pltdir,f'{obj_type1}'+f'{obj_type2}'+f'_{obj_type3}')
    # 2400,1600
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 1200, 800',\
    #                      output = '"'+ imgfile +'.png"')
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+ imgfile +'.pdf"')
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 enhanced size 1800, 1200',\
    #                      output = '"'+ imgfile +'.png"')

    timestamp = datetime.datetime.now().strftime("%M%S%f")
    pltfile1 = f"{imgfile}_{timestamp}_1.plt"
    dss1=0
    # with open(file1, 'r') as infile, open(pltfile1, 'w') as outfile:
    #     for line in infile:
    #         if line.strip().startswith('#'):
    #             continue  # Yorum satırını atla
    #         else:
    #             dss1 +=1
    #             parts = line.split()
    #             x1, y1 = parts[2], parts[3]
    #             if dspatialtype1=="region" :
    #                 x2, y2 = parts[4], parts[5]
    #                 outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
    #             else :
    #                 outfile.write(f"{x1} {y1}\n")

    data_outfile=open(pltfile1, 'w')
    with open(file1, 'r') as infile :
        for line in infile:
            if line.strip().startswith('#'):
                continue  # Yorum satırını atla
            else:
                dss1 +=1
                parts = line.split()
                x1, y1 = parts[2], parts[3]
                if dspatialtype1=="region" :
                    x2, y2 = parts[4], parts[5]
                    data_outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
                else :
                    data_outfile.write(f"{x1} {y1}\n")

    timestamp = datetime.datetime.now().strftime("%M%S%f")
    pltfile2 = f"{imgfile}_{timestamp}_2.plt"
    dss2=0
    with open(file2, 'r') as infile, open(pltfile2, 'w') as outfile:
        for line in infile:
            if line.strip().startswith('#'):
                continue  # Yorum satırını atla
            elif line.strip().startswith('1'):
                dss1 +=1
                parts = line.split()
                x1, y1 = parts[2], parts[3]
                if dspatialtype1=="region" :
                    x2, y2 = parts[4], parts[5]
                    data_outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
                else :
                    data_outfile.write(f"{x1} {y1}\n")
            else:
                dss2 +=1
                parts = line.split()
                x1, y1 = parts[2], parts[3]
                if dspatialtype2=="region" :
                    x2, y2 = parts[4], parts[5]
                    outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
                else :
                    outfile.write(f"{x1} {y1}\n")
    timestamp = datetime.datetime.now().strftime("%M%S%f")
    pltfile3 = f"{imgfile}_{timestamp}_3.plt"
    dss3=0
    # bu 3. dosya genelde result dosyası. Her sorgudan sonra 1 satır boslugu atlamak icin burada if line.strip():  var..
    with open(file3, 'r') as infile, open(pltfile3, 'w') as outfile:
        for line in infile:
            if line.strip():
                if line.strip().startswith('#'):
                    continue  # Yorum satırını atla
                else:
                    dss3 +=1
                    parts = line.split()
                    x1, y1 = parts[2], parts[3]
                    if dspatialtype3=="region" :
                        x2, y2 = parts[4], parts[5]
                        outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
                    else :
                        outfile.write(f"{x1} {y1}\n")

    g.cmd(f'set xrange[0:1.01]',
      f'set yrange[0:1.01]',
      'set mytics 4',
      f'set xlabel "x"',
      f'set ylabel "y"',
      # f'set title "{dss1} {obj_type1}-{dspatialtype1} AND {dss2} {obj_type2}-{dspatialtype2} \
      # AND {dss3} {obj_type3}-{dspatialtype3}"',
      # 'set grid',
      'set size square',
      'set title  font ",10" norotate',
      'set key right bottom')

    if dspatialtype1 == "point" and dspatialtype2 == "region" and dspatialtype3 == "point" :
        g.plot(f' "{pltfile1}" using 1:2 w p title "{dss1} {obj_type1}-points"  ps 0.01 pt 5 lc rgb "black" lw 0.1',
               f' "{pltfile2}" using 1:2 w l title "{dss2} {obj_type2}-regions" lw 0.1 lc rgb "red"',
               f' "{pltfile3}" using 1:2 w p title "{dss3} {obj_type3}-points" ps 0.01 pt 5 lc rgb "red" lw 0.1'
               )

  #   row-based filling..
  #   Mesela 1 agacın MBR değisimi icin   :         multiplot(rleafplts,1,3)   Burda rleafplts: 3 tane plot dosyası içermeli..
  #   3 farklı agacın 3 yuklemedeki değişimi için:  multiplot(rleafplts+inc_rleafplts+ws_inc_rleafplts,3,3)  herbiri 3 plot dosyası içermeli.
def multiplot(files,numOfrows,numOfcols,title):
    # imgfile=os.path.join(sc.pltdir,'leafMBRs-trend')
    imgfile=os.path.join(sc.pltdir,title)
    # 2400, 1600
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 size 1200, 800',\
    #                      output = '"'+ imgfile +'.png"')
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+ imgfile +'.pdf"')
    # g = gnuplot.Gnuplot (terminal = 'pngcairo font "arial,10" fontscale 1.0 enhanced size 1800, 1200',\
    #                      output = '"'+ imgfile +'.png"')

    # timestamp = datetime.datetime.now().strftime("%M%S%f")
    pltfiles=[]
    dss=[]  # a list
    for file in files:
        pltfile = f"{file}.plt"
        pltfiles.append(pltfile)
        count=0
        with open(file, 'r') as infile, open(pltfile, 'w') as outfile:
            for line in infile:
                count +=1
                parts = line.split()
                x1, y1 = parts[2], parts[3]
                x2, y2 = parts[4], parts[5]
                outfile.write(f"{x1} {y1}\n{x2} {y1}\n{x2} {y2}\n{x1} {y2}\n{x1} {y1}\n\n")
            dss.append(count)

    # plt_title="R*-tree's MBR-change  and  STR-tree's MBR-change"
    # plt_title=""
    # if 'rstar' in title:
    #     plt_title=" R*-tree's leaf-MBR-change  &  STR-tree's leaf-MBR-change db_0 qb_0 db_1 qb_1 db_2 qb_2 db_3 qb_3 db_4 qb_4"
    # else:
    #     plt_title=" STRof-tree's leaf-MBR-change  &  Incremental R-tree's leaf-MBR-change db_0 qb_0 db_1 qb_1 db_2 qb_2 db_3 qb_3 db_4 qb_4"

    #   row-based filling..
    #   numOfrows,numOfcols degistikce margin ve spacing degerlerini ayarlamak zor. deneyerek oluyor gibi..
    # margins 0.1, 0.9, 0.1, 0.9 komutu sol, sağ, alt ve üst marginleri (kenar boşluklarını) ayarlar.
    # 2 x 3 icin margins 0.1, 0.9, 0.1, 0.9 spacing 0.05, 0.01   iyi gibi.
    # 2 x 6 icin margins 0.1, 0.9, 0.5, 1.2 spacing 0.05, 0.01    iyi gibi..
    # 3 * 2 için default degerler iyi gibi..Yani bos birak..
    g.cmd(f'set multiplot layout {numOfrows}, {numOfcols}')  #title "{plt_title}" ','set title font "Arial,16" norotate')  # İlk değer yatay boşluk (sütunlar arası), ikinci değer dikey boşluk (satırlar arası) içindir.
    for i in range(len(files)):
        g.cmd(f'set xrange[0:1.01]',f'set yrange[0:1.01]',
          'set mytics 4',
          'set xtics font ",5"',
          'set ytics font ",5"',
          # f'set xlabel "x" font ",5"',
          # f'set ylabel "y" font ",5"',
          # f'set title "{dss[i]} MBRs" offset 0, -1',
          # 'set grid',
          'set size square',
          'set title  font ",5" norotate'   # graph title size
          # 'set key font ",4"',  # legend title size
          # 'set key right outside'
          )
        color=""
        # if "traditional" in title:
        #     if i<3 :
        #         g.cmd(f'set title "qb{i % 3}: {dss[i]} R*-leaves" offset 0, -0.8')
        #         color = "cyan blue"
        #     elif i<6:
        #         g.cmd(f'set title "qb{i % 3}: {dss[i]} STR-leaves" offset 0, -0.8')
        #         color = "purple"
        #     else:
        #         g.cmd(f'set title "qb{i % 3}: {dss[i]} STRof-leaves" offset 0, -0.8')
        #         color = "green"
        # else:
        #     color = "red"
        #     if i % 2 == 0:
        #         g.cmd(f'set title "qb{i//2}: {dss[i]} INC-R-leaves" offset 0, -0.8')
        #     else:
        #         g.cmd(f'set title "qb{i//2}: {dss[i]} INC-R-leaves" offset 0, -0.8')
        # g.plot(f' "{pltfiles[i]}" using 1:2 w l title "{dss[i]} Leaf-MBRs" lw 0.9 lc rgb "red" ')
        if i < 3:
            g.cmd(f'set title "qb{i % 3}: {dss[i]} R*-leaves" offset 0, -0.8')
            color = "cyan blue"
        elif i < 6:
            g.cmd(f'set title "qb{i % 3}: {dss[i]} STR-leaves" offset 0, -0.8')
            color = "purple"
        elif i<9:
            g.cmd(f'set title "qb{i % 3}: {dss[i]} STRof-leaves" offset 0, -0.8')
            color = "green"
        else:
            g.cmd(f'set title "qb{i % 3}: {dss[i]} Inc-R-leaves" offset 0, -0.8')
            color = "red"
        g.plot(f' "{pltfiles[i]}" using 1:2 notitle w l lc rgb "{color}" lw 0.1 ')

    g.cmd(f'unset multiplot')

def plotdist(file):
    outfile=os.path.join(sc.pltdir,'datadist.pdf')
    g = gnuplot.Gnuplot (terminal = 'pdfcairo enhanced color font "arial,7"',\
                         output = '"'+f'{outfile}'+'"')
    g.cmd(f'set xlabel "Time"',
          'set ylabel "Count"',
          'set xdata time',
          'set timefmt "%H:%M"',
          'set format x "%H:%M"'
          )
    g.plot( f'"{file}" using 1:2 with linespoints pointtype 3 pointsize 0.4 title "Pickups"')


#----------------------------------
# QRT comparison between others and BS.  Fig 5.a in Ditrich's paper.
# Assume we run BS, i.e they exists in output.txt
#---------------------------------
# if "BS" not in algset: # Full index is the baseline..
#     print(" BS is not run")
#     exit(0)
# # First obtain BStotalRTs from dseq.
# qts=dseq.get("BS")['qt']
# cts=dseq.get("BS")['ct']
# BStotalRTs=qts+cts
# BStotalRTs.reset_index(drop=True, inplace=True)  #reset index starting from 0
#
# df = pd.DataFrame()
# maxY=0
# minY=float('inf')
# for key, value in dseq.items():
#     if key == "BS":
#         continue
#     qts=value['qt']   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
#     cts=value['ct']
#     totalRTs=qts+cts  # total Response Time  (RT)
#     totalRTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
#     QRThigherthan=100*(totalRTs-BStotalRTs)/BStotalRTs
#     df[key]=QRThigherthan
#     maxY=max(maxY,max(QRThigherthan))
#     minY=min(minY,min(QRThigherthan))
#
# maxY=maxY*1.2
# minY=minY*.8
# wlsize=len(df)  # workload size.Total num of queries.
# df.drop(df.head(1).index,inplace=True)  # drop first. Because it contains BS built time.
# # df.index += 1
#
# # plot("QRT over Full Index",wlsize,maxY,"Q seq", "QRT HigherThan Full Index[%]",df)
# plot("QRT surplus over Full index",1,wlsize,minY,maxY,"Q seq", "QRT HigherThan Full Index[%]",df,"smooth bezier")
#
# #----------------------------------
# # QRT cumulative RT
# # Assume we run BS & SN to make comparison,
# #---------------------------------
# # Response Time (RT) analysis
# df = pd.DataFrame()
# maxY=0
# minY=float('inf')
# for key, value in dseq.items():
#     qts=value['qt']   # this is NOT dataframe !!!! it is pandas.series.  plot need dataframe!!
#     cts=value['ct']
#     RTs=qts+cts  # total Response Time  (RT)
#     RTs.reset_index(drop=True, inplace=True)  #reset index starting from 0. All algs. must start from same index.
#     cumRTs=pd.Series(RTs.to_numpy().cumsum())
#     df[key]=cumRTs
#     maxY=max(maxY,max(cumRTs))
#     minY=min(minY,min(cumRTs))
#
# maxY=maxY*1.2
# minY=minY*.8
# wlsize=len(df)  # workload size.Total num of queries.
# df.index += 1  # default start from 0. Pull it to 1.
#
# # plot("Cumulative Response Time",wlsize,maxY,"Q seq", "Cumulative RT[ms]",df)
# plot("Cumulative Response Time",1,wlsize,minY,maxY,"Q seq", "Cumulative RT[ms]",df,"")
#
#
# exit(0)  # to exclude the remaining.
# #----------------------------------
# # QRT comparison between SC and BS.  Fig 3.a in Ditrich's paper.
# # Assume we run SC and BS, i.e they exists in output.txt
# #---------------------------------
# if "SC" not in algset :
#     print(" SC is not run")
#     input()
#
# if "BS" not in algset: # Full index is the baseline..
#     print(" BS is not run")
#     input()
#
# # First obtain SCtotalRTs, BStotalRTs from dseq.
# qts=dseq.get("SC")['qt']
# cts=dseq.get("SC")['ct']
# SCtotalRTs=qts+cts
# SCtotalRTs.reset_index(drop=True, inplace=True)  #reset index starting from 0
# qts=dseq.get("BS")['qt']
# cts=dseq.get("BS")['ct']
# BStotalRTs=qts+cts
# BStotalRTs.reset_index(drop=True, inplace=True)  #reset index starting from 0
#
# QRThigherthan=100*(SCtotalRTs-BStotalRTs)/BStotalRTs
# maxY=max(QRThigherthan) * 1.2
# minY=min(QRThigherthan) * .8
# df = pd.DataFrame(data = {'': QRThigherthan
#                           })
# df.drop(df.head(1).index,inplace=True)  # drop first. Because it contains BS built time.
# # df.index += 1
#
# # plot("SC\'s QRT over Full Index",wlsize,maxY,"Q seq", "QRT HigherThan Full Index",df)
# plot("SC\'s QRT over Full Index",1,wlsize,minY,maxY,"Q seq", "QRT HigherThan Full Index",df,"")
#

if __name__ == "__main__": # it execute here if it were run as a standalone program..

    # To DEBUG w/ python console. start python console and set cwd. F.e.:
    pd.set_option('display.max_columns', None)  # Tüm sütunları göster
    # copy pltfile or stats file to this directory. Baska yolu yok. cunku burada sailconfig import ediyorsun. O da plt u siliyor.!!
    # plotQRT1("stats-spat")
    # multiplot_Load_TRT_QRTs("stats-spat",True)
    # plot_cumQRTs("stats-spat",True)
    # plotdist("distovertime")
    # plot1("rleafplt",'MBR','region')
    # stats=os.path.join(sc.pltdir,'stats-ALL')
    multiplot_Load_TRT_QRTs('stats-ALL',True)
    plot_cumQRTs('stats-ALL',False)

    # to debug queryfile, run the following plotting..
    # queryfile=os.path.join(sc.querydir, 'queryfile')
    # plot1(queryfile,'mixquery','region')
    # files=["plt-rleaf-0","plt-rleaf-1","plt-inc-rleaf-1","plt-inc-rleaf-2"]
    # multiplot(files,2,2) # yarısı ilk satırda, diger yarısı 2. satırda
    pd.reset_option('display.max_columns')
    # samplebar1()  # tek bar
    # samplebar2a()  # yanyana
    # samplebar2b()  # ustuste

