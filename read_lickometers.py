import serial 
import argparse
import csv
import numpy as np
from threading import Thread, Event
from datetime import date
from datetime import datetime as dt
import concurrent.futures
import time
import pandas as pd
import os

def stream_serial(mouse,folder,description,port,green_light):

    serial_port = serial.Serial(port,9600,timeout=1)
    columns=['datetime','time offset (ms)','left lick count','right lick count']
    data=pd.DataFrame(columns=columns)

    while serial_port.in_waiting>0:
        with open(f"{date.today().strftime('%Y%m%d')}_{port.split('/')[-1]}_old_data_pre_{mouse}_{description}.txt","a") as f:
            print('saving old data')
            f.write(serial_port.readline().decode("utf-8").strip())

    print('starting stream')
    while green_light.is_set():
        if serial_port.in_waiting>0:
            l=serial_port.readline().decode("utf-8").strip().split(',')
            if len(l)==3:
                data=data.append(dict(zip( columns, [dt.now()] + list(map(int,l)) )), ignore_index=True)
                print(l)
            elif l[0]=='stop':
                print('finished. waiting for KeyboardInterrupt')
                break
        else:
            time.sleep(.1)

    fname=f"{date.today().strftime('%Y%m%d')}_{mouse}_{description}.csv"
    while os.path.isfile( os.path.join(folder,fname) ):
        fname='updated_'+fname
    data.to_csv(os.path.join(folder,fname))
    return data

if __name__=="__main__": 

    parser = argparse.ArgumentParser()
    parser.add_argument("-pf","--ports_file", help = "address to file with ports",default='ports.txt')
    parser.add_argument("-pi","--ports_index", help = "indices of ports in use relative to the ports file",nargs="+",type=int,default=[])
    parser.add_argument("-m","--mice", help = "mice in order of the ports they are on",nargs="+",default=[])
    parser.add_argument("-d","--description", help = "description of the data to be collected",default="")
    parser.add_argument("-f","--file_loc", help = "address of the folder to save the data to",default="")


    args = parser.parse_args()

    if len(args.mice)==0:
        parser.error('must enter the identifier for at least one mouse')

    if len(args.ports_index)!=len(args.mice):
        parser.error('must specify the same number of port indices as mice')

    folder=os.path.join(args.file_loc,f"{date.today().strftime('%Y%m%d')}_{'_'.join(args.mice)}_{args.description}")
    if not os.path.isdir(folder):
        os.mkdir(folder)
    
    with open(args.ports_file) as f: ports=f.read().split()
    mp=[(m,ports[i]) for m,i in zip(args.mice,args.ports_index)]

    green_light=Event()
    green_light.set()

    with concurrent.futures.ThreadPoolExecutor() as executor:
        threads=[]
        for m,p in mp:
            threads.append(executor.submit(stream_serial,mouse=m,folder=folder,description=args.description,port=p,green_light=green_light))
        try:
            while True: time.sleep(.1)
        except KeyboardInterrupt:
            green_light.clear()
            print('stopping stream')
            for thread in concurrent.futures.as_completed(threads):
                print(thread.result())
            print("successfully stopped stream")
