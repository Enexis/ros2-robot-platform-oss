
import sys
import pandas    as pd
import numpy     as np
import datetime
import colorsys
import matplotlib
from enum import Enum as Enum

from pandas.plotting import register_matplotlib_converters
register_matplotlib_converters()

sys.dont_write_bytecode = True

##############################################################################################
class GenConst:
    aell = 6378137.000;
    fInv = 298.2572236;
    rho_deg = 180.0 / np.pi;
    def __init__(self):
        raise TypeError("Constants class cannot be instantiated.")
    
    def __setattr__(self, key, value):
        raise AttributeError("Constants cannot be modified.")    

##############################################################################################
class RetVal:
    class Value(Enum):
        SUCCESS = 0
        FAILURE = 1

    def __init__(self):
        self.value  = Value.FAILURE
        self.errmsg = "Uknown error"

    def is_success(self):
        return self.value == VALUE.SUCCESS

    def is_failure(self):
        return self.value == VALUE.FAILURE

    @staticmethod
    def success():
        retval = RetVal()
        retval.value = Value.SUCCESS
        return retval

    @staticmethod
    def failure(errmsg):
        retval = RetVal()
        retval.value  = Value.FAILURE
        retval.errmsg = errmsg
        return retval

##############################################################################################
class SatId:
    def __init__(self, sys, svn):
        if (sys == 'G' or
            sys == 'R' or
            sys == 'E' or
            sys == 'C' or
            sys == 'J' or
            sys == 'I') :
            self.sys = sys
            self.svn = svn
        else:
            self.sys = '?'
            self.svn = 0

    def set(self, satid):
        try:
            if   len(satid) == 0:
                self.sys = '?'
                self.svn = 0
            elif len(satid) == 3:
                if (satid[0] == 'G' or
                    satid[0] == 'R' or
                    satid[0] == 'E' or
                    satid[0] == 'C' or
                    satid[0] == 'J' or
                    satid[0] == 'I') :
                    self.sys = satid[0]
                    self.svn = int(satid[1:])
                else:
                    RaiseException("")
            else:
                RaiseException("")
        except:
            self.sys = '?'
            self.svn = 0

    def __str__(self):
        return "%s%02d" % (self.sys, self.svn)

    def __hash__(self):
        return hash((self.sys, self.svn))

    def __eq__(self,other):
        return self.sys == other.sys and self.svn == other.svn
    

##############################################################################################
class StaInfo:
    def __init__(self, name, xx, yy, zz):
        self.name = name
        self.xx   = xx
        self.yy   = yy
        self.zz   = zz

    def print(self):
        print("%6s  %12.3f %12.3f %12.3f" % (self.name, self.xx, self.yy, self.zz))

##############################################################################################
def readcrdfile(filename):
    df = pd.read_csv(filename, sep="\\s+", header=None, comment='#')
    return df.apply(lambda row: StaInfo(row[0], row[1], row[2], row[3]), axis=1).tolist()

##############################################################################################
def readstatfile(filename):
    df = pd.read_csv(filename, sep="\\s+", header=None, comment='#')
    retval= {}
    for row in df.itertuples():
        ##retval[row._1[0:4]] = [row._2, row._3, row._4, row._5, row._6, row._7]
        retval[row._1] = [row._2, row._3, row._4, row._5, row._6, row._7]
    return retval

##############################################################################################
def getsta(stalist, name):
    for sta in stalist:
        if sta.name == name:
            return sta
    return None
    
##############################################################################################
def xyz2ell(xyz):
  bell = GenConst.aell*(1.0-1.0/GenConst.fInv)
  e2   = (GenConst.aell*GenConst.aell-bell*bell)/(GenConst.aell*GenConst.aell)
  e2c  = (GenConst.aell*GenConst.aell-bell*bell)/(bell*bell)

  if (xyz[0] == 0.0 and xyz[1] == 0.0 and xyz[2] == 0.0):
    return []

  ss    = np.sqrt(xyz[0]*xyz[0]+xyz[1]*xyz[1]) ;
  zps   = xyz[2]/ss ;
  theta = np.arctan( (xyz[2]*GenConst.aell) / (ss*bell) );
  sin3  = np.sin(theta) * np.sin(theta) * np.sin(theta);
  cos3  = np.cos(theta) * np.cos(theta) * np.cos(theta);

  ell = [0, 0, 0];
  ell[0] = np.arctan( (xyz[2] + e2c * bell * sin3) / (ss - e2 * GenConst.aell * cos3) );  
  ell[1] = np.arctan2(xyz[1],xyz[0]) ;
  nn = GenConst.aell/np.sqrt(1.0-e2*np.sin(ell[0])*np.sin(ell[0])) ;
  ell[2] = ss / np.cos(ell[0]) - nn;

  for ii in range(10):
      nn     = GenConst.aell/np.sqrt(1.0-e2*np.sin(ell[0])*np.sin(ell[0]))
      hOld   = ell[2]
      phiOld = ell[0]
      ell[2] = ss/np.cos(ell[0])-nn
      ell[0] = np.arctan(zps/(1.0-e2*nn/(nn+ell[2])))
      if ( abs(phiOld-ell[0]) <= 1.0e-11 and abs(hOld-ell[2]) <= 1.0e-5 ):
          return ell;

