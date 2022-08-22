#
# Python class library loaded when the interpreter
# is installed by PythonEmbed

#--------------------------------------------------
# API for accessing GC data
#--------------------------------------------------

# basic activity data
def __GCactivity(join="repeat", activity=None):
   rd={}
   for x in range(0,GC.seriesLast()):
      if (GC.seriesPresent(x, activity)):
         rd[GC.seriesName(x)] = GC.series(x, activity)
   for name in GC.xdataNames("", activity):
      for serie in GC.xdataNames(name, activity):
         xd = GC.xdata(name, serie, join, activity)
         rd[str(xd)] = xd
   return rd

# xdata
def __GCactivityXdata(name="", activity=None):
   if not name:
      return GC.xdataNames("")
   rd={}
   for serie in GC.xdataNames(name, activity):
      xd = GC.xdataSeries(name, serie, activity)
      rd[str(xd)] = xd
   return rd

# setting up the chart
def __GCsetChart(title="",type=1,animate=False,legpos=2,stack=False,orientation=2):
    GC.configChart(title,type,animate,legpos,stack,orientation)

# add a curve
def __GCsetCurve(name="",x=list(),y=list(),f=list(),xaxis="x",yaxis="y", labels=list(), colors=list(),line=1,symbol=1,size=15,color="cyan",opacity=0,opengl=True,legend=True,datalabels=False,fill=False):
    if (name == ""):
       raise ValueError("curve 'name' must be set and unique.")
    GC.setCurve(name,list(x),list(y),list(f),xaxis,yaxis,list(labels),list(colors),line,symbol,size,color,opacity,opengl,legend,datalabels,fill)

# setting the axis
def __GCconfigAxis(name,visible=True,align=-1,min=-1,max=-1,type=-1,labelcolor="",color="",log=False,categories=list()):
    if (name == ""):
        raise ValueError("axis 'name' must be passed.")
    GC.configAxis(name, visible, align, min, max, type, labelcolor, color, log, categories)

# adding annotations
def __GCannotate(type="label", series="", label="", value=0):
    GC.addAnnotation(type,series,label,value)

# add to main GC entrypoint
GC.activity=__GCactivity
GC.activityXdata=__GCactivityXdata
GC.setChart=__GCsetChart
GC.addCurve=__GCsetCurve
GC.setAxis=__GCconfigAxis
GC.annotate=__GCannotate

# orientation
GC_HORIZONTAL=1
GC_VERTICAL=2

# line style
GC_LINE_NONE=0
GC_LINE_SOLID=1
GC_LINE_DASH=2
GC_LINE_DOT=3
GC_LINE_DASHDOT=4

# constants
GC_ALIGN_BOTTOM=0
GC_ALIGN_LEFT=1
GC_ALIGN_TOP=2
GC_ALIGN_RIGHT=3
GC_ALIGN_NONE=4

# 0 reserved for uninitialised
GC.CHART_LINE=1
GC.CHART_SCATTER=2
GC.CHART_BAR=3
GC.CHART_PIE=4
GC.CHART_STACK=5
GC.CHART_PERCENT=6

# Axis type
GC.AXIS_CONTINUOUS=0
GC.AXIS_DATE=1
GC.AXIS_TIME=2
GC.AXIS_CATEGORY=3

GC.SERIES_SECS = 0
GC.SERIES_CAD = 1
GC.SERIES_CADD = 2
GC.SERIES_HR = 3
GC.SERIES_HRD = 4
GC.SERIES_KM = 5
GC.SERIES_KPH = 6
GC.SERIES_KPHD = 7
GC.SERIES_NM = 8
GC.SERIES_NMD = 9
GC.SERIES_WATTS = 10
GC.SERIES_WATTSD = 11
GC.SERIES_ALT = 12
GC.SERIES_LON = 13
GC.SERIES_LAT = 14
GC.SERIES_HEADWIND = 15
GC.SERIES_SLOPE = 16
GC.SERIES_TEMP = 17
GC.SERIES_INTERVAL = 18
GC.SERIES_NP = 19
GC.SERIES_XPOWER = 20
GC.SERIES_VAM = 21
GC.SERIES_WATTSKG = 22
GC.SERIES_LRBALANCE = 23
GC.SERIES_LTE = 24
GC.SERIES_RTE = 25
GC.SERIES_LPS = 26
GC.SERIES_RPS = 27
GC.SERIES_APOWER = 28
GC.SERIES_WPRIME = 29
GC.SERIES_ATISS = 30
GC.SERIES_ANTISS = 31
GC.SERIES_SMO2 = 32
GC.SERIES_THB = 33
GC.SERIES_RVERT = 34
GC.SERIES_RCAD = 35
GC.SERIES_RCONTACT = 36
GC.SERIES_GEAR = 37
GC.SERIES_O2HB = 38
GC.SERIES_HHB = 39
GC.SERIES_LPCO = 40
GC.SERIES_RPCO = 41
GC.SERIES_LPPB = 42
GC.SERIES_RPPB = 43
GC.SERIES_LPPE = 44
GC.SERIES_RPPE = 45
GC.SERIES_LPPPB = 46
GC.SERIES_RPPPB = 47
GC.SERIES_LPPPE = 48
GC.SERIES_RPPPE = 49
GC.SERIES_WBAL = 50
GC.SERIES_TCORE = 51
GC.SERIES_CLENGTH = 52
GC.SERIES_APOWERKG = 53
GC.SERIES_INDEX = 54
