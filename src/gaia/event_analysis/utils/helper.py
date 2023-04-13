import numpy as np
import numpy as np
import os
import pandas as pd
import struct
from utils.event2d import event2d as event2d
from  matplotlib.colors import LinearSegmentedColormap
import matplotlib.pyplot as plt
from scipy import signal
import glob 


def read_dat_file(fileName):
    '''
    given a .dat file returns the events in a list of event2d
    '''
    with open(fileName, mode='rb') as file: # b is important -> binary  
        fileContent = file.read()
    code ="QIIII" # long long -  int 
    unpacked = struct.unpack(code * ((len(fileContent)) // 24), fileContent)
    events = []
    for i in range(len(unpacked)//5):
        ev = event2d(unpacked[i*len(code)+0],unpacked[i*len(code)+1], unpacked[i*len(code)+2], unpacked[i*len(code)+3])
        events.append(ev)

    return events


def max_pixel(events): 
    '''
    takes a list of event2d events and returns the x-y position of the most common
    '''
    empty_matrix = np.zeros((75,75))
    for i in range(len(events)):
        empty_matrix[events[i].x][events[i].y] += 1
    max = np.argmax(empty_matrix)

    return (max//75,max%75)


def gaia1_extractor(events):
    '''
    takes a list of event2d events and returns only the test pixel events
    '''
    tp_events = []
    for i in range(len(events)):
        if (events[i].x == 74 and events[i].y == 67) or (events[i].x == 67 and events[i].y == 74):
            tp_events.append(events[i])

    return tp_events

def gaia9_extractor(events):
    '''
    takes a list of event2d events and returns only the test array events
    '''
    ta_events = []
    for i in range(len(events)):
        if events[i].x > 68 and events[i].y > 68:
            ta_events.append(events[i])

    return ta_events

def gaia4096_extractor(events):
    '''
    takes a list of event2d events and returns only the test array events
    '''
    gaia_events = []
    for i in range(len(events)):
        if events[i].x < 65 and events[i].y < 65:
            gaia_events.append(events[i])

    return gaia_events

def get_event_rate(events, sampling_rate= 1e-5):
    '''
    given a list of events returns the mean number of events per second
    '''
    return len(events)/((events[-1].ts - events[0].ts)*sampling_rate)

def remove_ts_offset(events):
    '''
    given a list of events returns the same list of events, offseting the ts
    '''
    start_time = events[0].ts
    for i in range(len(events)):
        events[i].ts = events[i].ts - start_time
    return


def select_event_in_timewindow(events, t_start =0, t_stop=0.1, sampling_rate = 1e-5):
    '''
    given a list of events returns events in a defined time window (in ms)
    '''
    new_list = []

    for e in events:
        if e.ts > t_start/sampling_rate and e.ts < t_stop/sampling_rate :
            new_list.append(e)

    return new_list

def select_random_channels(events, lenght) :

    new_list = []
    rand_x = np.random.randint(0, 4095, lenght)
    rand_y = np.random.randint(0, 4095, lenght) 

    for e in events:
        for ch in range(lenght):
            if ((e.x == rand_x[ch]) and( e.y == rand_y[ch])):
             #   print(e.x, e.y)
                new_list.append(e)  
                print(new_list)

    return new_list


def unscrable_g9(events):
    '''
    given a list of events of g9 return them in a 3x3 array index
    '''
    for e in events:
        e.x = e.x-70
        e.y = e.y-70
            
    return 

def unscrable_g1(events):
    '''
    given a list of events of g9 return them in a 3x3 array index
    '''
    for e in events:
        if e.x ==74 and e.y ==67:
            if e.p ==0:
                e.x =0 
                e.y = 0
                e.p = 0
            if e.p ==1:
                e.p = 1
        if e.x == 67 and e.y ==74:
            if e.p ==0:
                e.x =0 
                e.y = 0
                e.p = 0
            if e.p ==1:
                e.p = 1
    return 


def reconstruct_channel(ev,up, dn, vref,sampling_rate=1e-5):
    '''
   reconstruct an input signal
    '''

    up_thr = up -vref 
    dn_thr = vref- dn
    signal_rec = []
    time_rec = []
    current_val = vref

    for ee in ev:
        if ee.p == 1:
            current_val = current_val + up_thr
            signal_rec.append(current_val)
            time_rec.append(ee.ts*sampling_rate ) 
        if ee.p == 0:
            current_val = current_val - dn_thr
            signal_rec.append(current_val)
            time_rec.append( ee.ts*sampling_rate) 
    return signal_rec, time_rec



def time_surfaces(timestamp, x, y, pol, tau=10000 * 1e3, img_size=(128, 128), t_ref=None, t_start=None):
    """
    Args:
        timestamp (us)
        pol
    Return:
        sae : surface of active events
        :param timestamp:
        :param x:
        :param y:
        :param pol:
        :param tau:
        :param img_size:
        :param t_ref:
        :param t_start:
    """
    print("x", x)
    print("y", y)
    print("len : ", len(y))

    sae = np.zeros(img_size, np.float32)
    if np.size(timestamp) > 0:
        if t_ref is None:
            t_ref = timestamp[-1]
        if t_start is None:
            t_start = timestamp[0]

    # If there are events in the selected time window
    idx = np.where(np.logical_and(timestamp >= t_start, timestamp < t_ref))[0]
    for i in idx:
        if pol[i] > 0:
            #sae[y[i], x[i]] = +np.exp(-(t_ref - timestamp[i]) / tau)
             sae[y[i], x[i]] += 1
        else:
            #sae[y[i], x[i]] = -np.exp(-(t_ref - timestamp[i]) / tau)
            sae[y[i], x[i]] -= 1

    return sae

def get_data_arrays(gaia4096):
    ts = np.asarray([gaia4096[i].ts*10 for i in range(len(gaia4096)) ])
    x = np.asarray([gaia4096[i].x for i in range(len(gaia4096)) ])
    y = np.asarray([gaia4096[i].y for i in range(len(gaia4096)) ])
    p = np.asarray([gaia4096[i].p for i in range(len(gaia4096)) ])

    return ts,x,y,p

def generate_movie(t_out, x_out, y_out, p_out,
                   fps=32,
                   img_size=(128, 128),
                   path_to_png='../tmp_gif',
                   path_to_movie="../media/out_edvs.mp4"):
    """
    Generate gif with list of events received
    :param t_out:
    :param x_out:
    :param y_out:
    :param p_out:
    :param fps: frame per sec
    :param path_to_png: folder with images used to generate gif
    :param path_to_movie: path to folder with output video
    :param img_size: size matrix of events
    """
    PATH_TO_FFMPEG = "~/Downloads/ffmpeg_4"


    # Make a custom map
    c = ["darkred","red","lightcoral","white", "palegreen","green","darkgreen"]
    v = [0,.15,.4,.5,0.6,.9,1.]
    l = list(zip(v,c))
    cmap=LinearSegmentedColormap.from_list('rg',l, N=256)

    # Generate movie
    files = glob.glob(path_to_png + '*.png')
    print('Removing files: ', files)
    for f in files:
        os.remove(f)

    t_start = t_out[0]
    t_end = t_out[-1]
    t_steps = np.arange(t_start, t_end, (1 / fps) * 1e6)
    for ii, t in enumerate(t_steps[:-1]):

        fig, axs = plt.subplots(1, 1, sharey='row', dpi=200)

        ax = axs

        idx = np.where(np.logical_and(t_out >= t_steps[ii], t_out < t_steps[ii + 1]))[0]

        t = t_out[idx].astype(int)
        x = x_out[idx].astype(int)
        y = y_out[idx].astype(int)
        p = p_out[idx].astype(int)

        sae = time_surfaces(t, x, y, p, tau=100*1e3, img_size=img_size)

        im = ax.imshow(sae, cmap=cmap, clim=[-2,2])
        ax.set_yticks([0, img_size[0]])
        ax.set_xticks([0, img_size[1]])
        ax.set_xlabel('x')
        ax.set_ylabel('y')
        ax.set_title('t=' + str(t_steps[ii] - t_start) + 'us')
        fig.subplots_adjust(top=0.91, bottom=0.2, right=0.80, left=0.1, hspace=0.5, wspace=0.1)
        fig.set_size_inches(5, 4)

        try:
            os.mkdir(path_to_png)
        except:
            pass

        fig.savefig(path_to_png + "/movie%04d.png" % ii)
        plt.close()

    try:
        os.mkdir('../media/')
    except:
        pass
    os.system(
        PATH_TO_FFMPEG + " -r " + str(
            fps) + " -i " + path_to_png + "/movie%04d.png -c:v libx264 -crf 0 -y " + path_to_movie)
    print('Movie generated at: ' + path_to_movie)