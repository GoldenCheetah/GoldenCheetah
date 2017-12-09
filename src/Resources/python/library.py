#
# Python class library loaded when the interpreter
# is installed by PythonEmbed

def __GCactivity():
   rd={}
   for x in range(0,GC.seriesLast()):
      if (GC.seriesPresent(x)):
         rd[GC.seriesName(x)] = GC.series(x)
   return rd

# add to main GC entrypoint
GC.activity=__GCactivity
   
