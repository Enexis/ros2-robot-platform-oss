#!/usr/bin/python3

import sys
import datetime 
import matplotlib.pyplot as plt
import matplotlib.dates  as mdates
import matplotlib.ticker as mticker
import numpy             as np
import pathlib           as pth

import utils

sys.dont_write_bytecode = True

# Data Class
##########################################################################################
class Data:
    def __init__(self):
        self.epo0     = None
        self.epo      = []
        self.dNfix    = []
        self.dEfix    = []
        self.dUfix    = []
        self.dNflt    = []
        self.dEflt    = []
        self.dUflt    = []
        self.nFix     = []
        self.conv2D   = {}

    def append(self, epo, dN, dE, dU, isFix, nFix, flgReset):
        self.epo.append(epo)
        self.nFix.append(nFix)
        if isFix:
            self.dNfix.append(dN)
            self.dEfix.append(dE)
            self.dUfix.append(dU)
            self.dNflt.append(np.nan)
            self.dEflt.append(np.nan)
            self.dUflt.append(np.nan)
        else:  
            self.dNflt.append(dN)
            self.dEflt.append(dE)
            self.dUflt.append(dU)
            self.dNfix.append(np.nan)
            self.dEfix.append(np.nan)
            self.dUfix.append(np.nan)
            
        if flgReset or self.epo0 is None:
            self.epo0 = epo

        nSec = int((epo - self.epo0).total_seconds())
        if nSec not in self.conv2D:
            self.conv2D[nSec] = []
        self.conv2D[nSec].append(np.sqrt(dN*dN + dE*dE))
        
# Function to generate the plot
##########################################################################################
def run_plot_client(staName, data, title, timeStr, pngFile, statFile):
    cm = 1/2.54 
    fig, (ax1, ax2, ax3, dummy, ax4) = plt.subplots(5, 1, figsize=(20*cm,20*cm),
                                                    gridspec_kw={'height_ratios': [1, 1, 1, 0.3, 1.5]})
    dummy.axis("off")

    ax1.set_title(title)
    ax1.set_ylabel("North [m]")
    ax1.tick_params(labelbottom=False)
    ax1.plot(data.epo, data.dNflt, color = "crimson", linewidth = 1.0)
    ax1.plot(data.epo, data.dNfix, color = "forestgreen", label = "fixed", linewidth = 1.0)

    ax2.set_ylabel("East [m]")
    ax2.tick_params(labelbottom=False)
    ax2.plot(data.epo, data.dEflt, color = "crimson", label = "float", linewidth = 1.0)
    ax2.plot(data.epo, data.dEfix, color = "forestgreen", linewidth = 1.0)

    ax3.set_xlabel(timeStr)
    ax3.set_ylabel("Height [m]")
    ax3.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M'))
    ax3.plot(data.epo, data.dUflt, color = "crimson", linewidth = 1.0)
    ax3.plot(data.epo, data.dUfix, color = "forestgreen", linewidth = 1.0)

    ax32 = ax3.twinx()
    ax32.plot(data.epo, data.nFix, color = "lightskyblue", label = "% fix", linewidth = 0.4)
    ax32.set_ylim(0,300)
    ax32.set_yticks([0,50,100])
    ax32.legend(loc="upper right")

    for ax in [ax1, ax2, ax3]:
        ax.set_ylim(-0.4, 0.4)
        hh, ll = ax.get_legend_handles_labels()
        if len(hh) > 0:
            ax.legend(loc="upper right")
        ax.grid(axis = 'y', which = 'both', color = 'grey', linestyle = '--', linewidth = 0.3)
        ax.set_yticks([x / 10 for x in range(-4, 5, 2)])
        ax.yaxis.set_minor_locator(mticker.AutoMinorLocator(2))

    maxLen  = 0
    perc    = {68: [], 95: []}
    percMin = {10: {68 : 0.0, 95 : 0.0}, 20: {68 : 0.0, 95 : 0.0}, 30: {68 : 0.0, 95 : 0.0}}
    keyMin  = {10: {68 : 0.0, 95 : 0.0}, 20: {68 : 0.0, 95 : 0.0}, 30: {68 : 0.0, 95 : 0.0}}

    for key in data.conv2D.keys():
        if len(data.conv2D[key]) > maxLen:
            maxLen = len(data.conv2D[key])

        for pKey in (68, 95):
          pp = np.percentile(data.conv2D[key], pKey)
          perc[pKey].append(pp)
          for minute in (10, 20, 30):
              tMin = 60*minute
              if  key in range(60*minute-10, 60*minute+1):
                  if abs(key - tMin) < abs(keyMin[minute][pKey] - tMin):
                      keyMin[minute][pKey] = key
                      percMin[minute][pKey] = pp

    print("%8s   68: %6.3f %6.3f %6.3f    95: %6.3f %6.3f %6.3f" %
          (staName, percMin[10][68], percMin[20][68], percMin[30][68],    percMin[10][95], percMin[20][95], percMin[30][95]))

    for ii in range(0, maxLen):
        dPos = []
        for key in data.conv2D.keys():
            if len(data.conv2D[key]) > ii:
                dPos.append(data.conv2D[key][ii])
            else:
                dPos.append(np.nan)
        ax4.plot(data.conv2D.keys(), dPos, linewidth = 0.5)

    ax4.set_title("Convergence")
    ax4.plot(data.conv2D.keys(), perc[68], linewidth = 1.0, color = "black", label = "68 %")
    ax4.plot(data.conv2D.keys(), perc[95], linewidth = 2.0, color = "black", label = "95 %")
    ax4.legend(loc="upper right")
    ax4.set_ylim(0, 1.0)
    ax4.set_xlabel("Seconds after Reset")
    ax4.set_ylabel("Meters")
    ax4.text( 0.1, 0.7,
              "percentile after 10 min:\n68%% : %8.3f m\n95%% : %8.3f m" % (percMin[10][68], percMin[10][95]),
              transform=ax4.transAxes)
    ax4.text( 0.4, 0.5,
              "percentile after 20 min:\n68%% : %8.3f m\n95%% : %8.3f m" % (percMin[20][68], percMin[20][95]),
              transform=ax4.transAxes)
    ax4.text( 0.7, 0.3,
              "percentile after 30 min:\n68%% : %8.3f m\n95%% : %8.3f m" % (percMin[30][68], percMin[30][95]),
              transform=ax4.transAxes)

    ### plt.show()
    plt.savefig(pngFile)
    plt.close()

    # Output Statistics
    # -----------------
    if statFile:
        with open(statFile, 'w') as outStat:
            print("%s %8.3f %8.3f %8.3f %8.3f %8.3f %8.3f" %
                  (fileName,
                   percMin[10][68], percMin[10][95],
                   percMin[20][68], percMin[20][95],
                   percMin[30][68], percMin[30][95]), file = outStat)
            