##############################################################################################
def ell2xyz(ell):
    bell = GenConst.aell*(1.0-1.0/GenConst.fInv) 

    e2     = (GenConst.aell*GenConst.aell-bell*bell)/(GenConst.aell*GenConst.aell) 
    sinPhi = np.sin(ell[0]) 
    cosPhi = np.sqrt(1.0-sinPhi*sinPhi) 
    nn     = GenConst.aell/np.sqrt(1.0-e2*sinPhi*sinPhi) 

    return [ (nn         + ell[2]) * cosPhi * np.cos(ell[1]),
             (nn         + ell[2]) * cosPhi * np.sin(ell[1]),
             (nn*(1.0-e2)+ ell[2]) * sinPhi ]

##############################################################################################
def mjd2datetime(mjd):
    EPOCH0 = dtatetime.datetime(1980, 1, 6)
    MJD0   = 44244.0
    return EPOCH0 + timedelta(days=mjd-MJD0)

##############################################################################################
def gpswkdow2datetime(gpswk, dow):
    EPOCH0 = datetime.datetime(1980, 1, 6)
    return EPOCH0 + timedelta(days=gpswk*7+dow)

##############################################################################################
def jacobiXyz2neu(ell):
    sinPhi = np.sin(ell[0])
    cosPhi = np.cos(ell[0])
    sinLam = np.sin(ell[1])
    cosLam = np.cos(ell[1])

    return np.array([[-sinPhi*cosLam, -sinPhi*sinLam, cosPhi],
                     [-sinLam,         cosLam,        0.0   ],
                     [cosPhi*cosLam,   cosPhi*sinLam, sinPhi]])

##############################################################################################
def xyz2neu(ell, xyz):
    return jacobiXyz2neu(ell).dot(xyz)

##########################################################################################
def modColor(cc, light, satur):
    hh, ll, ss = colorsys.rgb_to_hls(*matplotlib.colors.to_rgb(cc))
    return colorsys.hls_to_rgb(hh, light*ll, satur)

##############################################################################################                                                                                                         
def currentgpswk():                                                                                                                                                                                    
    now = datetime.now()                                                                                                                                                                               
    return gpswk(now)                                                                                                                                                                                  
                                                                                                                                                                                                       
##############################################################################################                                                                                                         
def currentgpswkdow():                                                                                                                                                                                 
    now = datetime.now()                                                                                                                                                                               
    return gpswkdow(now)                                                                                                                                                                               

##############################################################################################                                                                                                         
def getcurrentsp3file(localoutdir):                                                                                                                                                                    
    tt   = datetime.now(tz=timezone.utc) - timedelta(hours=2)                                                                                                                                          
    tnow = datetime.now(tz=timezone.utc)                                                                                                                                                               
                                                                                                                                                                                                       
    while True:                                                                                                                                                                                        
        gpsw, dow = gpswkdow(tt)                                                                                                                                                                       
        hours = tt.hour                                                                                                                                                                                
        minutes = int(tt.minute  / 15) * 15                                                                                                                                                            
        sp3filenameLocal = "%s/vmcpp%04d%d_%02d%02d.sp3" % (localoutdir, gpsw, dow, hours, minutes)                                                                                                    
        sp3filenameSftp  = "%04d/vmcpp%04d%d_%02d%02d.sp3" % (gpsw, gpsw, dow, hours, minutes)                                                                                                         
                                                                                                                                                                                                       
        if not os.path.exists(sp3filenameLocal):                                                                                                                                                       
            command = "echo -e \"lcd %s\\\\n get %s\" | sftp -i ~/.ssh/ocds_archive.rsa ocdsarchive@napeos.correction-products.veripos.com" % (localoutdir, sp3filenameSftp)                           
            print("Try to download file %s, cmd %s" % (sp3filenameSftp, command))                                                                                                                      
            return_code = subprocess.run(command, shell=True, stdout = subprocess.DEVNULL, stderr = subprocess.DEVNULL)                                                                                
            ###return_code = subprocess.run(command, shell=True)                                                                                                                                       
                                                                                                                                                                                                       
            if os.path.exists(sp3filenameLocal):                                                                                                                                                       
                print("  download succeed")                                                                                                                                                            
            else:                                                                                                                                                                                      
                print("  download failed")                                                                                                                                                             
        else:                                                                                                                                                                                          
            print("File %s exists already\n" % (sp3filenameLocal))                                                                                                                                     
                                                                                                                                                                                                       
        tt = tt + timedelta(minutes=15)                                                                                                                                                                
        if tt > tnow:                                                                                                                                                                                  
            break                                                                                                                                                                                      

##############################################################################################                                                                                                         
def deleteoldfiles(extension: str, nhours: float, dirname: str):                                                                                                                                       
    if not extension.startswith('.'):                                                                                                                                                                  
        extension = '.' + extension                                                                                                                                                                    
                                                                                                                                                                                                       
    dirpath = Path(dirname)                                                                                                                                                                            
    if not dirpath.is_dir():                                                                                                                                                                           
        print(f"Error: Directory not found: {dirname}")                                                                                                                                                
        return                                                                                                                                                                                         
                                                                                                                                                                                                       
    # Compute threshold time in seconds                                                                                                                                                                
    now = time.time()                                                                                                                                                                                  
    age_limit = now - nhours * 3600                                                                                                                                                                    
                                                                                                                                                                                                       
    deleted_count = 0                                                                                                                                                                                  
    for file in dirpath.glob(f"*{extension}"):                                                                                                                                                         
        try:                                                                                                                                                                                           
            mtime = file.stat().st_mtime                                                                                                                                                               
            if mtime < age_limit:                                                                                                                                                                      
                file.unlink()                                                                                                                                                                          
                deleted_count += 1                                                                                                                                                                     
                print(f"Deleted: {file}")                                                                                                                                                              
        except Exception as e:                                                                                                                                                                         
            print(f"Failed to delete {file}: {e}")                                                                                                                                                     
