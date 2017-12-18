#
# Python class library loaded when the interpreter
# is installed by PythonEmbed

#--------------------------------------------------
# API for accessing GC data
#--------------------------------------------------

# basic activity data
def __GCactivity(activity=None):
   rd={}
   for x in range(0,GC.seriesLast()):
      if (GC.seriesPresent(x, activity)):
         rd[GC.seriesName(x)] = GC.series(x, activity)
   return rd

# add to main GC entrypoint
GC.activity=__GCactivity