# Main Program
##########################################################################################
if __name__ == '__main__':
    import sys
    import argparse
    
    parser = argparse.ArgumentParser()
    parser.add_argument("crdFile")
    parser.add_argument("fileName")
    parser.add_argument("--pngFile",  type=str)
    parser.add_argument("--statFile", type=str)
    parser.add_argument("--title",    type=str)
    args = parser.parse_args()

    crdFile  = args.crdFile
    fileName = args.fileName
    if args.pngFile is None:
        pngFile = pth.Path(fileName).with_suffix(".png")
    else :
        pngFile  = args.pngFile
    if args.title is None:
        title = fileName
    else:
        title = args.title
    statFile = args.statFile
    
    dateFmt  = "%Y-%m-%d_%H:%M:%S.%f"
    dfCrd    = utils.readcrdfile(crdFile)
    station  = None

    # Read Data
    # ---------
    data     = Data()
    flgReset = True
    timeStr  = None
    with open(fileName, 'r') as inFile:
        for line in inFile:
        
            if line.find("RESET FILTER") == 0:
                flgReset = True
        
            elif line.find("X =") >= 0:
                fields = line.split()
        
                dateStr = fields[0]
                staName = fields[1]
                xx      = float(fields[4])
                yy      = float(fields[9])
                zz      = float(fields[14])
                isFix   = (fields[32] == "fix")
                if isFix:
                  numFix = int(fields[33])
                else:
                  numFix = 0

                if station is None:
                    station = utils.getsta(dfCrd, staName)
                    if station is None:
                        raise Exception("Station %s not found" % staName)
                ell = utils.xyz2ell((xx, yy, zz))

                xyz = (xx - station.xx,
                       yy - station.yy,
                       zz - station.zz)
        
                neu = utils.xyz2neu(ell, xyz)

                time = datetime.datetime.strptime(dateStr, dateFmt)
                data.append(time, neu[0], neu[1], neu[2], isFix, numFix, flgReset)
                flgReset = False

                if timeStr is None:
                    timeStr = time.strftime("%Y-%m-%d")

    run_plot_client(staName, data, title, timeStr, pngFile, statFile)
