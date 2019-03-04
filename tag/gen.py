import os
import sys
import pdb
import datetime
import random

basetime = datetime.datetime(2019,1,1,0,0,0)
now = "\"" + basetime.strftime('%Y/%m/%d-%H:%M:%S.%f')[:-3] + "\""

if len(sys.argv) != 2:
    print 'Usage : ' + sys.argv[0] + ' count'
    sys.exit()

loopc = int(sys.argv[1])
tagid = 0
for x in range(0, loopc):
    print "\"TEST^TAG%03d\",%s,%.3f" % (tagid, now, random.uniform(1,100))
    tagid = tagid + 1
    if (x+1) % 100 == 0:
        tagid = 0
        basetime = basetime + datetime.timedelta(0,0,100000)
        now = "\"" + basetime.strftime('%Y/%m/%d-%H:%M:%S.%f')[:-3] + "\""
